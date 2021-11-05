// ReSharper disable CppRedundantInlineSpecifier
#pragma once
#include "chars cache.h"
#include "type_traits.h"

namespace nstd
{
	namespace detail
	{
		template <typename Chr, size_t Size>
		constexpr auto remove_substring_start(const std::array<Chr, Size>& buffer, const std::basic_string_view<Chr>& pattern)
		{
			if (buffer.size( ) < pattern.size( ))
				return buffer;

			auto begin = buffer._Unchecked_begin( );
			auto end   = buffer._Unchecked_end( );
			auto end0  = end - pattern.size( );

			auto out   = std::array<Chr, Size>{};
			out[0]     = '\0';
			size_t pos = 0;

			for (;;)
			{
				auto test = std::basic_string_view<Chr>(begin, pattern.size( ));
				if (test == pattern)
					begin += pattern.size( );
				else
					out[pos++] = *begin++;

				if (begin >= end0)
				{
					while (begin < end)
						out[pos++] = *begin++;
					break;
				}
			}

			return out;
		}

		template <typename Chr, size_t Size, chars_cache Name, chars_cache ...Next>
		constexpr auto remove_substrings_start(const std::array<Chr, Size>& buffer)
		{
			auto temp_buff = remove_substring_start<Chr, Size>(buffer, Name.view( ));
			if constexpr (sizeof...(Next) == 0)
				return temp_buff;
			else
				return remove_substrings_start<Chr, Size, Next...>(temp_buff);
		}

		template <typename Chr, size_t Size>
		constexpr auto remove_substring_get_real_size(const std::array<Chr, Size>& buffer)
		{
			size_t count = 0;
			for (auto chr: buffer)
			{
				if (chr == '\0')
					break;
				++count;
			}
			return count;
			//return buffer.back( ) != '\0' ? buffer.size( ) : std::char_traits<Chr>::length(buffer.data( ));
		}

		template <size_t RealSize, typename Chr, size_t Size>
		constexpr auto remove_substring_end(const std::array<Chr, Size>& buffer)
		{
			auto out    = std::array<Chr, RealSize + 1>{};
			out.back( ) = '\0';
			std::copy(buffer.begin( ), buffer.begin( ) + RealSize, out.begin( ));
			return out;
		}

		template <typename T>
		constexpr decltype(auto) type_name_impl0( ) { return __FUNCSIG__; }

		template <typename T>
		constexpr auto type_name_impl( )
		{
			constexpr std::string_view n0 = type_name_impl0<T>( );

			constexpr auto start = n0.find('<') + 1;
			constexpr auto end   = n0.rfind('>');

			constexpr auto name_size = end - start;

			constexpr auto name = n0.substr(start, name_size);

			constexpr auto buff0 = [&]
			{
				auto tmp = std::array<char, name_size>{};
				std::copy(name.begin( ), name.end( ), tmp.begin( ));
				return tmp;
			}( );

			constexpr auto buff1 = remove_substrings_start<char, name_size, "struct ", "class ", "enum ", "union ">(buff0);
			return remove_substring_end<remove_substring_get_real_size(buff1)>(buff1);
		}

		template <chars_cache Namespace>
		constexpr auto fix_namespace_name( )
		{
			constexpr auto add_dots = []<size_t Num>(std::in_place_index_t<Num>)
			{
				auto buff  = chars_cache<Namespace.size + Num, char>(Namespace);
				auto buff2 = (buff.cache.rbegin( ));

				for (auto i = 1; i <= Num; ++i)
				{
					*std::next(buff2, i) = ':';
				}

				return buff;
			};

			constexpr auto view = Namespace.view( );
			if constexpr (view.ends_with("::"))
				return Namespace;
			else if constexpr (view.ends_with(':'))
				return add_dots(std::in_place_index<1>);
			else
				return add_dots(std::in_place_index<2>);
		}

		

		template <typename T, chars_cache ...DropNamespaces>
		_INLINE_VAR constexpr auto type_name_holder = []
		{
			constexpr auto sample = type_name_impl<T>( );
			if constexpr (sizeof...(DropNamespaces) == 0 || !std::_Has_class_or_enum_type<std::remove_cvref_t<remove_all_pointers_t<std::decay_t<T>>>>)
			{
				return chars_cache(sample);
			}
			else
			{
				constexpr auto buff    = remove_substrings_start<char, sample.size( ), fix_namespace_name<DropNamespaces>( )...>(sample);
				constexpr auto sample2 = remove_substring_end<remove_substring_get_real_size(buff)>(buff);
				return chars_cache(sample2);
			}
		}( );
	}

	template <typename T, chars_cache ...DropNamespaces>
	_INLINE_VAR constexpr auto type_name = detail::type_name_holder<T, DropNamespaces...>.view( );
}
