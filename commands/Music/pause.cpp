#include <saber/saber.hpp>

using namespace saber;

struct Pause : Command {
	explicit Pause(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("pause")
					  .category(DIRNAME)
					  .enabled(true)
					  .guild_only(true)
					  .usage("pause")
					  .description("Pauses the player.")
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
		SABER_TRY(bot.player().pause(*message.guild_id));
		SABER_TRY(bot.http()
					  .create_reaction(message.channel_id, message.id, "⏸️")
					  .send(yield));
		return outcome::success();
	}
};

COMMAND_ALLOC(Pause)
COMMAND_FREE(Pause)
