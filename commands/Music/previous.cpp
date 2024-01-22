#include <saber/saber.hpp>

using namespace saber;

struct Previous : Command {
	explicit Previous(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("previous")
					  .category(DIRNAME)
					  .enabled(true)
					  .guild_only(true)
					  .usage("previous")
					  .description("Go to the previous song.")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		SABER_TRY(
			auto guild, bot.http().get_guild(*message.guild_id).send(yield));

		auto voice_state =
			bot.voice_states()
				.get(*message.guild_id)
				.flat_map([&](auto &users) {
					return users.get(message.author.id);
				});

		if (!voice_state) {
			SABER_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("You are not in a voice channel!")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		SABER_TRY(bot.player().connect(
			*message.guild_id, *voice_state->channel_id, yield));
		return bot.player().previous(*message.guild_id);
	}
};

COMMAND_ALLOC(Previous)
COMMAND_FREE(Previous)
