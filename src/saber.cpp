#include <boost/asio/detached.hpp>
#include <saber/saber.hpp>

namespace {
template <typename... Func>
struct overload : Func... {
	using Func::operator()...;
};

template <typename... Func>
overload(Func...) -> overload<Func...>;
}  // namespace

namespace saber {
Saber::Saber(std::string_view token)
	: commands{this},
	  http{token},
	  shard{ekizu::ShardId::ONE, token, ekizu::Intents::AllIntents} {
	if (const auto *owner_id_e = std::getenv("OWNER_ID");
		owner_id_e != nullptr) {
		std::string_view owner_id_str{owner_id_e};
		owner_id = ekizu::Snowflake{std::stoull(std::string{owner_id_str})};
	}

	logger = spdlog::stdout_color_mt("saber");
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

#ifdef _DEBUG
	auto level = spdlog::level::debug;
#else
	auto level = spdlog::level::info;
#endif

	if (const auto *log_level_e = std::getenv("SABER_LOG_LEVEL");
		log_level_e != nullptr) {
		std::string_view log_level{log_level_e};

		if (log_level == "info") {
			level = spdlog::level::info;
		} else if (log_level == "warn") {
			level = spdlog::level::warn;
		} else if (log_level == "error") {
			level = spdlog::level::err;
		} else if (log_level == "debug") {
			level = spdlog::level::debug;
		} else if (log_level == "trace") {
			level = spdlog::level::trace;
		} else if (log_level == "critical") {
			level = spdlog::level::critical;
		} else if (log_level == "off") {
			level = spdlog::level::off;
		}
	}

	spdlog::set_level(level);

	shard.attach_logger([this](const ekizu::Log &log) {
		switch (log.level) {
			case ekizu::LogLevel::Info: logger->info(log.message); break;
			case ekizu::LogLevel::Warn: logger->warn(log.message); break;
			case ekizu::LogLevel::Error: logger->error(log.message); break;
			case ekizu::LogLevel::Debug: logger->debug(log.message); break;
			case ekizu::LogLevel::Trace: logger->trace(log.message); break;
			case ekizu::LogLevel::Critical:
				logger->critical(log.message);
				break;
		}
	});
}

void Saber::run(const boost::asio::yield_context &yield) {
	commands.load_all(yield);

	while (true) {
		auto res = shard.next_event(yield);

		if (!res) {
			if (res.error().failed()) {
				fmt::println(
					"Failed to get next event: {}", res.error().message());
				return;
			}
			// Could be handling a non-dispatch event.
			continue;
		}

		boost::asio::spawn(
			yield,
			[this, e = std::move(res.value())](auto y) { handle_event(e, y); },
			boost::asio::detached);
	}
}

void Saber::handle_event(ekizu::Event ev,
						 const boost::asio::yield_context &yield) {
	std::visit(
		overload{[this](const ekizu::Ready &r) {
					 user = r.user;
					 logger->info("Logged in as {}", user.username);
					 bot_id = user.id;
					 logger->info("API version: {}", r.v);
					 logger->info("Guilds: {}", r.guilds.size());
				 },
				 [this, &yield](const ekizu::MessageCreate &m) {
					 messages_cache.put(m.message.id, m.message);
					 users_cache.put(m.message.author.id, m.message.author);
					 commands.process_commands(m.message, yield);
				 },
				 [this](ekizu::Resumed) { logger->info("Resumed"); },
				 [this](const auto &e) {
					 logger->warn("Unhandled event: {}", typeid(e).name());
				 }},
		ev);
}
}  // namespace saber
