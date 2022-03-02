#pragma once
#include <concepts>
#include <xutility>

namespace std
{
	template <class _Elem>
	struct char_traits;

	template <class _Ty>
	class allocator;

	template <class _Elem, class _Traits /*= char_traits<_Elem>*/, class _Alloc /*= allocator<_Elem>*/>
	class basic_string;

	template <class _Elem, class _Traits /*= char_traits<_Elem>*/>
	class basic_string_view;
}

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

	template <class T, typename New>
	using rebind_helper = typename std::_Replace_first_parameter<New, T>::type;

#if 0
#pragma region STRING TOOLS
	namespace detail
	{
		template <typename T>
		concept basic_string_like = requires
		{
			typename T::value_type;
			typename T::traits_type;
			typename T::allocator_type;
		};

		template <typename T>
		concept std_string_based_impl = basic_string_like<T> &&
			std::derived_from<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;

		template <typename T>
		concept basic_string_view_like = requires
		{
			typename T::value_type;
			typename T::traits_type;
		};

		template <typename T>
		concept std_string_view_based_impl = basic_string_view_like<T> &&
			std::derived_from<T, std::basic_string_view<typename T::value_type, typename T::traits_type>>;

		template <typename T>
		concept std_string_or_view_impl = basic_string_view_like<T> &&
			std::constructible_from<std::basic_string_view<typename T::value_type, typename T::traits_type>, T>;
	}

	template <typename T>
	concept std_string_based = detail::std_string_based_impl<std::remove_cvref_t<T>>;

	template <typename T>
	concept std_string_view_based = detail::std_string_view_based_impl<std::remove_cvref_t<T>>;

	template <typename T>
	concept std_string_or_view = detail::std_string_or_view_impl<std::remove_cvref_t<T>>;

#pragma endregion
#endif


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
}

#if defined(_CONSTEXPR20_CONTAINTER)
#define NSTD_CONSTEXPR_CONTAINTER _CONSTEXPR20_CONTAINTER
#elif defined(_CONSTEXPR20)
#define NSTD_CONSTEXPR_CONTAINTER _CONSTEXPR20
#else
#define NSTD_CONSTEXPR_UNISTRING 
#endif