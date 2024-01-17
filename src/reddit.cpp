#include <fmt/format.h>

#include <boost/beast/core/detail/base64.hpp>
#include <boost/url/encode.hpp>
#include <boost/url/grammar/alnum_chars.hpp>
#include <ekizu/json_util.hpp>
#include <saber/reddit.hpp>

namespace {
constexpr const char* API_HOST = "oauth.reddit.com";
constexpr const char* ACCESS_TOKEN_PATH = "/api/v1/access_token";

std::string base64_encode(std::string_view str) {
	std::string ret;
	ret.resize(boost::beast::detail::base64::encoded_size(str.size()));
	boost::beast::detail::base64::encode(ret.data(), str.data(), str.size());
	return ret;
}

std::string to_query_string(const saber::RedditFields& fields) {
	std::vector<std::string> query_elements;

	auto add_query_param =
		[&query_elements](const std::string& key, const auto& opt_value) {
			if (opt_value) {
				query_elements.push_back(fmt::format(
					"{}={}", key,
					boost::urls::encode(fmt::to_string(opt_value.value()),
										boost::urls::grammar::alnum_chars)));
			}
		};

	add_query_param("q", fields.query);
	add_query_param("include_over_18", fields.include_over_18);
	add_query_param("restrict_sr", fields.restrict_sr);
	add_query_param("sort", fields.sort);
	add_query_param("time", fields.time);
	add_query_param("limit", fields.limit);
	add_query_param("count", fields.count);

	return fmt::format(
		"{}/search?{}",
		fields.subreddit ? fmt::format("/r/{}", fields.subreddit.value()) : "",
		fmt::join(query_elements, "&"));
}
}  // namespace

namespace saber {

Result<net::HttpResponse> GetReddit::send(
	const boost::asio::yield_context& yield) {
	if (!m_make_request) {
		return boost::system::errc::operation_not_permitted;
	}

	return m_make_request(to_query_string(m_fields), yield);
}

GetReddit::GetReddit(
	std::function<Result<net::HttpResponse>(
		std::string_view path, const boost::asio::yield_context& yield)>
		make_request)
	: m_make_request{std::move(make_request)} {}

Reddit::Reddit(RedditOptions options) : m_options{std::move(options)} {}

SABER_EXPORT GetReddit Reddit::get() {
	return GetReddit(
		[this](std::string_view path, const boost::asio::yield_context& yield)
			-> Result<net::HttpResponse> {
			SABER_TRY(get_token(yield));
			net::HttpRequest req{net::HttpMethod::get, path, 11};
			req.set(net::http::field::authorization, m_token);
			req.set(net::http::field::host, API_HOST);
			req.set(net::http::field::user_agent, "insomnia/8.4.5");
			req.prepare_payload();
			return request(req, yield);
		});
}

Result<net::HttpResponse> Reddit::request(
	const net::HttpRequest& req, const boost::asio::yield_context& yield) {
	if (!m_http) {
		SABER_TRY(auto http, net::HttpConnection::connect(
								 "https://www.reddit.com", yield));
		m_http = std::move(http);
	}

	if (auto res = m_http->request(req, yield); res) { return res; }

	SABER_TRY(auto http,
			  net::HttpConnection::connect("https://www.reddit.com", yield));
	m_http = std::move(http);

	return m_http->request(req, yield);
}

Result<std::string> Reddit::get_token(const boost::asio::yield_context& yield) {
	// If our access token is still valid, we can return it.
	if (std::chrono::system_clock::now() < m_token_expiration) {
		return m_token;
	}

	net::HttpRequest req{
		net::HttpMethod::post,
		fmt::format("{}?grant_type=password&username={}&password={}",
					ACCESS_TOKEN_PATH, m_options.username, m_options.password),
		11};
	const auto authorization = fmt::format(
		"Basic {}", base64_encode(fmt::format(
						"{}:{}", m_options.app_id, m_options.app_secret)));

	req.set(net::http::field::authorization, authorization);
	req.set(net::http::field::host, "www.reddit.com");
	req.set(net::http::field::user_agent, "insomnia/8.4.5");
	req.prepare_payload();

	SABER_TRY(auto res, request(req, yield));

	if (res.result() != net::http::status::ok) {
		return boost::system::errc::invalid_argument;
	}

	SABER_TRY(auto data, ekizu::json_util::try_parse(res.body()));

	if (!ekizu::json_util::not_null_all(data, "token_type", "access_token")) {
		return boost::system::errc::invalid_argument;
	}

	m_token = fmt::format("{} {}", data["token_type"].get<std::string>(),
						  data["access_token"].get<std::string>());
	m_token_expiration =
		std::chrono::system_clock::now() +
		std::chrono::seconds(data["expires_in"].get<uint32_t>());

	return m_token;
}
}  // namespace saber
