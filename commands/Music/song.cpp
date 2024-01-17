#include <ogg/ogg.h>
#include <opusfile.h>

#include <boost/asio/stream_file.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

static constexpr uint32_t SAMPLE_RATE{48'000};

using namespace saber;

struct Song : Command {
	explicit Song(Saber &creator)
		: Command(
			  creator,
			  CommandOptions{
				  "song",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "song",
				  "Songs the voice channel.",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  3000,
			  }) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		EKIZU_TRY(
			auto guild, bot.http().get_guild(*message.guild_id).send(yield));

		auto voice_state =
			bot.voice_states()
				.get(*message.guild_id)
				.flat_map([&](auto &users) {
					return users.get(message.author.id);
				});

		if (!voice_state) {
			EKIZU_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("You are not in a voice channel!")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		EKIZU_TRY(auto config,
				  bot.join_voice_channel(
					  *message.guild_id, *voice_state->channel_id, yield));

		boost::system::error_code ec;
		boost::asio::stream_file file{yield.get_executor()};
		if (file.open("Camilo, Christian Nodal - La Mitad [xCDXVoc35Vs].opus",
					  boost::asio::stream_file::read_only, ec)) {
			bot.log<ekizu::LogLevel::Error>("Failed to open file");
			return ec;
		}

		ogg_sync_state oy;
		ogg_stream_state os;
		ogg_page og;
		ogg_packet op;
		OpusHead header;

		ogg_sync_init(&oy);

		auto *data = ogg_sync_buffer(&oy, file.size());
		file.async_read_some(boost::asio::buffer(data, file.size()), yield);

		ogg_sync_wrote(&oy, file.size());

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
		EKIZU_TRY(auto conn, config->connect(yield));
		EKIZU_TRY(conn.run(yield));
		EKIZU_TRY(conn.speak(ekizu::SpeakerFlag::Microphone, yield));

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

				EKIZU_TRY(conn.send_opus(
					boost::as_bytes(
						boost::span{op.packet, static_cast<size_t>(op.bytes)}),
					yield));
			}
		}

		bot.log("Played {}ms of audio", time_total);
		EKIZU_TRY(conn.close(yield));
		EKIZU_TRY(bot.leave_voice_channel(*message.guild_id, yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Song)
COMMAND_FREE(Song)
