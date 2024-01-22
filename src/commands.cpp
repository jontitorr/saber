#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <filesystem>
#include <saber/saber.hpp>

namespace {
#ifdef _WIN32
constexpr boost::string_view LIBRARY_EXTENSION{".dll"};
#elif __linux__
constexpr boost::string_view LIBRARY_EXTENSION{".so"};
#elif __APPLE__
constexpr boost::string_view LIBRARY_EXTENSION{".dylib"};
#endif

using ekizu::Permissions;

std::string permissions_to_string(Permissions permission) {
	static const boost::unordered_flat_map<Permissions, std::string_view>
		permission_strings{
			{Permissions::CreateInstantInvite, "CREATE_INSTANT_INVITE"},
			{Permissions::KickMembers, "KICK_MEMBERS"},
			{Permissions::BanMembers, "BAN_MEMBERS"},
			{Permissions::Administrator, "ADMINISTRATOR"},
			{Permissions::ManageChannels, "MANAGE_CHANNELS"},
			{Permissions::ManageGuild, "MANAGE_GUILD"},
			{Permissions::AddReactions, "ADD_REACTIONS"},
			{Permissions::ViewAuditLog, "VIEW_AUDIT_LOG"},
			{Permissions::PrioritySpeaker, "PRIORITY_SPEAKER"},
			{Permissions::Stream, "STREAM"},
			{Permissions::ViewChannel, "VIEW_CHANNEL"},
			{Permissions::SendMessages, "SEND_MESSAGES"},
			{Permissions::SendTTSMessages, "SEND_TTS_MESSAGES"},
			{Permissions::ManageMessages, "MANAGE_MESSAGES"},
			{Permissions::EmbedLinks, "EMBED_LINKS"},
			{Permissions::AttachFiles, "ATTACH_FILES"},
			{Permissions::ReadMessageHistory, "READ_MESSAGE_HISTORY"},
			{Permissions::MentionEveryone, "MENTION_EVERYONE"},
			{Permissions::UseExternalEmojis, "USE_EXTERNAL_EMOJIS"},
			{Permissions::ViewGuildInsights, "VIEW_GUILD_INSIGHTS"},
			{Permissions::Connect, "CONNECT"},
			{Permissions::Speak, "SPEAK"},
			{Permissions::MuteMembers, "MUTE_MEMBERS"},
			{Permissions::DeafenMembers, "DEAFEN_MEMBERS"},
			{Permissions::MoveMembers, "MOVE_MEMBERS"},
			{Permissions::UseVAD, "USE_VAD"},
			{Permissions::ChangeNickname, "CHANGE_NICKNAME"},
			{Permissions::ManageNicknames, "MANAGE_NICKNAMES"},
			{Permissions::ManageRoles, "MANAGE_ROLES"},
			{Permissions::ManageWebhooks, "MANAGE_WEBHOOKS"},
			{Permissions::ManageGuildExpressions, "MANAGE_GUILD_EXPRESSIONS"},
			{Permissions::UseApplicationCommands, "USE_APPLICATION_COMMANDS"},
			{Permissions::RequestToSpeak, "REQUEST_TO_SPEAK"},
			{Permissions::ManageEvents, "MANAGE_EVENTS"},
			{Permissions::ManageThreads, "MANAGE_THREADS"},
			{Permissions::CreatePublicThreads, "CREATE_PUBLIC_THREADS"},
			{Permissions::CreatePrivateThreads, "CREATE_PRIVATE_THREADS"},
			{Permissions::UseExternalStickers, "USE_EXTERNAL_STICKERS"},
			{Permissions::SendMessagesInThreads, "SEND_MESSAGES_IN_THREADS"},
			{Permissions::UseEmbeddedActivities, "USE_EMBEDDED_ACTIVITIES"},
			{Permissions::ModerateMembers, "MODERATE_MEMBERS"},
			{Permissions::ViewCreatorMonetizationAnalytics,
			 "VIEW_CREATOR_MONETIZATION_ANALYTICS"},
			{Permissions::UseSoundboard, "USE_SOUNDBOARD"},
			{Permissions::UseExternalSounds, "USE_EXTERNAL_SOUNDS"},
			{Permissions::SendVoiceMessages, "SEND_VOICE_MESSAGES"}};

	auto it = permission_strings.find(permission);
	return (it != permission_strings.end()) ? std::string{it->second}
											: "UNKNOWN";
}
}  // namespace

