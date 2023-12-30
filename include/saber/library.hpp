#ifndef SABER_LIBRARY_HPP
#define SABER_LIBRARY_HPP

#include <saber/export.h>

#include <ekizu/util.hpp>

#ifdef _WIN32
#define _WINSOCKAPI_  // stops windows.h including winsock.h
#include <Windows.h>
using dlopen_handle_t = HMODULE;
#else
#include <dlfcn.h>
using dlopen_handle_t = void *;
#endif

namespace saber {
template <typename T>
using Result = ekizu::Result<T>;

/**
 * @brief Simple cross-platform wrapper for platform-specific dlopen()-like
 * functions. Supports loading instances of objects with supporting allocater
 * and deallocater functions.
 *
 * @tparam T The type of object to load and manage.
 * @tparam Args The types of the arguments for the allocater.
 */
struct Library {
	static Result<Library> create(std::string_view path);

	template <typename T>
	Result<T> get(std::string_view name) const {
		if (m_handle == nullptr) { return boost::system::errc::bad_address; }

#ifdef _WIN32
		auto *ptr = ::GetProcAddress(m_handle, name.data());
#else
		auto *ptr = dlsym(m_handle, name.data());
#endif

		if (ptr == nullptr) { return boost::system::errc::bad_address; }

		return reinterpret_cast<T>(ptr);
	}

	Library(const Library &) = delete;
	Library &operator=(const Library &) = delete;
	Library(Library &&) noexcept;
	Library &operator=(Library &&) noexcept;
	SABER_EXPORT ~Library();

   private:
	explicit Library(dlopen_handle_t handle);

	/// Our handle to the shared library.
	dlopen_handle_t m_handle{};
};
}  // namespace saber

#endif	// SABER_LIBRARY_HPP
