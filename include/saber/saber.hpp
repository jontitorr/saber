#ifndef SABER_SABER_HPP
#define SABER_SABER_HPP

#include <ekizu/http_client.hpp>
#include <ekizu/shard.hpp>
#include <saber/commands.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace saber
{
struct Saber {
	Saber(std::string_view token);

	void run();
	void handle_event(ekizu::Event ev);

	ekizu::Snowflake bot_id;
	CommandLoader commands;
	ekizu::HttpClient http;
	std::shared_ptr<spdlog::logger> logger;
	ekizu::Shard shard;
};
} // namespace saber

#endif // SABER_SABER_HPP
