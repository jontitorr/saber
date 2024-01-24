#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <saber/saber.hpp>

int main() {
	boost::asio::io_context ctx;
	auto config = saber::Config::from_file("config.json");

	if (!config || config.value().token.empty()) {
		fmt::println(
			"Missing `token` in config.json. Please include or use the "
			"`SABER_TOKEN` environment variable.");
		return boost::system::errc::invalid_argument;
	}

	auto saber = saber::Saber{config.value()};

	spawn(
		ctx, [&saber](auto y) { saber.run(y); }, boost::asio::detached);

	// TODO: Implement graceful shutdown
	boost::asio::signal_set signals(ctx, SIGINT, SIGTERM);
	signals.async_wait([&ctx](const boost::system::error_code& ec, int) {
		if (!ec) { ctx.stop(); }
	});

	ctx.run();
}
