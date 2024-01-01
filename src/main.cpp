#include <ekizu/async_main.hpp>
#include <saber/saber.hpp>

async_main(const boost::asio::yield_context& yield) {
	BOOST_OUTCOME_TRY(auto config, saber::Config::from_file("config.json"));

	if (config.token.empty()) {
		fmt::println(
			"Missing `token` in config.json. Please include or use the "
			"`SABER_TOKEN` environment variable.");
		return boost::system::errc::invalid_argument;
	}

	auto saber = saber::Saber{config};
	saber.run(yield);

	return ekizu::outcome::success();
}
