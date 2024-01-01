#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Countdown : Command {
	explicit Countdown(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "countdown",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "countdown",
				  "Start counting down from 5.",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  0,
			  }) {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (!args.empty()) { return outcome::success(); }

		const std::vector<std::string> countdown{
			"five", "four", "three", "two", "one"};

		boost::asio::deadline_timer timer{yield.get_executor()};

		for (const auto &num : countdown) {
			BOOST_OUTCOME_TRY(bot->http()
								  .create_message(message.channel_id)
								  .content(fmt::format("**:{}:**", num))
								  .send(yield));

			timer.expires_from_now(boost::posix_time::seconds(1));
			timer.async_wait(yield);
		}

		BOOST_OUTCOME_TRY(bot->http()
							  .create_message(message.channel_id)
							  .content("**:ok:** DING DING DING")
							  .send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Countdown)
COMMAND_FREE(Countdown)
