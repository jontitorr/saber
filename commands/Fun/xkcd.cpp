#include <nlohmann/json.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct XKCD : Command {
	explicit XKCD(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "xkcd",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "xkcd",
				  "",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  0,
			  }) {}

	void setup() override {}

	void execute(const ekizu::Message &message,
				 const std::vector<std::string> &args) override {
		fetchXKCD(message, args);
	}

   private:
	std::string api_url{"https://xkcd.com{}info.0.json"};

	void fetchXKCD(const ekizu::Message &message,
				   [[maybe_unused]] const std::vector<std::string> &args) {
		const auto res =
			net::http::get(api_url.replace(api_url.find("{}"), 2, "/"));

		if (!res || res->status_code != net::HttpStatus::Ok) {
			bot->logger->error("Error while fetching XKCD data");
			(void)bot->http.create_message(message.channel_id)
				.content("There was an error. Please try again.")
				.send();
			return;
		}

		const auto json = nlohmann::json::parse(res->body, nullptr, false);

		if (json.is_discarded() || !json.is_object()) {
			bot->logger->error("Error parsing XKCD data");
			(void)bot->http.create_message(message.channel_id)
				.content("There was an error. Please try again.")
				.send();
			return;
		}

		const auto comic_url = fmt::format("https://xkcd.com/{}", json["num"]);
		const auto msg = fmt::format(
			"**{}**\n{}\nAlt Text:```{}```XKCD Link: <{}>",
			json["safe_title"].get<std::string>(),
			json["img"].get<std::string>(), json["alt"].get<std::string>(),
			comic_url);

		(void)bot->http.create_message(message.channel_id).content(msg).send();
	}
};

COMMAND_ALLOC(XKCD)
COMMAND_FREE(XKCD)
