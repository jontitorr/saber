#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Beep : Command {
	explicit Beep(Saber *creator)
		: Command(creator, CommandOptions{
					   "beep",
					   DIRNAME,
					   true,
					   {},
					   {},
					   {},
					   {},
					   {},
					   "beep",
					   "Replies with boop.",
					   { ekizu::Permissions::SendMessages,
					     ekizu::Permissions::EmbedLinks },
					   {},
					   {},
					   {},
					   3000,
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
		(void)bot->http.create_message(message.channel_id)
			.content("Boop!")
			.send();
	}
};

COMMAND_ALLOC(Beep)
COMMAND_FREE(Beep)
