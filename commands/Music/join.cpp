#include <saber/saber.hpp>

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
