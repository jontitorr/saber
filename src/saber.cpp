#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

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
Saber::Saber(Config config)
	: m_commands{this},
	  m_http{config.token},
	  m_shard{ekizu::ShardId::ONE, config.token, ekizu::Intents::AllIntents},
	  m_config{std::move(config)} {
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

	auto file_sink =
		std::make_shared<spdlog::sinks::basic_file_sink_mt>("saber.log");
	file_sink->set_level(spdlog::level::trace);

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

	console_sink->set_level(level);

	m_shard.attach_logger([this](const ekizu::Log &log) {
		switch (log.level) {
			case ekizu::LogLevel::Info: m_logger->info(log.message); break;
			case ekizu::LogLevel::Warn: m_logger->warn(log.message); break;
			case ekizu::LogLevel::Error: m_logger->error(log.message); break;
			case ekizu::LogLevel::Debug: m_logger->debug(log.message); break;
			case ekizu::LogLevel::Trace: m_logger->trace(log.message); break;
			case ekizu::LogLevel::Critical:
				m_logger->critical(log.message);
				break;
		}
	});

	m_logger = spdlog::logger{"saber", {console_sink, file_sink}};
	m_logger->set_level(level);
}

void Saber::run(const boost::asio::yield_context &yield) {
	m_commands.load_all(yield);

	while (true) {
		auto res = m_shard.next_event(yield);

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

	spdlog::shutdown();
}

void Saber::handle_event(ekizu::Event ev,
						 const boost::asio::yield_context &yield) {
	std::visit(
		overload{
			[this](const ekizu::Ready &r) {
				m_user = r.user;
				log<ekizu::LogLevel::Info>("Logged in as {}", m_user.username);
				m_bot_id = m_user.id;
				log<ekizu::LogLevel::Info>("API version: {}", r.v);
				log<ekizu::LogLevel::Info>("Guilds: {}", r.guilds.size());
			},
			[this, &yield](const ekizu::MessageCreate &m) {
				m_messages_cache.put(m.message.id, m.message);
				m_users_cache.put(m.message.author.id, m.message.author);
				m_commands.process_commands(m.message, yield);
			},
			[this](ekizu::Resumed) { log<ekizu::LogLevel::Info>("Resumed"); },
			[this](const auto &e) {
				log<ekizu::LogLevel::Warn>(
					"Unhandled event: {}", typeid(e).name());
			}},
		ev);
}
}  // namespace saber
