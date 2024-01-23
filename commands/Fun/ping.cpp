#include <saber/saber.hpp>

using namespace saber;

struct Ping : Command {
	explicit Ping(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("ping")
					  .category(DIRNAME)
					  .enabled(true)
					  .usage("ping")
					  .description("Replies with pong.")
					  .bot_permissions(ekizu::Permissions::SendMessages |
									   ekizu::Permissions::EmbedLinks)
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .content("Pong!")
					  .send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Ping)
COMMAND_FREE(Ping)
