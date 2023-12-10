#include <ekizu/embed_builder.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct About : Command {
	explicit About(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "about",
				  DIRNAME,
				  true,
				  true,
				  false,
				  {},
				  {},
				  {},
				  "about",
				  "Info about me.",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  2000,
			  }) {}

	void setup() override {
		bot->logger->info("Setting up About command");

		bot->http.get_user(bot->owner_id)
			.send()
			.map([this](const ekizu::User &owner) {
				bot->logger->info(
					"Fetched owner: {} from the API", owner.username);
				bot->users_cache.put(owner.id, owner);
				bot->http.get_current_user().send().map(
					[this, &owner](const ekizu::User &user) {
						bot->logger->info(
							"Fetched our own user: {} from the API",
							user.username);
						bot->users_cache.put(user.id, user);

						about_embed =
							ekizu::EmbedBuilder{}
								.set_title(fmt::format(
									"🔥About :: Yamato | ID :: {}", user.id))
								.set_description(
									"Yamato is a simple bot that was built for "
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
									{"Info\nOwner",
									 fmt::format("{}", bot->owner_id), true},
								})
								.set_footer(
									{fmt::format("Made by {}", owner.username),
									 owner.display_avatar_url()})
								.build();

						bot->logger->info("About command setup complete");
					});
			});
	}

	void execute(
		const ekizu::Message &message,
		[[maybe_unused]] const std::vector<std::string> &args) override {
		if (!about_embed) { return; }

		(void)bot->http.create_message(message.channel_id)
			.embeds({*about_embed})
			.send();
	}

   private:
	std::optional<ekizu::Embed> about_embed;
};

COMMAND_ALLOC(About)
COMMAND_FREE(About)
