#include <boost/range/adaptors.hpp>
#include <ekizu/embed_builder.hpp>
#include <ekizu/json_util.hpp>
#include <saber/reddit.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Hentai : Command {
	explicit Hentai(Saber *creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("hentai")
					  .dirname(DIRNAME)
					  .enabled(true)
					  .init(true)
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(2000)
					  .build()),
		  m_reddit{RedditOptions{
			  std::getenv("SABER_REDDIT_USERNAME"),
			  std::getenv("SABER_REDDIT_PASSWORD"),
			  std::getenv("SABER_REDDIT_APP_ID"),
			  std::getenv("SABER_REDDIT_APP_SECRET"),
		  }} {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (args.empty()) {
			BOOST_OUTCOME_TRY(bot->http()
								  .create_message(message.channel_id)
								  .content("Please specify a query.")
								  .reply(message.id)
								  .send(yield));

			return outcome::success();
		}

		const auto query = fmt::to_string(fmt::join(args, ""));
		BOOST_OUTCOME_TRY(
			const auto sauce, search_hentai_subreddit(query, yield));
		auto cm =
			bot->http().create_message(message.channel_id).reply(message.id);

		if (sauce.empty()) {
			cm.content("No results found.");
			return outcome::success();
		}

		BOOST_OUTCOME_TRY(cm.content(sauce).send(yield));

		return outcome::success();
	}

   private:
	Result<std::string> search_hentai_subreddit(
		std::string_view query, const boost::asio::yield_context &yield) {
		BOOST_OUTCOME_TRY(
			auto res,
			m_reddit.get()
				.subreddit("hentai")
				.query(query)
				.include_over_18(true)
				.restrict_sr(true)
				.limit(100)
				.count(100 *
					   std::floor(util::get_random_number<double>(0, 1) * 3))
				.time("all")
				.sort("relevance")
				.send(yield));
		BOOST_OUTCOME_TRY(
			const auto data, ekizu::json_util::try_parse(res.body()));

		const auto &posts = data["data"]["children"];

		std::vector<nlohmann::json> filtered_posts;
		using ekizu::json_util::not_null_all;
		std::copy_if(posts.begin(), posts.end(),
					 std::back_inserter(filtered_posts), [](const auto &post) {
						 return not_null_all(post, "data") &&
								not_null_all(post["data"], "post_hint") &&
								post["data"]["post_hint"] == "image";
					 });

		// Return a random post.
		if (filtered_posts.empty()) { return ""; }

		const auto &post = filtered_posts[util::get_random_number<size_t>(
			0, filtered_posts.size() - 1)];

		// Get the post's URL.
		if (!not_null_all(post["data"], "url")) { return ""; }
		return post["data"]["url"];
	}

	Reddit m_reddit;
};

COMMAND_ALLOC(Hentai)
COMMAND_FREE(Hentai)
