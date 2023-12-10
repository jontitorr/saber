#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct CSS : Command {
	explicit CSS(Saber *creator)
		: Command(creator, CommandOptions{
					   "css",
					   DIRNAME,
					   true,
					   {},
					   {},
					   {},
					   {},
					   {},
					   "css",
					   "",
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

	void execute(const ekizu::Message &message,
		     const std::vector<std::string> &args) override
	{
		if (!args.empty()) {
			return;
		}

		(void)bot->http.create_message(message.channel_id)
			.content(
				"https://media2.giphy.com/media/yYSSBtDgbbRzq/giphy.gif?cid=ecf05e47ckbtzm84p629vw655dbua1qzaiw8tl46ejp4f0xj&ep=v1_gifs_search&rid=giphy.gif&ct=g")
			.send();
	}
};

COMMAND_ALLOC(CSS)
COMMAND_FREE(CSS)
