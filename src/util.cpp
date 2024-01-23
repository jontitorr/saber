#include <boost/algorithm/string/join.hpp>
#include <saber/util.hpp>

namespace {
using ekizu::Permissions;

const boost::unordered_flat_map<Permissions, std::string_view>
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
}  // namespace

namespace saber::util {
Result<> ensure_permissions(
	Saber& bot, const ekizu::Message& message, ekizu::Snowflake user_id,
	ekizu::Permissions required, const boost::asio::yield_context& yield) {
	auto user_perms = bot.get_guild_permissions(*message.guild_id, user_id);

	if (!user_perms) {
		SABER_TRY(bot.http()
					  .create_message(message.channel_id)
					  .content("Something went wrong. Please try again later.")
					  .reply(message.id)
					  .send(yield));
		return boost::system::error_code{};
	}

	std::vector<std::string> missing_permissions;
	auto missing = static_cast<size_t>(required) &
				   ~static_cast<size_t>(user_perms.value());

	if (missing == 0) { return outcome::success(); }

	for (const auto& [perm, name] : permission_strings) {
		if ((missing & static_cast<size_t>(perm)) != 0) {
			missing_permissions.emplace_back(name);
		}
	}

	SABER_TRY(bot.http()
				  .create_message(message.channel_id)
				  .content(fmt::format(
					  "{} need the following permissions: {}",
					  user_id == bot.bot_id() ? "I" : "You",
					  boost::algorithm::join(missing_permissions, ", ")))
				  .reply(message.id)
				  .send(yield));

	return boost::system::error_code{};
}

Result<boost::optional<ekizu::VoiceState&>> in_voice_channel(
	Saber& bot, const ekizu::Message& msg,
	const boost::asio::yield_context& yield) {
	if (!msg.guild_id) {
		return outcome::failure(boost::system::errc::invalid_argument);
	}

	auto voice_state =
		bot.voice_states().get(*msg.guild_id).flat_map([&](auto& users) {
			return users.get(msg.author.id);
		});

	if (!voice_state.map([](auto& state) { return !!state.channel_id; })
			 .value_or(false)) {
		SABER_TRY(bot.http()
					  .create_message(msg.channel_id)
					  .content("You are not in a voice channel!")
					  .reply(msg.id)
					  .send(yield));
		return outcome::failure(boost::system::error_code{});
	}

	return outcome::success(voice_state);
}
}  // namespace saber::util