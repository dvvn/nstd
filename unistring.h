#pragma once

#include <ww898/utf_converters.hpp>

#include "type_traits.h"

template < >
struct ww898::utf::detail::utf_selector<char8_t> final
{
	using type = utf8;
};

// ReSharper disable CppRedundantInlineSpecifier
namespace nstd
{
	namespace detail
	{
		template <typename Test, typename T>
		struct size_checker : std::bool_constant<sizeof(Test) == sizeof(T)>
		{
			using type = T;
		};

		template <typename T, typename ...Ts>
		using unistring_type_selector_t = typename std::disjunction<size_checker<T, Ts>...>::type;

		template <typename CharType>
		concept unistring_valid = std::destructible<ww898::utf::detail::utf_selector<CharType>>;

		template <class T, typename New>
		using rebind_helper = typename std::_Replace_first_parameter<New, T>::type;

		template <typename Out, typename In>
		_INLINE_VAR constexpr bool unistring_must_be_converted = sizeof(Out) < sizeof(In) || std::_Is_any_of_v<char8_t, Out, In> && !std::same_as<Out, In>;

		template <typename In, typename Out>
		void unistring_convert_impl(const In& in, Out& out)
		{
			using namespace ww898::utf;
			conv<utf_selector_t<typename In::value_type>, utf_selector_t<typename Out::value_type>>(
					in._Unchecked_begin( ), in._Unchecked_end( ), std::back_inserter(out)
					);
		}

		template <typename In, typename Out>
		void unistring_rewrap_impl(In&& in, Out& out)
		{
			using input_type = std::remove_cvref_t<In>;
			using value_type = typename Out::value_type;

			// ReSharper disable once CppInconsistentNaming
			auto _Size = [&]
			{
				if constexpr (std::is_class_v<input_type>)
					return in.size( );
				else
				{
					static_assert(std::is_bounded_array_v<input_type>, __FUNCTION__": unknown input type");

					constexpr size_t val = sizeof(input_type) / sizeof(value_type);
					return val - 1;
				}
			};

			if constexpr (sizeof(std::remove_cvref_t<decltype(in[0])>) != sizeof(value_type))
			{
				out.reserve(_Size( ));
				for (auto c : in)
					out.push_back(static_cast<value_type>(c));
			}
			else if constexpr (std_string_based<input_type> && std::is_rvalue_reference_v<input_type>)
			{
				/*using char_type = typename input_type::value_type;
				using traits_type = typename input_type::traits_type;
				using allocator_type = typename input_type::allocator_type;

				using in_str_type = std::basic_string<char_type, traits_type, allocator_type>;*/
				out = reinterpret_cast<Out&&>(in);
			}
			else
			{
				// ReSharper disable once CppInconsistentNaming
				auto _Begin = std::addressof(in[0]);
				// ReSharper disable once CppInconsistentNaming
				auto _End = std::next(_Begin, _Size( ));
				out.append(static_cast<const value_type*>(_Begin), static_cast<const value_type*>(_End));
			}
		}

		template <typename CharType, std_string_based T>
		decltype(auto) unistring_convert(T&& in)
		{
			using Traw = std::remove_cvref_t<T>;
			//static_assert(std::_Is_specialization_v<typename Traw::traits_type, std::char_traits>, __FUNCTION__": custom char_traits unsupported");

			using char_type = typename Traw::value_type;
			using traits_type = typename Traw::traits_type;
			using allocator_type = typename Traw::allocator_type;

			using in_str_type = std::basic_string<char_type, traits_type, allocator_type>;
			using out_str_type = std::basic_string<CharType, rebind_helper<traits_type, CharType>, rebind_helper<allocator_type, CharType>>;

			if constexpr (std::convertible_to<in_str_type, out_str_type>)
			{
				using in_type = decltype(in);
				if constexpr (std::is_rvalue_reference_v<in_type>)
					return (static_cast<out_str_type&&>(in));
				else if constexpr (std::is_const_v<in_type>)
					return static_cast<const out_str_type&>(in);
				else
					return static_cast<out_str_type&>(in);
			}
			else
			{
				out_str_type out;

				if constexpr (unistring_must_be_converted<CharType, char_type>)
					unistring_convert_impl<in_str_type>((in), out);
				else
					unistring_rewrap_impl/*<in_str_type>*/(std::forward<T>(in), out);

				return out;
			}
		}

		template <typename CharType, std_string_view_based T>
		auto unistring_convert(const T& in)
		{
			using Traw = std::remove_reference_t<T>;

			using char_type = typename Traw::value_type;
			using traits_type = typename Traw::traits_type;

			using in_str_type = std::basic_string_view<char_type, traits_type>;

			if constexpr (std::same_as<CharType, char_type>)
			{
				//static_assert(std::is_trivially_destructible_v<T>, __FUNCTION__": custom string_view detected");
				//return in;
				return static_cast<const in_str_type&>(in);
			}
			else
			{
				using out_str_type = std::basic_string<CharType, rebind_helper<traits_type, CharType>>;
				out_str_type out;

				if constexpr (unistring_must_be_converted<CharType, char_type>)
					unistring_convert_impl<in_str_type>(in, out);
				else
					unistring_rewrap_impl/*<in_str_type>*/(in, out);

				return out;
			}
		}

