#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Join : Command {
	explicit Join(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("join")
					  .dirname(DIRNAME)
					  .enabled(true)
					  .usage("join")
					  .description("Joins the voice channel.")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(3000)
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		EKIZU_TRY(
			auto guild, bot.http().get_guild(*message.guild_id).send(yield));

		auto voice_state =
			bot.voice_state_cache()
				.get(*message.guild_id)
				.flat_map([&](auto &users) {
					return users.get(message.author.id);
				});

		if (!voice_state) {
			EKIZU_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("You are not in a voice channel!")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		EKIZU_TRY(bot.join_voice_channel(
			*message.guild_id, *voice_state->channel_id, yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Join)
COMMAND_FREE(Join)
