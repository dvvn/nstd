#pragma once

#include "type_traits.h"

#include <ww898/utf_converters.hpp>

#ifdef _DEBUG
#include <algorithm>
#endif
#include <vector>

template < >
struct ww898::utf::detail::utf_selector<char8_t> final
{
	using type = utf8;
};

// ReSharper disable CppRedundantInlineSpecifier
namespace nstd
{
	template <typename CharType>
	concept unistring_support = std::destructible<ww898::utf::detail::utf_selector<CharType>>;

	namespace detail
	{
		template <std::random_access_iterator It, class Out>
		void unistring_convert_impl(It&& bg, It&& ed, Out& out)
		{
			using in_val = std::iter_value_t<It>;
			using out_val = typename Out::value_type;
			using namespace ww898::utf;
			using _In = utf_selector_t<in_val>;
			using _Out = utf_selector_t<out_val>;

			std::vector<out_val> buff;
			conv<_In, _Out>(std::forward<It>(bg), std::forward<It>(ed), std::back_inserter(buff));

			out.insert(out.end( ), buff.begin( ), buff.end( ));
		}

		template <typename In, class Out>
		void unistring_convert_impl(const In& in, Out& out)
		{
			namespace rn = std::ranges;
			unistring_convert_impl(rn::_Ubegin(in), rn::_Uend(in), out);
		}

		template <typename In, class Out>
		void unistring_copy_move(In&& in, Out& out)
		{
			using out_t = std::ranges::range_value_t<Out>;
			if (out.empty( ))
			{
				if constexpr (std::assignable_from<Out, In>)
					out = std::forward<In>(in);
				else
					out = Out(in.begin( ), in.end( ));
			}
			else
			{
				out.insert(out.end( ), in.begin( ), in.end( ));
			}
		}

		template <typename In, typename Out>
		constexpr bool compatible_allocators( )
		{
			if constexpr (!_Has_member_allocator_type<In>)
				return false;
			else
				return same_template<typename In::allocator_type, typename Out::allocator_type>;
		}

		template <std::ranges::random_access_range In, class Out>
		void unistring_convert(In&& in, Out& out)
		{
			namespace rn = std::ranges;
			using in_t = rn::range_value_t<In>;
			using out_t = rn::range_value_t<Out>;

			if constexpr (std::same_as<in_t, out_t>)
			{
				unistring_copy_move(std::forward<In>(in), out);
			}
			else if constexpr (sizeof(in_t) <= sizeof(out_t)
#ifdef __cpp_lib_char8_t
							   && !std::_Is_any_of_v<char8_t, in_t, out_t>
#endif
			)
			{
#ifdef _DEBUG
				std::vector<out_t> out_debug;
				unistring_convert_impl(out, out_debug);
				unistring_convert_impl(in, out_debug);
#endif
				using InRaw = std::remove_reference_t<In>;
				if constexpr (std::is_rvalue_reference_v<decltype(in)>
							  && sizeof(InRaw) == sizeof(Out)
							  && same_template<InRaw, Out>( )
							  && compatible_allocators<InRaw, Out>( ))
				{
					static_assert(same_template<std::allocator<out_t>, typename Out::allocator_type>( ), "Only default allocator type supported");
					runtime_assert(out.empty());
					std::swap(reinterpret_cast<Out&>(in), out);
				}
				else
				{
					unistring_copy_move(in, out);
				}
#ifdef _DEBUG
				runtime_assert(std::ranges::equal(out_debug, out));
#endif
			}
			else
			{
				unistring_convert_impl(in, out);
			}
		}

		template <typename Chr, size_t N, class Out>
		void unistring_convert(const Chr (&in)[N], Out& out)
		{
			using view_type = std::basic_string_view<Chr>;
			unistring_convert<view_type>(in, out);
		}
	}

	template <unistring_support CharType
	  , typename Tr = std::char_traits<CharType>, typename Al = std::allocator<CharType>, class Base = std::basic_string<CharType, Tr, Al>>
	class basic_unistring : public Base
	{
	public:
		using Base::basic_string;

		template <class T>
		NSTD_CONSTEXPR_CONTAINTER basic_unistring(T&& obj)
			: Base( )
		{
			detail::unistring_convert(std::forward<T>(obj), *this);
		}

		using Base::assign;

		template <class T>
		NSTD_CONSTEXPR_CONTAINTER basic_unistring& assign(T&& other)
		{
			auto tmp = basic_unistring(std::forward<T>(other));
			std::swap(*this, tmp);
			return *this;
		}

		using Base::append;

		template <class T>
		NSTD_CONSTEXPR_CONTAINTER basic_unistring& append(T&& str)
		{
			if (this->empty( ))
				return assign(std::forward<T>(str));

			detail::unistring_convert(str, *this);
			return *this;
		}

		template <class T>
			requires(!std::is_class_v<T> && !std::is_pointer_v<T>)
		NSTD_CONSTEXPR_CONTAINTER basic_unistring& append(T chr)
		{
			static_assert(sizeof(T) <= sizeof(CharType));
			Base::operator+=(static_cast<CharType>(chr));
			return *this;
		}

		/*template <typename T>
			requires(!has_array_access<T>)
		NSTD_CONSTEXPR_CONTAINTER basic_unistring& append(T str)
			requires(other_char_type_v<T>)
		{
			const T fake_string[] = { str, static_cast<T>('\0') };
			return this->append(fake_string);
		}*/
	};

	namespace detail
	{
		/*template <typename T>
		concept unistring_based_impl = basic_string_like<T> &&
			std::derived_from<T, basic_unistring<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;*/

		template <typename Test, typename T>
		struct size_checker : std::bool_constant<sizeof(Test) == sizeof(T)>
		{
			using type = T;
		};

		template <typename T, typename ...Ts>
		using unistring_type_selector_t = typename std::disjunction<size_checker<T, Ts>...>::type;
	}

	/*template <typename T>
	concept unistring_based = detail::unistring_based_impl<std::remove_cvref_t<T>>;

	template <typename T>
	concept unistring_covertible = !unistring_based<T> && requires(const T & obj)
	{
		detail::unistring_convert<wchar_t>(obj);
	};*/

	template <typename T>
	using unistring_select_type = detail::unistring_type_selector_t<T, char8_t, char16_t, char32_t>;

	template <typename T>
	using unistring = basic_unistring<unistring_select_type<T>>;

	/*template <unistring_covertible T>
	basic_unistring(T&& val)->basic_unistring<unistring_select_type<std::remove_cvref_t<decltype(val[0])>>>;

	template <unistring_covertible L, unistring_based R>
	bool operator==(L&& l, const R& r)
	{
		return detail::unistring_convert<typename R::value_type>(std::forward<L>(l)) == r;
	}

	template <unistring_covertible L, unistring_based R>
	bool operator!=(L&& l, const R& r)
	{
		return !operator==(std::forward<L>(l), r);
	}*/
}
