#include <ekizu/embed_builder.hpp>
#include <saber/saber.hpp>
#include <saber/util.hpp>

// template <typename T, typename Predicate>
// std::vector<T> filter(const std::vector<T> &v, Predicate p)
// {
// 	std::vector<T> result{};
// 	result.reserve(v.size());
// 	std::copy_if(v.begin(), v.end(), std::back_inserter(result), p);
// 	return result;
// }

// // function template equivalent to filter but for unordered_maps. takes an unordered_map and a predicate. returns a new
// // unordered_map with all the elements that satisfy the predicate.
// template <typename T, typename U, typename Predicate, typename... Args>
// std::unordered_map<T, U, Args...>
// filter(const std::unordered_map<T, U, Args...> &v, Predicate p)
// {
// 	std::unordered_map<T, U, Args...> result{};
// 	for (const auto &pair : v) {
// 		if (p(pair)) {
// 			result.emplace(pair);
// 		}
// 	}
// 	return result;
// }

// // function template equivalent to map but for unordered_maps. takes an unordered_map and a function. returns an array
// // of the results of the function.
// template <typename K, typename V, typename Ret, typename... Args>
// std::vector<Ret> map(const std::unordered_map<K, V, Args...> &v,
// 		     std::function<Ret(const std::pair<K, V> &)> f)
// {
// 	std::vector<Ret> result{};
// 	result.reserve(v.size());
// 	for (const auto &pair : v) {
// 		result.emplace_back(f(pair));
// 	}
// 	return result;
// }

// template <typename T, typename U>
// std::vector<U> map(const std::vector<T> &v, std::function<U(const T &)> func)
// {
// 	std::vector<U> result{};
// 	result.reserve(v.size());
// 	for (const auto &elem : v) {
// 		result.emplace_back(func(elem));
// 	}
// 	return result;
// }

// // to_lower function for strings.
// std::string to_lower(std::string_view str)
// {
// 	std::string result{ str };
// 	std::transform(result.begin(), result.end(), result.begin(),
// 		       [](uint8_t c) {
// 			       return static_cast<char>(std::tolower(c));
// 		       });
// 	return result;
// }

using namespace saber;

struct Help : Command {
	struct CategoricCommand {
		std::string category{};
		std::vector<std::string> commands{};
	};

	explicit Help(Saber *creator)
		: Command(creator,
			  CommandOptions{ "help",
					  DIRNAME,
					  true,
					  {},
					  {},
					  {},
					  {},
					  {},
					  "help",
					  "",
					  { ekizu::Permissions::SendMessages,
					    ekizu::Permissions::EmbedLinks },
					  {},
					  {},
					  {},
					  3000 })
	{
	}

	void setup() override
	{
	}

	void execute(const ekizu::Message &message,
		     const std::vector<std::string> &args) override
	{
		// if (args.empty()) {
		// 	return create_help_menu(message);
		// }

		return get_command_help(message, args[0]);
	}

	void get_command_help(const ekizu::Message &message,
			      const std::string &command)
	{
		bot->commands.get_commands([this, &command,
					    &message](const std::unordered_map<
						      std::string,
						      std::shared_ptr<Command> >
							      &commands) {
			if (commands.find(command) == commands.end()) {
				(void)bot->http
					.create_message(message.channel_id)
					.content("Command not found.")
					.send();
				return;
			}

			const auto &cmd = commands.at(command);
			auto builder = ekizu::EmbedBuilder{}.set_title(
				cmd->options.name);

			if (!cmd->options.description.empty()) {
				builder.set_description(
					cmd->options.description);
			}

			if (!cmd->options.examples.empty()) {
				std::string examples{};
				examples.reserve((bot->prefix.length() + 3) *
						 cmd->options.examples.size());
				for (const auto &example :
				     cmd->options.examples) {
					examples += fmt::format("`{}{}`\n",
								bot->prefix,
								example);
				}

				builder.add_field({ "❯ Examples", examples });
			}

			if (!cmd->options.usage.empty()) {
				builder.add_field(
					{ "❯ Usage",
					  fmt::format("`{}{}`", bot->prefix,
						      cmd->options.usage) });
			}

			if (const auto &aliases = cmd->options.aliases;
			    !aliases.empty()) {
				const auto aliases_str = std::accumulate(
					std::next(aliases.begin()),
					aliases.end(),
					fmt::format("`{}`", aliases[0]),
					[](const auto &a, const auto &b) {
						return fmt::format("{} | `{}`",
								   a, b);
					});

				builder.add_field({ "❯ Aliases", aliases_str });
			}

			builder.add_field(
				{ "❯ Cooldown",
				  fmt::format("{}ms", cmd->options.cooldown) });
			builder.add_field(
				{ "❯ Legend",
				  fmt::format("`<> required, [] optional`") });
			builder.set_footer(
				{ fmt::format("Prefix: {}", bot->prefix) });

			(void)bot->http.create_message(message.channel_id)
				.embeds({ builder.build() })
				.send();
		});
	}

