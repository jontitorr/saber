#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct CSS : Command {
	explicit CSS(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("css")
					  .category(DIRNAME)
					  .enabled(true)
					  .usage("css")
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (!args.empty()) {
			return boost::system::errc::operation_not_permitted;
		}

		BOOST_OUTCOME_TRY(
			bot.http()
				.create_message(message.channel_id)
				.content("https://media2.giphy.com/media/yYSSBtDgbbRzq/"
						 "giphy.gif?cid="
						 "ecf05e47ckbtzm84p629vw655dbua1qzaiw8tl46ejp4f0xj&ep="
						 "v1_gifs_"
						 "search&rid=giphy.gif&ct=g")
				.send(yield));

		return outcome::success();
	}
};

COMMAND_ALLOC(CSS)
COMMAND_FREE(CSS)
