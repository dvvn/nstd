#pragma once
#include "type_traits.h"

#include <algorithm>
#include <string>
#include <array>
#ifdef _DEBUG
#include <stdexcept>
#endif

namespace nstd
{
	constexpr void erase_substring(std::string& str, const std::string_view& substr)
	{
#if 1
		size_t pos = 0;
		for (;;)
		{
			pos = str.find(substr, pos);
			if (pos == str.npos)
				return;
			str.erase(pos, substr.size( ));
		}
#else
		auto itr = str.begin( );
		for (;;)
		{
			itr = std::search(itr, str.end( ), substr.begin( ), substr.end( ));
			if (itr == str.end( ))
				return;
			itr = str.erase(itr, itr + substr.size( ));
		}
#endif
	}

	template<typename Chr, size_t Size>
	struct buffered_string
	{
		using array_type = std::array<Chr, Size>;
		using traits_type = std::char_traits<Chr>;

		using _View = std::basic_string_view<Chr, traits_type>;

		array_type str;
		size_t str_size;

		constexpr buffered_string(const array_type& arr, size_t size) :str(arr), str_size(size)
		{
#ifdef _DEBUG
			if (arr.size( ) <= str_size)
				throw std::logic_error("Incorrect array size");
			if (arr.back( ) != 0)
				throw std::logic_error("Incorrect array null character");
#endif
		}
		constexpr buffered_string(const array_type& arr) : str(arr), str_size(traits_type::length(arr._Unchecked_begin( ))) { }

		constexpr bool ideal( ) const { return str.size( ) - 1 == str_size; }
		constexpr auto view( ) const { return _View(str._Unchecked_begin( ), str_size); }

		template<size_t StrSize>
		constexpr buffered_string<Chr, StrSize + 1> make_ideal( )const
		{
#ifdef _DEBUG
			if (StrSize != str_size)
				throw std::logic_error("Incorrect StrSize");
			if (ideal( ))
				throw std::logic_error("Already ideal");
#endif
			std::array<Chr, StrSize + 1> out = {};
			out.back( ) = 0;
			std::copy(str.begin( ), str.begin( ) + StrSize, out.begin( ));
			return {out, StrSize};
		}
	};

	template<typename Chr, size_t Size>
	buffered_string(std::array<Chr, Size>)->buffered_string<Chr, Size>;

	template<typename Chr, size_t Size, typename Sv>
	constexpr bool operator==(const buffered_string<Chr, Size>& l, const Sv& r)
	{
		return l.view( ) == r;
	}

	template<typename It>
	constexpr void filter_substring(It writer, const std::string_view& str, const std::string_view& substr)
	{
		bool already_filled = *writer != 0;
		auto itr = str.begin( );
		const auto end = str.end( );
		for (;;)
		{
			const auto limit = std::distance(itr, end);
			if (limit < substr.size( ))
			{
				if (!already_filled)
					*std::copy(itr, end, writer) = 0;

				break;
			}

			already_filled = false;

			if (std::equal(substr.begin( ), substr.end( ), itr))
				itr += substr.size( );
			else
				*writer++ = *itr++;
		}

#if 0
		size_t pos = 0;
		size_t old_pos;

		const auto copy = [&]<typename Itr>(Itr end)
		{
			Itr begin = str.begin( ) + old_pos;
			std::copy(begin, end, writer);
			//writer += std::distance(begin, end);
		};

		for (;;)
		{
			old_pos = pos;
			pos = str.find(substr, pos);
			if (pos == str.npos)
			{
				copy(str.end( ));
				*writer = 0;
				return;
			}
			copy(str.begin( ) + pos);
		}
#endif
	}

	namespace detail
	{
		template<typename Chr, size_t Size>
		constexpr auto prepare_buffer(size_t limit = static_cast<size_t>(-1))
		{
			std::array<Chr, Size + 1> buffer = {};
			/*if (buffer.size( ) >= limit)
				throw std::length_error("Buffer too small");*/
			*buffer.begin( ) = 0;
			return buffer;
		}

