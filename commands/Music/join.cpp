#include <saber/util.hpp>

using namespace saber;

struct Join : Command {
	explicit Join(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("join")
					  .category(DIRNAME)
					  .enabled(true)
					  .guild_only(true)
					  .usage("join")
					  .description("Joins the voice channel.")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		SABER_TRY(auto guild, bot.get_guild(*message.guild_id, yield));
		SABER_TRY(
			auto voice_state, util::in_voice_channel(bot, message, yield));
		SABER_TRY(auto just_connected,
				  bot.player().connect(
					  *message.guild_id, *voice_state->channel_id, yield));

		if (!just_connected) {
			SABER_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("I am already in a voice channel!")
						  .reply(message.id)
						  .send(yield));
		}

		return outcome::success();
	}
};

COMMAND_ALLOC(Join)
COMMAND_FREE(Join)
