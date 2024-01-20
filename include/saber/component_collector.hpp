#ifndef SABER_COMPONENT_LISTENER_HPP
#define SABER_COMPONENT_LISTENER_HPP

#include <saber/export.h>

#include <boost/asio/experimental/channel.hpp>
#include <ekizu/interaction.hpp>
#include <saber/result.hpp>

namespace saber {
struct ComponentCollector {
	SABER_EXPORT ComponentCollector(
		ekizu::ComponentType component_type,
		std::chrono::steady_clock::duration expiry,
		std::function<bool(const ekizu::Interaction&,
						   const ekizu::MessageComponentData&)>
			filter,
		const boost::asio::yield_context& yield);

	SABER_EXPORT Result<std::tuple<const ekizu::Interaction*,
								   const ekizu::MessageComponentData*>>
	async_receive(const boost::asio::yield_context& yield);

	SABER_EXPORT void async_send(const ekizu::Interaction& interaction,
								 const boost::asio::yield_context& yield);

   private:
	ekizu::ComponentType m_component_type;
	std::function<bool(
		const ekizu::Interaction&, const ekizu::MessageComponentData&)>
		m_filter;
	boost::asio::experimental::channel<void(
		boost::system::error_code, const ekizu::Interaction*,
		const ekizu::MessageComponentData*)>
		m_on_collect_chan;
	std::function<void(const boost::asio::yield_context&)> m_on_end;
};
}  // namespace saber

#endif	// SABER_COMPONENT_LISTENER_HPP
