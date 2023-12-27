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
	logger = spdlog::stdout_color_mt("logger");
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
	spdlog::set_level(spdlog::level::debug);
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
				 [this](const ekizu::Log &l) { logger->debug(l.message); },
				 [this](ekizu::Resumed) { logger->info("Resumed"); },
				 [this](const auto &e) {
					 logger->warn("Unhandled event: {}", typeid(e).name());
				 }},
		ev);
}
}  // namespace saber
