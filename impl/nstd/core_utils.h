#pragma once
#include <concepts>
#include <utility>

#ifndef _CONCAT
#define NSTD_CONCATX(x, y) x##y
#define NSTD_CONCAT(x, y) NSTD_CONCATX(x, y)
#else
#define NSTD_CONCAT _CONCAT
#endif

#ifndef _STRINGIZE
#define NSTD_STRINGIZEX(x) #x
#define NSTD_STRINGIZE(x) NSTD_STRINGIZEX(x)
#else
#define NSTD_STRINGIZE _STRINGIZE
#endif

#ifndef _CRT_WIDE
#define NSTD_STRINGIZE_WIDE(x) NSTD_CONCAT(L,NSTD_STRINGIZE(x))
#else
#define NSTD_STRINGIZE_WIDE _CRT_WIDE
#endif

#define NSTD_STRINGIZE_RAW(x) NSTD_CONCAT(R,NSTD_STRINGIZE(##(x)##))
#define NSTD_STRINGIZE_RAW_WIDE(x) NSTD_CONCAT(L,NSTD_STRINGIZE_RAW(x))

namespace nstd
{
	template <typename T>
	concept has_array_access = requires(const T & obj)
	{
		obj[0u];
	};

	template <class _Ty>
	concept _Has_member_allocator_type = requires {
		typename _Ty::allocator_type;
	};

	// template <class T, typename New>
	// using rebind_helper = typename std::_Replace_first_parameter<New, T>::type;

	template <typename T>
	struct remove_all_pointers : std::conditional_t<
		std::is_pointer_v<T>,
		remove_all_pointers<std::remove_pointer_t<T>>,
		std::type_identity<T> >
	{
	};

	template <typename T>
	using remove_all_pointers_t = typename remove_all_pointers<T>::type;

	template <bool Val, typename T>
	using add_const_if_v = std::conditional_t<Val, std::add_const_t<T>, T>;

	template <typename Test, typename T>
	using add_const_if = add_const_if_v<std::is_const_v<Test>, T>;

#ifdef __cpp_lib_unreachable
	using std::unreachable;
#else
	[[noreturn]]
	inline void unreachable() noexcept
	{
		std::abort();
	}
#endif
}