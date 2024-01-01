#ifndef SABER_REDDIT_HPP
#define SABER_REDDIT_HPP

#include <saber/export.h>

#include <ekizu/http.hpp>
#include <nlohmann/json_fwd.hpp>
#include <saber/result.hpp>

namespace saber {
namespace net = ekizu::net;

struct RedditOptions {
	std::string username{};
	std::string password{};
	std::string app_id{};
	std::string app_secret{};
};

struct RedditFields {
	std::optional<std::string> query{};
	// NSFW posts
	std::optional<bool> include_over_18{};
	// Restrict to the subreddit only.
	std::optional<bool> restrict_sr{};
	std::optional<std::string> sort{};
	std::optional<std::string> time{};
	std::optional<uint8_t> limit{};
	std::optional<uint64_t> count{};
	std::optional<std::string> subreddit{};
};

void to_json(nlohmann::json& j, const RedditFields& fields);

struct GetReddit {
	GetReddit& query(std::string_view query) {
		m_fields.query = query;
		return *this;
	}

	GetReddit& include_over_18(bool include_over_18) {
		m_fields.include_over_18 = include_over_18;
		return *this;
	}

	GetReddit& restrict_sr(bool restrict_sr) {
		m_fields.restrict_sr = restrict_sr;
		return *this;
	}

	GetReddit& sort(std::string_view sort) {
		m_fields.sort = sort;
		return *this;
	}

	GetReddit& time(std::string_view time) {
		m_fields.time = time;
		return *this;
	}

	GetReddit& limit(uint8_t limit) {
		m_fields.limit = limit;
		return *this;
	}

	GetReddit& count(uint64_t count) {
		m_fields.count = count;
		return *this;
	}

	GetReddit& subreddit(std::string_view subreddit) {
		m_fields.subreddit = subreddit;
		return *this;
	}

	[[nodiscard]] SABER_EXPORT Result<net::HttpResponse> send(
		const boost::asio::yield_context& yield);

   private:
	friend struct Reddit;

	explicit GetReddit(
		std::function<Result<net::HttpResponse>(
			std::string_view path, const boost::asio::yield_context& yield)>
			make_request);

	std::function<Result<net::HttpResponse>(
		std::string_view path, const boost::asio::yield_context& yield)>
		m_make_request;

	RedditFields m_fields;
};

struct Reddit {
	SABER_EXPORT explicit Reddit(RedditOptions options);

	SABER_EXPORT GetReddit get();
	// SABER_EXPORT Result<net::HttpResponse> get(
	// 	std::string_view path, const nlohmann::json& params,
	// 	const boost::asio::yield_context& yield);

   private:
	// Function which makes a request to the reddit API.
	Result<net::HttpResponse> request(const net::HttpRequest& req,
									  const boost::asio::yield_context& yield);

	Result<std::string> get_token(const boost::asio::yield_context& yield);
	RedditOptions m_options;
	std::optional<net::HttpConnection> m_http;
	std::string m_token;
	std::chrono::system_clock::time_point m_token_expiration;
};
}  // namespace saber

#endif	// SABER_REDDIT_HPP