	// void create_help_menu(const ekizu::Message &message) const
	// {
	// 	if (!message.channel()) {
	// 		return;
	// 	}
	// 	const auto &commands = bot->command_loader.get_commands();
	// 	// Create a set of all the categories.
	// 	std::vector<std::string> categories{};
	// 	{
	// 		std::unordered_set<std::string> categories_set{};
	// 		std::transform(
	// 			commands.begin(), commands.end(),
	// 			std::inserter(categories_set,
	// 				      categories_set.end()),
	// 			[](const auto &pair) {
	// 				return pair.second->options.category;
	// 			});
	// 		std::copy(categories_set.begin(), categories_set.end(),
	// 			  std::back_inserter(categories));
	// 	}

	// 	// Emojis for the categories.
	// 	static const std::unordered_map<std::string, std::string> emojis{
	// 		{ "music", "🎵" },  { "fun", "🎉" },
	// 		{ "hentai", "🔞" }, { "moderation", "🔨" },
	// 		{ "owner", "👑" },  { "templates", "📷" },
	// 		{ "steam", "🎮" },  { "guild", "🏠" },
	// 		{ "misc", "❓" },   { "activity", "👷" },
	// 	};

	// 	const auto formatted_categories = map(
	// 		categories,
	// 		std::function([&commands](const std::string &category) {
	// 			const auto filtered_cmds = filter(
	// 				commands,
	// 				[&category](const auto &pair) {
	// 					return pair.second->options
	// 						       .category ==
	// 					       category;
	// 				});

	// 			const auto cmds = map(
	// 				filtered_cmds,
	// 				std::function(
	// 					[](const std::pair<
	// 						std::string,
	// 						std::shared_ptr<Command> >
	// 						   &pair) {
	// 						return pair.first;
	// 					}));

	// 			return CategoricCommand{ category, cmds };
	// 		}));

	// 	const auto embed = ekizu::Embed().set_description(
	// 		"Please choose a category.");

	// 	const auto components = [&formatted_categories](bool disabled) {
	// 		// Create a new MessageComponent, being an ActionRow, who is disabled if disabled is true.
	// 		return ekizu::ActionRow().add_component(
	// 			ekizu::SelectMenu()
	// 				.set_custom_id("helpMenu")
	// 				.set_placeholder(
	// 					"Please select a category.")
	// 				.set_disabled(disabled)
	// 				.add_options(map(
	// 					formatted_categories,
	// 					std::function([](const CategoricCommand
	// 								 &cmd) {
	// 						return ekizu::SelectOptions{
	// 							.label =
	// 								cmd.category,
	// 							.value = to_lower(
	// 								cmd.category),
	// 							.description = fmt::format(
	// 								"Commands from {} category.",
	// 								cmd.category),
	// 							.emoji =
	// 								ekizu::PartialEmoji{
	// 									{},
	// 									emojis.at(to_lower(
	// 										cmd.category)) },
	// 						};
	// 					}))));
	// 	};

	// 	// const auto& author = message.author();

	// 	const auto initial_message = message.reply({
	// 		.embeds = { { embed } },
	// 		.components = { { components(false) } },
	// 	});

	// 	auto &channel = message.channel();
	// 	auto &collector =
	// 		channel.add_message_component_listener("helpMenu");

	// 	collector.on_add.add_listener([this,
	// 				       fmt_categories = std::move(
	// 					       formatted_categories)](
	// 					      ekizu::Interaction
	// 						      &interaction) {
	// 		const auto category = interaction.data.values[0];
	// 		const auto formatted_category = *std::find_if(
	// 			fmt_categories.begin(), fmt_categories.end(),
	// 			[&category](const auto &cmd) {
	// 				return to_lower(cmd.category) ==
	// 				       category;
	// 			});
	// 		const auto category_embed =
	// 			ekizu::Embed()
	// 				.set_title(fmt::format("{} commands.",
	// 						       category))
	// 				.set_description(fmt::format(
	// 					"Here are the commands you can use."))
	// 				.add_fields(map(
	// 					formatted_category.commands,
	// 					std::function([this](const std::string
	// 								     &cmd) {
	// 						return ekizu::EmbedField{
	// 							.name = cmd,
	// 							.value = fmt::format(
	// 								"`{}{}`",
	// 								bot->prefix,
	// 								cmd),
	// 						};
	// 					})));

	// 		interaction.update(
	// 			{ .embeds = { { category_embed } } });
	// 	});

	// 	collector.on_remove.add_listener([initial_message, components] {
	// 		initial_message->edit(
	// 			{ .components = { { components(true) } } });
	// 	});
	// }
};

COMMAND_ALLOC(Help)
COMMAND_FREE(Help)
