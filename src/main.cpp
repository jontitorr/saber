#include <saber/saber.hpp>

int main()
{
	auto saber = saber::Saber{ std::getenv("DISCORD_TOKEN") };

	saber.run();
}
