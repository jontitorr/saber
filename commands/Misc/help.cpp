#include <ekizu/embed_builder.hpp>
#include <numeric>
#include <saber/saber.hpp>
#include <saber/util.hpp>

using namespace saber;

struct Help : Command {
	explicit Help(Saber *creator)
		: Command(
			  creator,
			  CommandOptions{
				  "help",
				  DIRNAME,
				  true,
				  {},
				  {},
				  {},
				  {},
				  {},
				  "help",
				  "",
				  {ekizu::Permissions::SendMessages,
				   ekizu::Permissions::EmbedLinks},
				  {},
				  {},
				  {},
				  3000}) {}

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

		bot->commands().get_commands(
			[&](const std::unordered_map<std::string, std::shared_ptr<Command>>
					&commands) {
				for (const auto &command : commands) {
					names += command.first + "\n";
				}
			});

		(void)bot->http()
			.create_message(message.channel_id)
			.content("Available commands:\n" + names)
			.send(yield);

		return ekizu::outcome::success();
	}

	Result<> get_command_help(const ekizu::Message &message,
							  const std::string &command,
							  const boost::asio::yield_context &yield) {
		bot->commands().get_commands(
			[&, this](
				const std::unordered_map<std::string, std::shared_ptr<Command>>
					&commands) {
				if (commands.find(command) == commands.end()) {
					return (void)bot->http()
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
					std::string examples{};
					examples.reserve((bot->prefix().length() + 3) *
									 cmd->options.examples.size());
					for (const auto &example : cmd->options.examples) {
						examples +=
							fmt::format("`{}{}`\n", bot->prefix(), example);
					}

					builder.add_field({"❯ Examples", examples});
				}

				if (!cmd->options.usage.empty()) {
					builder.add_field(
						{"❯ Usage", fmt::format("`{}{}`", bot->prefix(),
												cmd->options.usage)});
				}

				if (const auto &aliases = cmd->options.aliases;
					!aliases.empty()) {
					const auto aliases_str = std::accumulate(
						std::next(aliases.begin()), aliases.end(),
						fmt::format("`{}`", aliases[0]),
						[](const auto &a, const auto &b) {
							return fmt::format("{} | `{}`", a, b);
						});

					builder.add_field({"❯ Aliases", aliases_str});
				}

				builder.add_field(
					{"❯ Cooldown", fmt::format("{}ms", cmd->options.cooldown)});
				builder.add_field(
					{"❯ Legend", fmt::format("`<> required, [] optional`")});
				builder.set_footer({fmt::format(
					"Prefix: {}", bot->prefix().empty()
									  ? "None"
									  : fmt::format("`{}`", bot->prefix()))});

				(void)bot->http()
					.create_message(message.channel_id)
					.embeds({builder.build()})
					.send(yield);
			});

		return outcome::success();
	}
};

COMMAND_ALLOC(Help)
COMMAND_FREE(Help)
