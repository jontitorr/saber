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

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		BOOST_OUTCOME_TRY(bot->http()
							  .create_message(message.channel_id)
							  .content("Pong!")
							  .send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Ping)
COMMAND_FREE(Ping)
