#include <iostream>
#include <saber/saber.hpp>

static constexpr uint64_t DEFAULT_OWNER_ID{155780111197536256};

int main() {
	const auto* token = std::getenv("DISCORD_TOKEN");

	if (token == nullptr) {
		std::cerr << "Missing DISCORD_TOKEN environment variable\n";
		return 1;
	}

	const auto* owner_id_str = std::getenv("OWNER_ID");
	ekizu::Snowflake owner_id{
		owner_id_str != nullptr ? std::stoull(owner_id_str) : DEFAULT_OWNER_ID};

	auto saber = saber::Saber{token, owner_id};

	saber.run();
}
