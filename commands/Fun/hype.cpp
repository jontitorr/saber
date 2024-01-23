#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Hype : Command {
	explicit Hype(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("hype")
					  .category(DIRNAME)
					  .enabled(true)
					  .aliases({"hypu", "train"})
					  .usage("hype")
					  .description("Hype someone.")
					  .bot_permissions(ekizu::Permissions::SendMessages |
									   ekizu::Permissions::EmbedLinks)
					  .cooldown(std::chrono::seconds(3))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (!args.empty()) { return outcome::success(); }

		const std::string &selectedHype =
			hypu[util::get_random_number<size_t>(0, hypu.size() - 1)];
		std::string msg = ":train2: CHOO CHOO " + selectedHype;

		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .content(msg)
					  .send(yield));

		return outcome::success();
	}

   private:
	std::vector<std::string> hypu{
		"https://cdn.discordapp.com/attachments/102817255661772800/"
		"219514281136357376/tumblr_nr6ndeEpus1u21ng6o1_540.gif",
		"https://cdn.discordapp.com/attachments/102817255661772800/"
		"219518372839161859/tumblr_n1h2afSbCu1ttmhgqo1_500.gif",
		"https://gfycat.com/HairyFloweryBarebirdbat",
		"https://i.imgur.com/PFAQSLA.gif",
		"https://abload.de/img/ezgif-32008219442iq0i.gif",
		"https://i.imgur.com/vOVwq5o.jpg",
		"https://i.imgur.com/Ki12X4j.jpg",
		"https://media.giphy.com/media/b1o4elYH8Tqjm/giphy.gif"};
};

COMMAND_ALLOC(Hype)
COMMAND_FREE(Hype)
