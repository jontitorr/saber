#include <filesystem>
#include <saber/saber.hpp>
#include <saber/util.hpp>

namespace {
#ifdef _WIN32
constexpr std::string_view LIBRARY_EXTENSION{".dll"};
#elif __linux__
constexpr std::string_view LIBRARY_EXTENSION{".so"};
#elif __APPLE__
constexpr std::string_view LIBRARY_EXTENSION{".dylib"};
#endif
}  // namespace

namespace saber {
CommandLoader::CommandLoader(Saber *parent) : m_parent{parent} {}

void CommandLoader::load(std::string_view path,
						 const boost::asio::yield_context &yield) {
	auto library = Library::create(path);

	if (!library) {
		m_parent->log<ekizu::LogLevel::Error>(
			"Failed to load command {}: {}", path, library.error().message());
		return;
	}

	auto init_command =
		library.value().get<Command *(*)(Saber *)>("init_command");
	auto free_command =
		library.value().get<void (*)(Command *)>("free_command");

	if (!init_command || !free_command) { return; }

	std::scoped_lock lk{m_mtx};

	auto *command_ptr = (init_command.value())(m_parent);

	if (command_ptr == nullptr) { return; }

	if (command_ptr->options.init) {
		auto res = command_ptr->setup(yield);

		if (!res) {
			m_parent->log<ekizu::LogLevel::Error>(
				"Failed to setup command {}: {}", command_ptr->options.name,
				res.error().message());
			return;
		}
	}

	const auto &command_name = command_ptr->options.name;
	const std::shared_ptr<Command> loader{
		command_ptr,
		[dealloc = free_command.value()](Command *ptr) { dealloc(ptr); }};

	commands.insert_or_assign(command_name, std::move(library.value()));
	command_map.insert_or_assign(command_name, loader);

	for (const auto &alias : command_ptr->options.aliases) {
		alias_map.insert_or_assign(alias, loader);
	}

	if (command_ptr->options.slash) {
		slash_commands.insert_or_assign(command_name, loader);
	}

	if (command_ptr->options.user) {
		user_commands.insert_or_assign(command_name, loader);
	}

	m_parent->log<ekizu::LogLevel::Info>("Loaded command {}", command_name);
}

void CommandLoader::load_all(const boost::asio::yield_context &yield) {
	namespace fs = std::filesystem;

	for (const auto &file : fs::directory_iterator(".")) {
		const auto filename = file.path().filename().string();

		if (filename.find("cmd_") != std::string::npos &&
			(filename.size() >= LIBRARY_EXTENSION.size() &&
			 filename.compare(
				 filename.size() - LIBRARY_EXTENSION.size(),
				 LIBRARY_EXTENSION.size(), LIBRARY_EXTENSION) == 0)) {
			load(fs::absolute(file.path()).lexically_normal().string(), yield);
		}
	}

	m_parent->log<ekizu::LogLevel::Info>("Loaded {} commands", commands.size());
}

void CommandLoader::process_commands(const ekizu::Message &message,
									 const boost::asio::yield_context &yield) {
	if (message.author.bot) { return; }

	auto content = message.content.substr(m_parent->prefix().size());
	auto args = util::split(util::trim(content), " ");

	if (args.empty()) { return; }

	auto command_name = std::move(args.front());
	args.erase(args.begin());

	std::transform(
		command_name.begin(), command_name.end(), command_name.begin(),
		[](uint8_t c) { return static_cast<char>(std::tolower(c)); });

	std::unique_lock lk{m_mtx};

	if (command_map.find(command_name) == command_map.end()) { return; }

	auto cmd = command_map.at(command_name);

	if (cmd->options.guild_only && !message.guild_id) {
		return (void)m_parent->http()
			.create_message(message.channel_id)
			.content("This command can only be used in guilds.")
			.reply(message.id)
			.send(yield);
	}

	// NOTE: I'm seeing a case in which the commands will need the lock so it
	// should be unlocked here. i.e. an unload command or something.
	lk.unlock();

	if (auto res = cmd->execute(message, args, yield); !res) {
		m_parent->log<ekizu::LogLevel::Error>(
			"Failed to execute command {}: {}", command_name,
			res.error().message());
	}
}

void CommandLoader::unload(const std::string &name) {
	std::scoped_lock lk{m_mtx};

	if (commands.find(name) == commands.end()) { return; }

	const auto command = std::move(command_map.at(name));

	for (const auto &alias : command->options.aliases) {
		alias_map.erase(alias);
	}

	commands.erase(name);
	command_map.erase(name);
	slash_commands.erase(name);
	user_commands.erase(name);
	m_parent->log<ekizu::LogLevel::Info>("Unloaded command {}", name);
}

void CommandLoader::get_commands(
	ekizu::FunctionView<void(
		const std::unordered_map<std::string, std::shared_ptr<Command> > &)>
		cb) const {
	std::scoped_lock lk{m_mtx};

	cb(command_map);
}
}  // namespace saber