namespace saber {
CommandLoader::CommandLoader(Saber &parent) : m_parent{parent} {}

void CommandLoader::load(std::string_view path,
						 const boost::asio::yield_context &yield) {
	auto library = Library::create(path);

	if (!library) {
		m_parent.log<ekizu::LogLevel::Error>(
			"Failed to load command {}: {}", path, library.error().message());
		return;
	}

	auto init_command =
		library.value().get<Command *(*)(Saber &)>("init_command");
	auto free_command =
		library.value().get<void (*)(Command *)>("free_command");

	if (!init_command || !free_command) { return; }

	std::scoped_lock lk{m_mtx};
	auto *command_ptr = (init_command.value())(m_parent);

	if (command_ptr == nullptr) { return; }

	if (command_ptr->options.init) {
		auto res = command_ptr->setup(yield);

		if (!res) {
			m_parent.log<ekizu::LogLevel::Error>(
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

	m_parent.log<ekizu::LogLevel::Info>("Loaded command {}", command_name);
}

void CommandLoader::load_all(const boost::asio::yield_context &yield) {
	namespace fs = std::filesystem;

	for (const auto &file : fs::directory_iterator(".")) {
		const auto filename = file.path().filename().string();
		boost::string_view filename_sv{filename};

		if (filename_sv.starts_with("cmd_") &&
			filename_sv.ends_with(LIBRARY_EXTENSION)) {
			load(fs::absolute(file.path()).lexically_normal().string(), yield);
		}
	}

	m_parent.log<ekizu::LogLevel::Info>("Loaded {} commands", commands.size());
}

Result<> CommandLoader::process_commands(
	const ekizu::Message &message, const boost::asio::yield_context &yield) {
	if (message.author.bot) { return outcome::success(); }

	auto content = message.content.substr(m_parent.prefix().size());
	std::vector<std::string> args;
	boost::algorithm::split(
		args, boost::algorithm::trim_copy(content), boost::is_any_of(" "));

	if (args.empty()) { return outcome::success(); }

	auto command_name = std::move(args.front());
	args.erase(args.begin());

	std::transform(
		command_name.begin(), command_name.end(), command_name.begin(),
		[](uint8_t c) { return static_cast<char>(std::tolower(c)); });

	std::unique_lock lk{m_mtx};

	if (!command_map.contains(command_name)) { return outcome::success(); }

	auto cmd = command_map.at(command_name);

	if (cmd->options.guild_only) {
		if (!message.guild_id) {
			SABER_TRY(m_parent.http()
						  .create_message(message.channel_id)
						  .content("This command can only be used in guilds.")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		auto bot_permissions = m_parent.get_guild_permissions(
			*message.guild_id, m_parent.bot_id());

		if (!bot_permissions) {
			SABER_TRY(m_parent.http()
						  .create_message(message.channel_id)
						  .content("Failed to get bot permissions.")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		// Check permissions.
		for (const auto &perm : cmd->options.bot_permissions) {
			if ((bot_permissions.value() & perm) != perm) {
				SABER_TRY(
					m_parent.http()
						.create_message(message.channel_id)
						.content(fmt::format(
							"Needed permissions: {}",
							boost::algorithm::trim_copy(boost::algorithm::join(
								cmd->options.bot_permissions |
									boost::adaptors::transformed(
										permissions_to_string),
								", "))))
						.reply(message.id)
						.send(yield));
				return outcome::success();
			}
		}

		// Check member permissions
		auto member_permissions = m_parent.get_guild_permissions(
			*message.guild_id, message.author.id);

		if (!member_permissions) {
			SABER_TRY(m_parent.http()
						  .create_message(message.channel_id)
						  .content("Failed to get member permissions.")
						  .reply(message.id)
						  .send(yield));
			return outcome::success();
		}

		for (const auto &perm : cmd->options.member_permissions) {
			if ((member_permissions.value() & perm) != perm) {
				SABER_TRY(
					m_parent.http()
						.create_message(message.channel_id)
						.content(fmt::format(
							"You need the following permissions: {}",
							boost::algorithm::trim_copy(boost::algorithm::join(
								cmd->options.member_permissions |
									boost::adaptors::transformed(
										permissions_to_string),
								", "))))
						.reply(message.id)
						.send(yield));
				return outcome::success();
			}
		}
	}

	if (m_parent.command_cooldowns().contains(message.author.id)) {
		auto cooldown = m_parent.command_cooldowns().at(message.author.id);

		if (cooldown.contains(command_name)) {
			auto expiry = cooldown.at(command_name);
			auto delta = std::chrono::floor<std::chrono::seconds>(
							 expiry - std::chrono::steady_clock::now())
							 .count();

			if (delta > 0) {
				SABER_TRY(
					m_parent.http()
						.create_message(message.channel_id)
						.content(fmt::format(
							"Please wait {} more seconds before using this "
							"command.",
							delta))
						.reply(message.id)
						.send(yield));
				return outcome::success();
			}
		}
	}

	m_parent.command_cooldowns()[message.author.id][command_name] =
		std::chrono::steady_clock::now() + cmd->options.cooldown;

	// NOTE: I'm seeing a case in which the commands will need the lock so it
	// should be unlocked here. i.e. an unload command or something.
	lk.unlock();

	return cmd->execute(message, args, yield);
}

void CommandLoader::unload(const std::string &name) {
	std::scoped_lock lk{m_mtx};

	if (!commands.contains(name)) { return; }

	const auto command = std::move(command_map.at(name));

	for (const auto &alias : command->options.aliases) {
		alias_map.erase(alias);
	}

	commands.erase(name);
	command_map.erase(name);
	slash_commands.erase(name);
	user_commands.erase(name);
	m_parent.log<ekizu::LogLevel::Info>("Unloaded command {}", name);
}

void CommandLoader::get_commands(
	ekizu::FunctionView<void(const boost::unordered_flat_map<
							 std::string, std::shared_ptr<Command> > &)>
		cb) const {
	std::scoped_lock lk{m_mtx};

	cb(command_map);
}
}  // namespace saber