#include <saber/util.hpp>

namespace saber::util {
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