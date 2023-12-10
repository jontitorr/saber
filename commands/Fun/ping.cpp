#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Ping : Command {
	explicit Ping(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "ping",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "ping",
				  "Replies with pong.",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  3000,
			  }) {}

	void setup() override {}

	void execute(
		const ekizu::Message &message,
		[[maybe_unused]] const std::vector<std::string> &args) override {
		(void)bot->http.create_message(message.channel_id)
			.content("Pong!")
			.send();
	}
};

COMMAND_ALLOC(Ping)
COMMAND_FREE(Ping)
