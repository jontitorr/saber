#include <ekizu/json_util.hpp>
#include <filesystem>
#include <fstream>
#include <saber/config.hpp>

namespace {
template <typename T>
void set_from_env(std::string_view name, T& value) {
	const auto* value_e = std::getenv(name.data());
	if (value_e == nullptr) { return; }
	value = nlohmann::json(value_e);
}
}  // namespace

namespace saber {
ekizu::Result<Config> Config::from_file(std::string_view path) {
	Config config;

	if (std::filesystem::exists(path)) {
		std::ifstream file(path.data());

		if (!file.is_open()) {
			return boost::system::errc::no_such_file_or_directory;
		}

		const std::string contents((std::istreambuf_iterator<char>(file)),
								   std::istreambuf_iterator<char>());
		const auto json = nlohmann::json::parse(contents, nullptr, false);

		if (json.is_discarded() || !json.is_object()) {
			return boost::system::errc::invalid_argument;
		}

		config.from_json(json);
	}

	config.from_env();
	return config;
}

void Config::from_env() {
	if (token.empty()) { set_from_env("SABER_TOKEN", token); }
	if (prefix.empty()) { set_from_env("SABER_PREFIX", prefix); }
	if (owner_id.id == 0) { set_from_env("SABER_OWNER_ID", owner_id); }
}

void Config::from_json(const nlohmann::json& j) {
	using ekizu::json_util::deserialize;

	deserialize(j, "token", token);
	deserialize(j, "prefix", prefix);
	deserialize(j, "owner_id", owner_id);
}
}  // namespace saber
