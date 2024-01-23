#include <ekizu/embed_builder.hpp>
#include <saber/saber.hpp>

using namespace saber;

struct About : Command {
	explicit About(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("about")
					  .category(DIRNAME)
					  .enabled(true)
					  .init(true)
					  .usage("about")
					  .description("Info about me.")
					  .bot_permissions(ekizu::Permissions::SendMessages |
									   ekizu::Permissions::EmbedLinks)
					  .cooldown(std::chrono::seconds(2))
					  .build()) {}

	Result<> setup(const boost::asio::yield_context &yield) override {
		bot.log<ekizu::LogLevel::Info>("Setting up About command");

		SABER_TRY(auto owner, bot.http().get_user(bot.owner_id()).send(yield));

		bot.log<ekizu::LogLevel::Info>(
			"Fetched owner: {} from the API", owner.username);
		bot.users().put(owner.id, owner);

		SABER_TRY(auto user, bot.http().get_current_user().send(yield));

		bot.log<ekizu::LogLevel::Info>(
			"Fetched our own user: {} from the API", user.username);
		bot.users().put(user.id, user);

		about_embed =
			ekizu::EmbedBuilder{}
				.set_title(fmt::format("🔥About :: Saber | ID :: {}", user.id))
				.set_description(
					"Saber is a simple bot that was built for "
					"my "
					"personal Discord server. From providing "
					"7DS "
					"game "
					"updates to moderating your own server, "
					"this "
					"is "
					"the bot for you! I am glad you enjoy my "
					"bot "
					"and "
					"hope you enjoy your stay 💖.")
				.set_thumbnail({user.display_avatar_url()})
				.add_fields({
					{"Info\nOwner", fmt::format("<@{}>", bot.owner_id()), true},
				})
				.set_footer({fmt::format("Made by {}", owner.username),
							 owner.display_avatar_url()})
				.build();

		bot.log<ekizu::LogLevel::Info>("About command setup complete");
		return outcome::success();
	}

	Result<> execute(const ekizu::Message &message,
					 [[maybe_unused]] const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (!about_embed) {
			return boost::system::errc::operation_not_permitted;
		}

		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .embeds({*about_embed})
					  .send(yield));

		return outcome::success();
	}

   private:
	std::optional<ekizu::Embed> about_embed;
};

COMMAND_ALLOC(About)
COMMAND_FREE(About)
