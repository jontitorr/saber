#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Eightball : Command {
	explicit Eightball(Saber &creator)
		: Command(
			  creator,
			  CommandOptionsBuilder()
				  .name("8ball")
				  .dirname(DIRNAME)
				  .enabled(true)
				  .aliases({"eight-ball", "eightball"})
				  .usage("8ball <question>")
				  .description("Asks the Magic 8-Ball for some psychic wisdom.")
				  .bot_permissions({ekizu::Permissions::SendMessages,
									ekizu::Permissions::EmbedLinks})
				  .cooldown(std::chrono::seconds(3))
				  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (args.empty()) {
			BOOST_OUTCOME_TRY(bot.http()
								  .create_message(message.channel_id)
								  .content("You need to ask a question!")
								  .send(yield));
			return outcome::success();
		}

		// Otherwise, pick a random response and send it
		/// Use random engine.
		BOOST_OUTCOME_TRY(
			bot.http()
				.create_message(message.channel_id)
				.content(responses[util::get_random_number<size_t>(
					0, responses.size() - 1)])
				.send(yield));

		return outcome::success();
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