		template <typename T>
		constexpr auto type_name_impl( )
		{
			constexpr std::string_view n0 = type_name_impl0<T>( );

			constexpr auto start = n0.find('<') + 1;
			constexpr auto end = n0.rfind('>');
			constexpr auto name_size = end - start;

#if 1
			constexpr auto name = n0.substr(start, name_size);
			auto buffer = prepare_buffer<char, name_size>( );
			auto bg = buffer.begin( );

			constexpr bool _Class = std::is_class_v<T>;
			constexpr bool _Enum = std::is_enum_v<T>;
			constexpr bool _Union = std::is_union_v<T>;

			if constexpr (!_Class && !_Enum && !_Union)
			{
				std::ranges::copy(name, bg);
				buffer.back( ) = 0;
			}
			else
			{
				if constexpr (_Class)
				{
					filter_substring(bg, name, "struct ");
					filter_substring(bg, name, "class ");
				}
				//class checked because of inner templates
				if constexpr (_Class || _Enum)
					filter_substring(bg, name, "enum ");
				if constexpr (_Class || _Union)
					filter_substring(bg, name, "union ");
			}

			return buffered_string(buffer);

#else

			auto name = std::string(n0.substr(start, name_size));

			erase_substring(name, "struct ");
			erase_substring(name, "class ");
			erase_substring(name, "enum ");
			erase_substring(name, "union ");

			return string_to_buffer<name_size>(name);
#endif

		}

		template <typename T>
		_INLINE_VAR constexpr auto type_name_holder = []
		{
			constexpr auto tmp = type_name_impl<T>( );
			if constexpr (tmp.ideal( ))
				return tmp;
			else
				return tmp.make_ideal<tmp.str_size>( );
		}();

		constexpr auto drop_namespace_impl(std::string& str, const std::string_view& drop)
		{
			std::string drop_str;
			std::string_view drop_sv;

			if (drop.ends_with("::"))
			{
				drop_sv = drop;
			}
			else
			{
				drop_str = drop;
				if (drop.back( ) == ':')//skip empty check
					drop_str += ':';
				else
					drop_str += "::";
				drop_sv = drop_str;
			}
			erase_substring(str, drop_sv);
		}
	}


	template <typename T>
	_INLINE_VAR constexpr auto type_name = detail::type_name_holder<T>.view( );

	static_assert(type_name<int> == "int");

#if 1
	constexpr auto drop_namespace(const std::string_view& in, const std::string_view& drop)
	{
#ifdef _DEBUG
		if (drop.ends_with(":::")
			|| (drop.ends_with("::") && drop.size( ) == 2)
			|| (drop.ends_with(':') && drop.size( ) == 1)
			|| drop.empty( ))
		{
			throw std::logic_error("Incorrect drop namespace");
		}
#endif

		auto buffer = detail::prepare_buffer<char, 255>(in.size( ));
		auto bg = buffer.begin( );
		if (drop.ends_with("::"))
		{
			filter_substring(bg, in, drop);
		}
		else
		{
			auto tmp_buffer = detail::prepare_buffer<char, 63>(drop.size( ));
			auto pos = std::copy(drop.begin( ), drop.end( ), tmp_buffer.begin( ));
			*pos++ = ':';
			auto tmp_buffer_size = drop.size( ) + 1;
			if (!drop.ends_with(':'))
			{
				*pos++ = ':';
				++tmp_buffer_size;
			}
			*pos = 0;//maybe unwanted
			filter_substring(bg, in, {tmp_buffer._Unchecked_begin( ),tmp_buffer_size});
		}

		return buffered_string(buffer);
	}
	static_assert(drop_namespace("test::test::string", "test") == "string");
	static_assert(drop_namespace("test::test2::string", "test2") == "test::string");
#else
	template<typename Str>
	constexpr auto drop_namespace(Str&& in, const std::string_view& drop)
	{
		auto out = std::string(std::forward<Str>(in));
		detail::drop_namespace_impl(out, drop);
		return out;
	}
#endif

}
