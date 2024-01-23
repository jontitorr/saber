#include <boost/algorithm/string/join.hpp>
#include <ekizu/embed_builder.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Play : Command {
	explicit Play(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("play")
					  .category(DIRNAME)
					  .enabled(true)
					  .guild_only(true)
					  .usage("play <query>")
					  .description("Play a youtube video in the voice channel.")
					  .bot_permissions(ekizu::Permissions::SendMessages |
									   ekizu::Permissions::EmbedLinks)
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		SABER_TRY(
			auto voice_state, util::in_voice_channel(bot, message, yield));
		if (args.empty()) {
			SABER_TRY(bot.http()
						  .create_message(message.channel_id)
						  .content("Please specify a query.")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		auto query = boost::algorithm::join(args, " ");

		SABER_TRY(bot.player().connect(
			*message.guild_id, *voice_state->channel_id, yield));
		SABER_TRY(auto track, bot.player().play(*message.guild_id, query,
												message.author.id, yield));
		SABER_TRY(auto current_track_id,
				  bot.player().current_track_id(*message.guild_id));

		auto description =
			current_track_id != track.id
				? fmt::format(
					  "**✅ Added to queue\n** `{}` - `{}`", query, track.id)
				: fmt::format("**▶️ Started playing\n** `{}`", query);

		auto embed = ekizu::EmbedBuilder().set_description(description).build();

		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .embeds({std::move(embed)})
					  .send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(Play)
COMMAND_FREE(Play)
