#include <ekizu/embed_builder.hpp>
#include <nlohmann/json.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Meme : Command {
	explicit Meme(Saber *creator)
		: Command(creator,
			  CommandOptions{
				  "meme",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "meme",
				  "Displays a random meme from the `memes`, `dankmemes`, or `me_irl` subreddits.",
				  { ekizu::Permissions::SendMessages,
				    ekizu::Permissions::EmbedLinks },
				  {},
				  {},
				  {},
				  0,
			  })
	{
	}

	void setup() override
	{
	}

	void
	execute(const ekizu::Message &message,
		[[maybe_unused]] const std::vector<std::string> &args) override
	{
		fetch_meme(message);
	}

	void fetch_meme(const ekizu::Message &message)
	{
		auto res = net::http::get("https://meme-api.com/gimme");

		if (!res) {
			return;
		}

		const auto json =
			nlohmann::json::parse(res->body, nullptr, false);

		if (json.is_discarded() || !json.is_object()) {
			return;
		}

		auto embed =
			ekizu::EmbedBuilder{}
				.set_title(json["title"])
				.set_description(fmt::format(
					"By: **{}** | {}",
					json["author"].get<std::string>(),
					json["postLink"].get<std::string>()))
				.set_image({
					json["url"].get<std::string>(),
				})
				.build();

		(void)bot->http.create_message(message.channel_id)
			.embeds({ std::move(embed) })
			.send();
	}
};

COMMAND_ALLOC(Meme)
COMMAND_FREE(Meme)
