#include <ogg/ogg.h>
#include <opusfile.h>

#include <boost/asio/detached.hpp>
#include <boost/beast/core/file.hpp>
#include <boost/process/v2.hpp>
#include <boost/scope_exit.hpp>
#include <saber/player.hpp>

namespace {
constexpr uint32_t SAMPLE_RATE{48'000};
namespace asio = boost::asio;
}  // namespace

namespace saber {
Track GuildQueue::add_track(Track track) {
	track.id = last_track_id++;
	if (!current_track_id) { current_track_id = track.id; }
	return track;
}

std::optional<uint64_t> GuildQueue::skip(uint64_t track_id) {
	// Check if we're playing and we're not the last track.
	if (!current_track_id || current_track_id == last_track_id) {
		return std::nullopt;
	}

	auto ret = *current_track_id;

	// Check if track_id is within our bounds. Going backwards is possible.
	if (track_id >= last_track_id || track_id == ret) { return std::nullopt; }
	current_track_id = track_id;
	return ret;
}

PlayerConnection::PlayerConnection(ekizu::VoiceConnectionConfig config,
								   GuildQueue& queue)
	: m_queue{queue}, m_config{std::move(config)} {}

Result<> PlayerConnection::pause() {
	if (m_pause_timer) {
		m_pause_timer->expires_at(boost::posix_time::pos_infin);
	}
	return outcome::success();
}

Result<> PlayerConnection::resume() {
	if (m_pause_timer) {
		m_pause_timer->expires_at(boost::posix_time::neg_infin);
	}
	return outcome::success();
}

Result<> PlayerConnection::send_track_data(TrackData data,
										   const asio::yield_context& yield) {
	if (!m_pause_timer) {
		m_pause_timer.emplace(yield.get_executor());
		m_pause_timer->expires_at(boost::posix_time::neg_infin);
	}

	// Only finish packet can be empty.
	if (data.data.empty() && !data.finished) {
		return boost::system::errc::invalid_argument;
	}

	if (!m_queue.current_track_id) { m_queue.current_track_id = data.track_id; }

	boost::system::error_code ec;
	++m_tasks;

	if (m_tasks == 1) {
		if (!m_task_timer) { m_task_timer.emplace(yield.get_executor()); }
		m_task_timer->expires_at(boost::posix_time::pos_infin);
	} else {
		m_task_timer->async_wait(yield[ec]);
		if (ec != asio::error::operation_aborted) { return ec; }
	}

	return do_send(std::move(data), yield);
}

Result<> PlayerConnection::do_send(TrackData data,
								   const boost::asio::yield_context& yield) {
	BOOST_SCOPE_EXIT_ALL(this) {
		--m_tasks;
		m_task_timer->cancel_one();
	};

	// Enqueue the data if it's not related to the current one.
	if (m_queue.current_track_id != data.track_id) {
		// Skip a finished non-pending track.
		if (m_pending_tracks.find(data.track_id) == m_pending_tracks.end() &&
			data.finished) {
			return outcome::success();
		}

		m_pending_tracks[data.track_id].emplace(std::move(data));
		return outcome::success();
	}

	// Proceed as normal.
	if (!data.finished) { return send(data, yield); }

	// Decrement the last_track_id.
	--m_queue.last_track_id;

	// If we have no more tracks, reset to original state.
	if (m_pending_tracks.empty()) {
		m_queue.current_track_id.reset();
		SABER_TRY(m_voice_connection->silence(yield));
		SABER_TRY(m_voice_connection->speak(ekizu::SpeakerFlag::None, yield));
		m_speaking = false;
		fmt::println(
			"Reset to original state since there are no more songs to "
			"play.");
		return outcome::success();
	}

	// Play the next track.
	auto& [track_id, queue] = *m_pending_tracks.begin();
	m_queue.current_track_id = track_id;
	fmt::println("Now playing track {}.", track_id);

	BOOST_SCOPE_EXIT_ALL(this, tid = track_id) { m_pending_tracks.erase(tid); };

	while (!queue.empty()) {
		SABER_TRY(send(queue.front(), yield));

		// TODO: Handle the finished flag if received.
		if (queue.front().finished) {
			fmt::println("Received finish flag when running queue.");
			// return outcome::success();
		}

		queue.pop();
	}

	return outcome::success();
}

Result<> PlayerConnection::send(const TrackData& data,
								const boost::asio::yield_context& yield) {
	if (!m_voice_connection) {
		SABER_TRY(auto conn, m_config.connect(yield));
		m_voice_connection.emplace(std::move(conn));
		SABER_TRY(m_voice_connection->run(yield));
	}

	if (!m_speaking) {
		SABER_TRY(
			m_voice_connection->speak(ekizu::SpeakerFlag::Microphone, yield));
		fmt::println("Speaking");
		m_speaking = true;
	}

	if (m_pause_timer) {
		boost::system::error_code ec;
		m_pause_timer->async_wait(yield[ec]);
		if (ec && ec != asio::error::operation_aborted) { return ec; }
	}

	return m_voice_connection->send_opus(data.data, yield);
}

Player::Player(
	std::function<Result<ekizu::VoiceConnectionConfig*>(
		ekizu::Snowflake, ekizu::Snowflake, const asio::yield_context&)>
		connector)
	: m_connector(std::move(connector)) {}

Result<std::optional<uint64_t>> Player::current_track_id(
	ekizu::Snowflake guild_id) {
	if (m_queues.find(guild_id) == m_queues.end()) {
		return boost::system::errc::operation_not_permitted;
	}

	return m_queues[guild_id].current_track_id;
}

Result<bool> Player::connect(ekizu::Snowflake guild_id,
							 ekizu::Snowflake channel_id,
							 const asio::yield_context& yield) {
	if (m_connections.has(guild_id)) { return false; }

	SABER_TRY(auto config, m_connector(guild_id, channel_id, yield));

	m_connections.emplace(guild_id, *config, m_queues[guild_id]);
	return true;
}

Result<Track> Player::play(ekizu::Snowflake guild_id, std::string_view query,
						   ekizu::Snowflake requester_id,
						   const asio::yield_context& yield) {
	if (!m_connections.has(guild_id)) {
		return boost::system::errc::operation_not_permitted;
	}

	auto track =
		m_queues[guild_id].add_track({{}, requester_id, std::string{query}});

	asio::spawn(
		yield,
		[this, guild_id, q = track.url, requester_id, tid = track.id](auto y) {
			(void)play_sync(guild_id, q, requester_id, tid, y);
		},
		asio::detached);

	return track;
}

Result<> Player::pause(ekizu::Snowflake guild_id) {
	if (!m_connections.has(guild_id)) {
		return boost::system::errc::no_such_file_or_directory;
	}
	return m_connections[guild_id]->pause();
}

Result<> Player::resume(ekizu::Snowflake guild_id) {
	if (!m_connections.has(guild_id)) {
		return boost::system::errc::no_such_file_or_directory;
	}
	return m_connections[guild_id]->resume();
}

Result<> Player::skip(ekizu::Snowflake guild_id) {
	if (!m_queues.contains(guild_id)) {
		return boost::system::errc::no_such_file_or_directory;
	}
	m_queues[guild_id].skip();
	return outcome::success();
}

Result<> Player::previous(ekizu::Snowflake guild_id) {
	if (!m_queues.contains(guild_id)) {
		return boost::system::errc::no_such_file_or_directory;
	}
	m_queues[guild_id].previous();
	return outcome::success();
}

Result<std::string> Player::download_song(std::string_view query,
										  const asio::yield_context& yield) {
	namespace bp = boost::process::v2;

	boost::system::error_code ec;
	auto exe = bp::environment::find_executable("yt-dlp");
	if (exe.empty()) { return boost::system::errc::no_such_file_or_directory; }

	asio::readable_pipe rp{yield.get_executor()};
	bp::process proc(
		yield.get_executor(), exe,
		{fmt::format("ytsearch1:\"{}\"", query), "-x", "--audio-format", "opus",
		 "-f", "bestaudio", "-o", "%(id)s.f%(format_id)s.opus", "-O",
		 "%(id)s.f%(format_id)s.opus", "--no-simulate"},
		bp::process_stdio{{}, rp, {}});

	auto code = proc.async_wait(yield[ec]);
	if (code != 0) { return boost::system::errc::executable_format_error; }
	if (ec) { return ec; }

	size_t bytes{};
	std::string song_name(128, '\0');

	while (true) {
		auto got = rp.async_read_some(
			asio::buffer(song_name.data() + bytes, song_name.size() - bytes),
			yield[ec]);
		bytes += got;
		if (ec && ec != asio::error::broken_pipe) { return ec; }
		if (got < song_name.size()) { break; }
		song_name.resize(song_name.size() * 2);
	}

	song_name.resize(bytes - 1);
	return song_name;
}

Result<> Player::play_sync(ekizu::Snowflake guild_id, std::string_view query,
						   ekizu::Snowflake requester_id, uint64_t track_id,
						   const boost::asio::yield_context& yield) {
	SABER_TRY(auto song_name, download_song(query, yield));
	boost::system::error_code ec;
	boost::beast::file file;

	file.open(song_name.data(), boost::beast::file_mode::read, ec);
	if (ec) { return ec; }

	auto sz = static_cast<long>(file.size(ec));
	if (ec) { return ec; }

	ogg_sync_state oy;
	ogg_stream_state os;
	ogg_page og;
	ogg_packet op;
	OpusHead header;

	BOOST_SCOPE_EXIT_ALL(&) {
		ogg_stream_clear(&os);
		ogg_sync_clear(&oy);
	};

	ogg_sync_init(&oy);

	auto* data = ogg_sync_buffer(&oy, sz);

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

	if (opus_head_parse(&header, op.packet, static_cast<size_t>(op.bytes)) !=
		OPUS_OK) {
		return boost::system::errc::io_error;
	}

	if (header.channel_count != 2 || header.input_sample_rate != SAMPLE_RATE) {
		return boost::system::errc::io_error;
	}

	auto& conn = *m_connections[guild_id];

	BOOST_SCOPE_EXIT_ALL(&) {
		(void)conn.send_track_data({{}, true, requester_id, track_id}, yield);
	};

	while (ogg_sync_pageout(&oy, &og) == 1) {
		ogg_stream_init(&os, ogg_page_serialno(&og));

		if (ogg_stream_pagein(&os, &og) < 0) {
			return boost::system::errc::io_error;
		}

		while (ogg_stream_packetout(&os, &op) != 0) {
			if (op.bytes > 8 && std::memcmp("OpusHead", op.packet, 8) == 0) {
				if (opus_head_parse(&header, op.packet,
									static_cast<size_t>(op.bytes)) != OPUS_OK) {
					return boost::system::errc::io_error;
				}

				if (header.channel_count != 2 ||
					header.input_sample_rate != SAMPLE_RATE) {
					return boost::system::errc::io_error;
				}

				continue;
			}

			if (op.bytes > 8 && std::memcmp("OpusTags", op.packet, 8) == 0) {
				continue;
			}

			auto span = boost::as_bytes(
				boost::span{op.packet, static_cast<size_t>(op.bytes)});

			SABER_TRY(conn.send_track_data(
				{{span.begin(), span.end()}, {}, requester_id, track_id},
				yield));
		}
	}

	return outcome::success();
}
}  // namespace saber
