#ifndef SABER_PLAYER_HPP
#define SABER_PLAYER_HPP

#include <saber/export.h>

#include <boost/unordered/unordered_flat_map.hpp>
#include <ekizu/lru_cache.hpp>
#include <ekizu/shard.hpp>
#include <ekizu/voice_connection.hpp>
#include <queue>
#include <saber/result.hpp>

namespace saber {
struct Track {
	/// The ID of the track.
	uint64_t id{};
	/// The ID of the requester.
	ekizu::Snowflake requester_id{};
	/// To be replaced with an actual URL in the future.
	std::string url{};
};

struct GuildQueue {
	Track add_track(Track track);
	std::optional<uint64_t> skip(uint64_t track_id);
	std::optional<uint64_t> skip() { return skip(*current_track_id + 1); }
	std::optional<uint64_t> previous() { return skip(*current_track_id - 1); }

	std::optional<uint64_t> current_track_id;
	uint64_t last_track_id{};
};

struct TrackData {
	std::vector<std::byte> data;
	bool finished{};
	ekizu::Snowflake requester_id;
	uint64_t track_id{};
};

struct PlayerConnection {
	SABER_EXPORT explicit PlayerConnection(ekizu::VoiceConnectionConfig config,
										   GuildQueue& queue);

	SABER_EXPORT Result<> pause();
	SABER_EXPORT Result<> resume();
	SABER_EXPORT Result<> send_track_data(
		TrackData data, const boost::asio::yield_context& yield);

   private:
	Result<> do_send(TrackData data, const boost::asio::yield_context& yield);
	Result<> send(const TrackData& data,
				  const boost::asio::yield_context& yield);

	GuildQueue& m_queue;
	ekizu::VoiceConnectionConfig m_config;
	std::optional<ekizu::VoiceConnection> m_voice_connection;
	bool m_speaking{};
	std::map<uint64_t, std::queue<TrackData>> m_pending_tracks;
	uint64_t m_tasks{};
	std::optional<boost::asio::deadline_timer> m_task_timer;
	std::optional<boost::asio::deadline_timer> m_pause_timer;
};

/**
 * @brief Ideally a music player.
 */
struct Player {
	SABER_EXPORT explicit Player(
		std::function<Result<ekizu::VoiceConnectionConfig*>(
			ekizu::Snowflake, ekizu::Snowflake,
			const boost::asio::yield_context&)>
			connector);

	SABER_EXPORT Result<std::optional<uint64_t>> current_track_id(
		ekizu::Snowflake guild_id);

	SABER_EXPORT Result<bool> connect(ekizu::Snowflake guild_id,
									  ekizu::Snowflake channel_id,
									  const boost::asio::yield_context& yield);

	SABER_EXPORT Result<Track> play(
		ekizu::Snowflake guild_id, std::string_view query,
		ekizu::Snowflake requester_id, const boost::asio::yield_context& yield);
	SABER_EXPORT Result<> pause(ekizu::Snowflake guild_id);
	SABER_EXPORT Result<> resume(ekizu::Snowflake guild_id);
	SABER_EXPORT Result<> skip(ekizu::Snowflake guild_id);
	SABER_EXPORT Result<> previous(ekizu::Snowflake guild_id);

   private:
	Result<std::string> download_song(std::string_view query,
									  const boost::asio::yield_context& yield);
	Result<> play_sync(ekizu::Snowflake guild_id, std::string_view query,
					   ekizu::Snowflake requester_id, uint64_t track_id,
					   const boost::asio::yield_context& yield);

	std::function<Result<ekizu::VoiceConnectionConfig*>(
		ekizu::Snowflake, ekizu::Snowflake, const boost::asio::yield_context&)>
		m_connector;
	ekizu::SnowflakeLruCache<PlayerConnection> m_connections{500};
	boost::unordered_flat_map<ekizu::Snowflake, GuildQueue> m_queues;
};
}  // namespace saber

#endif	// SABER_PLAYER_HPP
