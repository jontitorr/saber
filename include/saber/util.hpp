#ifndef SABER_UTIL_HPP
#define SABER_UTIL_HPP

#include <limits>
#include <random>

namespace saber::util {
template <typename T>
T get_random_number(T begin = (std::numeric_limits<T>::min)(),
					T end = (std::numeric_limits<T>::max)()) {
	std::random_device rd;
	std::mt19937 gen(rd());

	if constexpr (std::is_floating_point_v<T>) {
		std::uniform_real_distribution<T> dis(begin, end);
		return dis(gen);
	} else {
		std::uniform_int_distribution<T> dis(begin, end);
		return dis(gen);
	}
}
}  // namespace saber::util

#endif	// SABER_UTIL_HPP
