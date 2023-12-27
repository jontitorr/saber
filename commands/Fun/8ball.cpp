#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Eightball : Command {
	explicit Eightball(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "8ball",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {"eight-ball", "eightball"},
				  "8ball <question>",
				  "Asks the Magic 8-Ball for some psychic wisdom.",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  3000,
			  }) {}

	void execute(const ekizu::Message &message,
				 const std::vector<std::string> &args,
				 const boost::asio::yield_context &yield) override {
		if (args.empty()) {
			(void)bot->http.create_message(message.channel_id)
				.content("You need to ask a question!")
				.send(yield);
			return;
		}

		// Otherwise, pick a random response and send it
		/// Use random engine.
		if (auto res = bot->http.create_message(message.channel_id)
						   .content(responses[util::get_random_number<size_t>(
							   0, responses.size() - 1)])
						   .send(yield);
			!res) {
			bot->logger->error(
				"Error sending response: {}", res.error().message());
		}
	}

   private:
	std::vector<std::string> responses{
		"It is certain.",
		"It is decidedly so.",
		"Without a doubt.",
		"Yes - definitely.",
		"You may rely on it.",
		"As I see it, yes.",
		"Most likely.",
		"Outlook good.",
		"Yes.",
		"Signs point to yes.",
		"Reply hazy, try again.",
		"Ask again later.",
		"Better not tell you now.",
		"Cannot predict now.",
		"Concentrate and ask again.",
		"Don't count on it.",
		"My reply is no.",
		"My sources say no.",
		"Outlook not so good.",
		"Very doubtful.",
	};
};

COMMAND_ALLOC(Eightball)
COMMAND_FREE(Eightball)
