#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <ekizu/embed_builder.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>
#include <unordered_set>

using namespace saber;

struct Help : Command {
	explicit Help(Saber &creator)
		: Command(creator,
				  CommandOptionsBuilder()
					  .name("help")
					  .category(DIRNAME)
					  .enabled(true)
					  .init(true)
					  .usage("help")
					  .examples({"help", "help ping"})
					  .bot_permissions({ekizu::Permissions::SendMessages,
										ekizu::Permissions::EmbedLinks})
					  .cooldown(std::chrono::seconds(2))
					  .build()) {}

	Result<> execute(const ekizu::Message &message,
					 const std::vector<std::string> &args,
					 const boost::asio::yield_context &yield) override {
		if (args.empty()) { return get_help(message, yield); }

		return get_command_help(message, args[0], yield);
	}

	Result<> get_help(const ekizu::Message &message,
					  const boost::asio::yield_context &yield) {
		// print all command names.
		std::string names;

		bot.commands().get_commands(
			[&](const boost::unordered_flat_map<
				std::string, std::shared_ptr<Command>> &commands) {
				for (const auto &command : commands) {
					names += command.first + "\n";
				}
			});

		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .content("Available commands:\n" + names)
					  .reply(message.id)
					  .send(yield));

		return ekizu::outcome::success();
	}

	Result<> get_command_help(const ekizu::Message &message,
							  const std::string &command,
							  const boost::asio::yield_context &yield) {
		bot.commands().get_commands(
			[&, this](const boost::unordered_flat_map<
					  std::string, std::shared_ptr<Command>> &commands) {
				if (!commands.contains(command)) {
					return (void)bot.http()
						.create_message(message.channel_id)
						.content("Command not found.")
						.send(yield);
				}

				const auto &cmd = commands.at(command);
				auto builder =
					ekizu::EmbedBuilder{}.set_title(cmd->options.name);

				if (!cmd->options.description.empty()) {
					builder.set_description(cmd->options.description);
				}

				if (!cmd->options.examples.empty()) {
					builder.add_field(
						{"❯ Examples",
						 boost::algorithm::join(
							 cmd->options.examples |
								 boost::adaptors::transformed(
									 [this](const auto &example) {
										 return fmt::format(
											 "`{}{}`\n", bot.prefix(), example);
									 }),
							 "")});
				}

				if (!cmd->options.usage.empty()) {
					builder.add_field(
						{"❯ Usage", fmt::format("`{}{}`", bot.prefix(),
												cmd->options.usage)});
				}

				if (!cmd->options.aliases.empty()) {
					builder.add_field(
						{"❯ Aliases",
						 boost::algorithm::join(
							 cmd->options.aliases |
								 boost::adaptors::transformed(
									 [](const auto &alias) {
										 return fmt::format("`{}`", alias);
									 }),
							 " | ")});
				}

				builder.add_field(
					{"❯ Cooldown",
					 fmt::format(
						 "{}ms",
						 std::chrono::duration_cast<std::chrono::milliseconds>(
							 cmd->options.cooldown)
							 .count())});
				builder.add_field(
					{"❯ Legend", fmt::format("`<> required, [] optional`")});
				builder.set_footer({fmt::format(
					"Prefix: {}",
					bot.prefix().empty() ? "None" : bot.prefix())});

				(void)bot.http()
					.create_message(message.channel_id)
					.embeds({builder.build()})
					.send(yield);
			});

		return outcome::success();
	}
};

COMMAND_ALLOC(Help)
COMMAND_FREE(Help)