		template <typename CharType, typename Chr, size_t N>
		auto unistring_convert(const Chr (&in)[N])
		{
			using char_type = Chr;

			using in_str_type = std::basic_string_view<char_type>;

			if constexpr (std::same_as<CharType, char_type>)
			{
#if _CONTAINER_DEBUG_LEVEL > 0
				_STL_ASSERT(in_str_type::traits_type::length(in) == N - 1, "string contains unsupported character inside!");
#endif
				return in_str_type(in, N - 1);
			}
			else
			{
				using out_str_type = std::basic_string<CharType>;
				out_str_type out;

				if constexpr (unistring_must_be_converted<CharType, char_type>)
					unistring_convert_impl<in_str_type>(in, out);
				else
					unistring_rewrap_impl<in_str_type>(in, out);

				return out;
			}
		}

		template <typename CharType, typename Ptr>
			requires(std::is_pointer_v<Ptr>)
		auto unistring_convert(Ptr cstr)
		{
			//if you're passing a temporary pointer here, go fuck yourself
			return unistring_convert(std::basic_string_view<std::remove_const_t<std::remove_pointer_t<Ptr>>>(cstr));
		}
	}

	template <detail::unistring_valid CharType   //--
	  , typename Tr = std::char_traits<CharType> //--
	  , typename Al = std::allocator<CharType>>
	class basic_unistring : public std::basic_string<CharType, Tr, Al>
	{
		template <typename Chr>
		static constexpr bool other_char_type_v = !std::same_as<CharType, Chr> && detail::unistring_valid<Chr>;

	public:
		using _Str_type = std::basic_string<CharType, Tr, Al>;

		_CONSTEXPR20_CONTAINER basic_unistring() = default;

		template <has_array_access T>
		_CONSTEXPR20_CONTAINER basic_unistring(T&& obj)
			: _Str_type(detail::unistring_convert<CharType>(std::forward<T>(obj)))
		{
		}

		template <has_array_access T>
		_CONSTEXPR20_CONTAINER bool starts_with(T&& other) const
			requires(other_char_type_v<std::remove_cvref_t<decltype(other[0])>>)
		{
			const auto other2 = detail::unistring_convert<CharType>(std::forward<T>(other));
			return _Str_type::starts_with(other);
		}

		using _Str_type::starts_with;

		template <has_array_access T>
		_CONSTEXPR20_CONTAINER basic_unistring& assign(T&& other)
			requires(other_char_type_v<std::remove_cvref_t<decltype(other[0])>>)
		{
			_Str_type::assign(detail::unistring_convert<CharType>(std::forward<T>(other)));
			return *this;
		}

		using _Str_type::assign;

		template <has_array_access T>
		_CONSTEXPR20_CONTAINER basic_unistring& append(T&& str)
			requires(other_char_type_v<std::remove_cvref_t<decltype(str[0])>>)
		{
			_Str_type::append(detail::unistring_convert<CharType>(std::forward<T>(str)));
			return *this;
		}

		template <typename T>
			requires(!has_array_access<T>)
		_CONSTEXPR20_CONTAINER basic_unistring& append(const T& str)
			requires(other_char_type_v<T>)
		{
			const T fake_string[] = {str, static_cast<T>('\0')};
			return this->append(fake_string);
		}

		using _Str_type::append;
	};

	namespace detail
	{
		template <typename T>
		concept unistring_based_impl = basic_string_like<T> &&
									   std::derived_from<T, basic_unistring<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;
	}

	template <typename T>
	concept unistring_based = detail::unistring_based_impl<std::remove_cvref_t<T>>;

	template <typename T>
	concept unistring_covertible = !unistring_based<T> && requires(const T& obj)
	{
		detail::unistring_convert<wchar_t>(obj);
	};

	template <typename T>
	using unistring_select_type = detail::unistring_type_selector_t<T, char8_t, char16_t, char32_t>;

	template <typename T>
	using unistring = basic_unistring<unistring_select_type<T>>;

	template <unistring_covertible T>
	basic_unistring(T&& val) -> basic_unistring<unistring_select_type<std::remove_cvref_t<decltype(val[0])>>>;

	template <unistring_covertible L, unistring_based R>
	bool operator==(L&& l, const R& r)
	{
		return detail::unistring_convert<typename R::value_type>(std::forward<L>(l)) == r;
	}

	template <unistring_covertible L, unistring_based R>
	bool operator!=(L&& l, const R& r)
	{
		return !operator==(std::forward<L>(l), r);
	}
}
