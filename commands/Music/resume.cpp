#include <saber/saber.hpp>

using namespace saber;

struct Resume : Command {
	explicit Resume(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("resume")
					  .category(DIRNAME)
					  .enabled(true)
					  .guild_only(true)
					  .usage("resume")
					  .description("Resumes the player.")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
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
		SABER_TRY(bot.player().resume(*message.guild_id));
		SABER_TRY(bot.http()
					  .create_reaction(message.channel_id, message.id, "▶")
					  .send(yield));
		return outcome::success();
	}
};

COMMAND_ALLOC(Resume)
COMMAND_FREE(Resume)
