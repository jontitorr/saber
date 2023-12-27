#include <ekizu/embed_builder.hpp>
#include <ekizu/http.hpp>
#include <nlohmann/json.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Meme : Command {
	explicit Meme(Saber *creator)
		: Command(
			  creator,
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
				  "Displays a random meme from the `memes`, `dankmemes`, or "
				  "`me_irl` subreddits.",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  0,
			  }) {}

	void execute(const ekizu::Message &message,
				 [[maybe_unused]] const std::vector<std::string> &args,
				 const boost::asio::yield_context &yield) override {
		fetch_meme(message, yield);
	}

	void fetch_meme(const ekizu::Message &message,
					const boost::asio::yield_context &yield) const {
		auto res = ekizu::net::HttpConnection::get(
			"https://meme-api.com/gimme", yield);

		if (!res) { return; }

		const auto json =
			nlohmann::json::parse(res.value().body(), nullptr, false);

		if (json.is_discarded() || !json.is_object()) { return; }

		auto embed =
			ekizu::EmbedBuilder{}
				.set_title(json["title"])
				.set_description(fmt::format(
					"By: **{}** | {}", json["author"].get<std::string>(),
					json["postLink"].get<std::string>()))
				.set_image({
					json["url"].get<std::string>(),
				})
				.build();

		(void)bot->http.create_message(message.channel_id)
			.embeds({std::move(embed)})
			.send(yield);
	}
};

COMMAND_ALLOC(Meme)
COMMAND_FREE(Meme)
