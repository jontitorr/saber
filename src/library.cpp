#include <saber/library.hpp>

#include "fmt/core.h"

namespace saber {
Result<Library> Library::create(std::string_view path) {
	if (path.empty()) {
		return tl::make_unexpected(
			std::make_error_code(std::errc::bad_address));
	}

#ifdef _WIN32
	auto *handle = LoadLibrary(path.data());
#else
	auto *handle = dlopen(path.data(), RTLD_NOW | RTLD_LOCAL);
#endif

	if (handle == nullptr) {
		fmt::print("Failed to load library: {}\n", dlerror());
		return tl::make_unexpected(
			std::make_error_code(std::errc::bad_address));
	}

	return Library{handle};
}

Library::Library(dlopen_handle_t handle) : m_handle{handle} {}

Library::Library(Library &&other) noexcept : m_handle{other.m_handle} {
	other.m_handle = nullptr;
}

Library &Library::operator=(Library &&other) noexcept {
	if (this != &other) {
		m_handle = other.m_handle;
		other.m_handle = nullptr;
	}

	return *this;
}

Library::~Library() {
	if (m_handle == nullptr) { return; }

#ifdef _WIN32
	FreeLibrary(m_handle);
#else
	dlclose(m_handle);
#endif
}
}  // namespace saber
