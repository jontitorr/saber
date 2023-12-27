#include <ekizu/async_main.hpp>
#include <saber/saber.hpp>

async_main(const boost::asio::yield_context& yield) {
	const auto* token = std::getenv("DISCORD_TOKEN");

	if (token == nullptr) {
		fmt::println("Missing DISCORD_TOKEN environment variable");
		return boost::system::errc::invalid_argument;
	}

	auto saber = saber::Saber{token};
	saber.run(yield);

	return ekizu::outcome::success();
}
