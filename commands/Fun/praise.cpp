#include <saber/saber.hpp>

using namespace saber;

struct Praise : Command {
	explicit Praise(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("praise")
					  .category(DIRNAME)
					  .enabled(true)
					  .usage("praise")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (!args.empty()) { return outcome::success(); }

		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .content("https://i.imgur.com/K8ySn3e.gif")
					  .send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Praise)
COMMAND_FREE(Praise)
