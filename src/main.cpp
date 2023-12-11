#include <iostream>
#include <saber/saber.hpp>

int main() {
	const auto* token = std::getenv("DISCORD_TOKEN");

	if (token == nullptr) {
		std::cerr << "Missing DISCORD_TOKEN environment variable\n";
		return 1;
	}

	auto saber = saber::Saber{token};

	saber.run();
}
