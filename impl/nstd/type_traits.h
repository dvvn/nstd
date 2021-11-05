#pragma once
#include <concepts>

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
	concept has_array_access = requires(const T& obj)
	{
		obj[0u];
	};

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

	template <typename T>
		struct remove_all_pointers : std::conditional_t<
					std::is_pointer_v<T>,
					remove_all_pointers<std::remove_pointer_t<T>>,
					std::type_identity<T>
				>
		{
		};

		template <typename T>
		using remove_all_pointers_t = typename remove_all_pointers<T>::type;
}
