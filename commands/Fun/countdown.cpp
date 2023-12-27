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

	void execute(const ekizu::Message &message,
				 const std::vector<std::string> &args,
				 const boost::asio::yield_context &yield) override {
		if (!args.empty()) { return; }

		const std::vector<std::string> countdown{
			"five", "four", "three", "two", "one"};

		for (const std::string &num : countdown) {
			(void)bot->http.create_message(message.channel_id)
				.content("**:" + num + ":**")
				.send(yield);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		(void)bot->http.create_message(message.channel_id)
			.content("**:ok:** DING DING DING")
			.send(yield);
	}
};

COMMAND_ALLOC(Countdown)
COMMAND_FREE(Countdown)
