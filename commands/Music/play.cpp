#include <ogg/ogg.h>
#include <opusfile.h>

#include <boost/asio/stream_file.hpp>
#include <boost/process/v2.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

#include "saber/commands.hpp"

static constexpr uint32_t SAMPLE_RATE{48'000};

using namespace saber;

struct Play : Command {
	explicit Play(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("play")
					  .dirname(DIRNAME)
					  .enabled(true)
					  .guild_only(true)
					  .usage("play <query>")
					  .description("Play a youtube video in the voice channel.")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		SABER_TRY(
			auto guild, bot.http().get_guild(*message.guild_id).send(yield));

		auto voice_state =
			bot.voice_states()
				.get(*message.guild_id)
				.flat_map([&](auto &users) {
					return users.get(message.author.id);
				});

		if (!voice_state) {
			SABER_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("You are not in a voice channel!")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		if (args.empty()) {
			SABER_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("Please specify a query.")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		const auto &query = args[0];

		namespace bp = boost::process::v2;

		boost::system::error_code ec;
		auto exe = bp::environment::find_executable("yt-dlp");
		if (exe.empty()) {
			return boost::system::errc::no_such_file_or_directory;
		}

		boost::asio::readable_pipe rp{yield.get_executor()};
		bp::process proc(
			yield.get_executor(), exe,
			{query, "-x", "--audio-format", "opus", "-f", "bestaudio", "-o",
			 "%(title)s.f%(format_id)s.opus", "-O",
			 "%(title)s.f%(format_id)s.opus", "--no-simulate"},
			bp::process_stdio{{}, rp, {}});
		auto code = proc.async_wait(yield[ec]);
		if (code != 0) { return boost::system::errc::executable_format_error; }
		if (ec) { return ec; }

		size_t bytes{};
		std::string song_name(256, '\0');

		while (true) {
			auto got = rp.async_read_some(
				boost::asio::buffer(
					song_name.data() + bytes, song_name.size() - bytes),
				yield[ec]);
			bytes += got;
			if (ec && ec != boost::asio::error::broken_pipe) { return ec; }
			if (got < song_name.size()) { break; }
			song_name.resize(song_name.size() * 2);
		}

		song_name.resize(bytes - 1);

		boost::beast::file file;
		// TODO: Use boost::asio::stream_file file with uring support for
		// async I/O.

		file.open(song_name.c_str(), boost::beast::file_mode::read, ec);
		if (ec) { return ec; }

		auto sz = file.size(ec);
		if (ec) { return ec; }

		ogg_sync_state oy;
		ogg_stream_state os;
		ogg_page og;
		ogg_packet op;
		OpusHead header;

		ogg_sync_init(&oy);

		auto *data = ogg_sync_buffer(&oy, sz);
		// file.async_read_some(boost::asio::buffer(data, sz), yield);
		file.read(data, sz, ec);
		if (ec) { return ec; }

		ogg_sync_wrote(&oy, sz);

		if (ogg_sync_pageout(&oy, &og) != 1) {
			return boost::system::errc::io_error;
		}

		ogg_stream_init(&os, ogg_page_serialno(&og));

		if (ogg_stream_pagein(&os, &og) < 0 ||
			ogg_stream_packetout(&os, &op) != 1) {
			return boost::system::errc::io_error;
		}

		if (op.bytes < 8 || std::memcmp(op.packet, "OpusHead", 8) != 0) {
			return boost::system::errc::io_error;
		}

		if (opus_head_parse(
				&header, op.packet, static_cast<size_t>(op.bytes)) != OPUS_OK) {
			return boost::system::errc::io_error;
		}

		if (header.channel_count != 2 ||
			header.input_sample_rate != SAMPLE_RATE) {
			return boost::system::errc::io_error;
		}

		int time_total{};

		// Start our voice connection.
		SABER_TRY(auto config,
				  bot.join_voice_channel(
					  *message.guild_id, *voice_state->channel_id, yield));
		SABER_TRY(auto conn, config->connect(yield));
		SABER_TRY(conn.run(yield));
		SABER_TRY(conn.speak(ekizu::SpeakerFlag::Microphone, yield));

		while (ogg_sync_pageout(&oy, &og) == 1) {
			// Start a new stream, using the serial number from the page.
			ogg_stream_init(&os, ogg_page_serialno(&og));

			if (ogg_stream_pagein(&os, &og) < 0) {
				return boost::system::errc::io_error;
			}

			while (ogg_stream_packetout(&os, &op) != 0) {
				// Read the remaining headers.
				if (op.bytes > 8 &&
					std::memcmp("OpusHead", op.packet, 8) == 0) {
					if (opus_head_parse(&header, op.packet,
										static_cast<size_t>(op.bytes)) !=
						OPUS_OK) {
						return boost::system::errc::io_error;
					}

					// Now we make sure the encoding is correct for Discord.
					if (header.channel_count != 2 ||
						header.input_sample_rate != SAMPLE_RATE) {
						return boost::system::errc::io_error;
					}

					continue;
				}

				// Skip the Opus tags.
				if (op.bytes > 8 &&
					std::memcmp("OpusTags", op.packet, 8) == 0) {
					continue;
				}

				const auto samples =
					opus_packet_get_samples_per_frame(op.packet, SAMPLE_RATE);
				const auto duration = samples / 48;
				time_total += duration;

				SABER_TRY(conn.send_opus(
					boost::as_bytes(
						boost::span{op.packet, static_cast<size_t>(op.bytes)}),
					yield));
			}
		}

		bot.log("Played {}ms of audio", time_total);
		SABER_TRY(conn.close(yield));
		SABER_TRY(bot.leave_voice_channel(*message.guild_id, yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Play)
COMMAND_FREE(Play)
