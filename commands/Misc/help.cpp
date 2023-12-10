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

	void setup() override {}

	void execute(const ekizu::Message &message,
				 const std::vector<std::string> &args) override {
		// TODO: Implement help menu when no args are given.

		return get_command_help(message, args[0]);
	}

	void get_command_help(const ekizu::Message &message,
						  const std::string &command) {
		bot->commands.get_commands(
			[this, &command, &message](
				const std::unordered_map<std::string, std::shared_ptr<Command> >
					&commands) {
				if (commands.find(command) == commands.end()) {
					(void)bot->http.create_message(message.channel_id)
						.content("Command not found.")
						.send();
					return;
				}

				const auto &cmd = commands.at(command);
				auto builder =
					ekizu::EmbedBuilder{}.set_title(cmd->options.name);

				if (!cmd->options.description.empty()) {
					builder.set_description(cmd->options.description);
				}

				if (!cmd->options.examples.empty()) {
					std::string examples{};
					examples.reserve((bot->prefix.length() + 3) *
									 cmd->options.examples.size());
					for (const auto &example : cmd->options.examples) {
						examples +=
							fmt::format("`{}{}`\n", bot->prefix, example);
					}

					builder.add_field({"❯ Examples", examples});
				}

				if (!cmd->options.usage.empty()) {
					builder.add_field(
						{"❯ Usage", fmt::format("`{}{}`", bot->prefix,
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
				builder.set_footer({fmt::format("Prefix: `{}`", bot->prefix)});

				(void)bot->http.create_message(message.channel_id)
					.embeds({builder.build()})
					.send();
			});
	}
};

COMMAND_ALLOC(Help)
COMMAND_FREE(Help)
