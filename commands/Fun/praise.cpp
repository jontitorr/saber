#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Praise : Command {
	explicit Praise(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "praise",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "praise",
				  "",
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

		BOOST_OUTCOME_TRY(bot->http()
							  .create_message(message.channel_id)
							  .content("https://i.imgur.com/K8ySn3e.gif")
							  .send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Praise)
COMMAND_FREE(Praise)
