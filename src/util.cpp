#include <algorithm>
#include <saber/util.hpp>

namespace saber::util {
std::vector<std::string> split(std::string_view s, std::string_view delimiter) {
	std::vector<std::string> ret;
	size_t pos{};
	size_t prev{};

	while ((pos = s.find(delimiter, prev)) != std::string::npos) {
		ret.emplace_back(s.substr(prev, pos - prev));
		prev = pos + delimiter.length();
	}

	ret.emplace_back(s.substr(prev));
	return ret;
}

std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
				return std::isspace(ch) == 0;
			}));

	return s;
}

std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
						 [](unsigned char ch) { return std::isspace(ch) == 0; })
				.base(),
			s.end());

	return s;
}

std::string &trim(std::string &s) { return ltrim(rtrim(s)); }
}  // namespace saber::util