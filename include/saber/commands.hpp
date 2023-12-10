#ifndef SABER_COMMANDS_HPP
#define SABER_COMMANDS_HPP

#include <ekizu/lru_cache.hpp>
#include <ekizu/message.hpp>
#include <ekizu/permissions.hpp>
#include <filesystem>
#include <mutex>
#include <optional>
#include <saber/library.hpp>
#include <unordered_map>
#include <vector>

namespace saber {
struct Command;
struct Saber;

struct CommandLoader {
	explicit CommandLoader(Saber *parent);
	CommandLoader(const CommandLoader &) = delete;
	CommandLoader &operator=(const CommandLoader &) = delete;
	CommandLoader(CommandLoader &&) = delete;
	CommandLoader &operator=(CommandLoader &&) = delete;
	~CommandLoader() = default;

	void load(std::string_view path);
	void load_all();
	void process_commands(const ekizu::Message &message);
	void unload(const std::string &name);
	void get_commands(
		ekizu::FunctionView<void(const std::unordered_map<
								 std::string, std::shared_ptr<Command> > &)>)
		const;

   private:
	mutable std::mutex m_mtx;
	Saber *m_parent{};
	std::unordered_map<std::string, Library> commands;
	std::unordered_map<std::string, std::shared_ptr<Command> > command_map;
	std::unordered_map<std::string, std::shared_ptr<Command> > alias_map;
	std::unordered_map<std::string, std::shared_ptr<Command> > slash_commands;
	std::unordered_map<std::string, std::shared_ptr<Command> > user_commands;
	std::unordered_map<
		std::string,
		std::unordered_map<std::string, std::chrono::steady_clock::time_point> >
		cooldowns;
};

struct CommandOptions {
	std::string name;
	std::string dirname;
	bool enabled{true};
	bool init{};
	bool guild_only{};
	bool slash{};
	bool user{};
	// std::optional<std::vector<ekizu::ApplicationCommandOption> > options;
	std::vector<std::string> aliases;
	std::string usage;
	std::string description;
	std::vector<ekizu::Permissions> member_permissions;
	std::vector<ekizu::Permissions> bot_permissions;
	bool nsfw{};
	bool owner_only{};
	uint32_t cooldown{3000};
	std::vector<std::string> examples{};
	std::string subcommands{};
	bool activity{};
	bool voice_only{};
	std::string category{};
};

struct Command {
	Command(Saber *instigator, CommandOptions options_)
		: bot{instigator}, options{std::move(options_)} {
		// Determine category based on the name of the folder the command is in.
		options.category = !options.dirname.empty() ? options.dirname : "Other";

		// If the command is a slash command, make sure to make the appropriate
		// JSON data.
		if (options.slash) {
			// slash_data.name = options.name;

			// if (!options.description.empty()) {
			// 	slash_data.description = options.description;
			// }

			// if (options.options.has_value() && !options.options->empty()) {
			// 	slash_data.options = options.options;
			// }
		}
	};

	Command(const Command &) = delete;
	Command &operator=(const Command &) = delete;
	Command(Command &&) = delete;
	Command &operator=(Command &&) = delete;
	virtual ~Command() = default;

	/// Method reserved for any initial setup for the command.
	virtual void setup() = 0;
	/// Method reserved for message commands' execution.
	virtual void execute(const ekizu::Message &message,
						 const std::vector<std::string> &args) = 0;

	Saber *bot{};
	CommandOptions options;
};

#if defined(__linux__) || defined(__APPLE__)
#define COMMAND_ALLOC(name)                                       \
	extern "C" {                                                  \
	__attribute__((visibility("default"))) Command *init_command( \
		Saber *parent) {                                          \
		return new name(parent);                                  \
	}                                                             \
	}
#elif defined(_WIN32)
#define COMMAND_ALLOC(name)                                      \
	struct name;                                                 \
	extern "C" {                                                 \
	__declspec(dllexport) Command *init_command(Saber *parent) { \
		return new name(parent);                                 \
	}                                                            \
	}
#endif

#if defined(__linux__) || defined(__APPLE__)
#define COMMAND_FREE(name)                                    \
	extern "C" {                                              \
	__attribute__((visibility("default"))) void free_command( \
		Command *command) {                                   \
		delete command;                                       \
	}                                                         \
	}
#elif defined(_WIN32)
#define COMMAND_FREE(name)                                      \
	struct name;                                                \
	extern "C" {                                                \
	__declspec(dllexport) void free_command(Command *command) { \
		delete command;                                         \
	}                                                           \
	}
#endif

// I miss javascript
#define DIRNAME \
	std::filesystem::path(__FILE__).parent_path().filename().string()
}  // namespace saber

#endif	// SABER_COMMANDS_HPP
