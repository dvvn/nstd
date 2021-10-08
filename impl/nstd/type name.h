// ReSharper disable CppRedundantInlineSpecifier
#pragma once
#include "chars cache.h"

namespace nstd
{
	namespace detail
	{
		template <typename T, chars_cache ...Ignore>
		constexpr std::string_view type_name_impl( )
		{
			if constexpr (sizeof...(Ignore) > 0)
			{
				static_assert((std::is_same_v<char, typename decltype(Ignore)::value_type> && ...));
				constexpr auto obj_name_fixed = []
				{
					auto name = type_name_impl<T>( );

					const auto do_ignore = [&](std::string_view str)
					{
						if (name.starts_with(str))
						{
							name.remove_prefix(str.size( ));
							if (name.starts_with("::"))
								name.remove_prefix(2);
						}
					};
					(do_ignore(Ignore.view( )), ...);

					return name;
				}( );
				return obj_name_fixed;
#if 0
			constexpr auto obj_name      = type_name<T>( );
			constexpr auto ignore_buffer = []
			{
				auto buff     = std::array<char, ((Ignore.view( ).size( ) + 2/*::*/) + ...) + 1/*\0*/>( );
				buff.back( )  = '\0';
				auto   names  = std::array{Ignore.view( )...};
				size_t offset = 0;
				for (std::string_view& str: names)
				{
					for (auto c: str)
						buff[offset++] = c;

					buff[offset++] = ':';
					buff[offset++] = ':';
				}
				return buff;
			}( );

			constexpr auto left_marker2 = std::string_view(ignore_buffer._Unchecked_begin( ), ignore_buffer.size( ) - 1);

			constexpr auto ret_val = [&]
			{
				const auto left_marker_index2 = obj_name.find(left_marker2);
				if (left_marker_index2 != std::string_view::npos)
					return obj_name.substr(left_marker_index2 + left_marker2.size( ));
				return obj_name;
			}( );
			return ret_val;
#endif
			}
			else
			{
				constexpr auto full_name         = std::string_view(__FUNCSIG__);
				constexpr auto left_marker       = std::string_view("type_name_impl<");
				constexpr auto right_marker      = std::string_view(">(");
				constexpr auto left_marker_index = full_name.find(left_marker);
				//static_assert(left_marker_index != std::string_view::npos);
				constexpr auto start_index = left_marker_index + left_marker.size( );
				constexpr auto end_index   = full_name.find(right_marker, left_marker_index);
				//static_assert(end_index != std::string_view::npos);
				constexpr auto length   = end_index - start_index;
				constexpr auto obj_name = [&]
				{
					auto name = full_name.substr(start_index, length);
					if constexpr (std::_Has_class_or_enum_type<T>)
					{
						//type_name<class X>
						constexpr std::string_view skip[] = {"struct", "class", "enum", "union"};
						for (auto& s: skip)
						{
							if (name.starts_with(s))
							{
								name.remove_prefix(s.size( ));
								//type_name<class X >
								if (name.starts_with(' '))
									name.remove_prefix(1);
								break;
							}
						}
					}
					//type_name<..., >
					while (name.ends_with(',') || name.ends_with(' '))
						name.remove_suffix(1);
					return name;
				}( );
				return obj_name;
			}
		}
	}

	template <typename T, chars_cache ...Ignore>
	_INLINE_VAR constexpr auto type_name = detail::type_name_impl<T, Ignore...>( );

	namespace detail
	{
		class drop_namespaces_impl
		{
			template <typename E, typename Tr>
			constexpr static size_t drop_count(const std::basic_string_view<E, Tr>& str)
			{
				const auto template_start = str.find('<');
				const auto str_tmp        = template_start != str.npos ? str.substr(0, template_start) : str;
				const auto namespace_size = str_tmp.rfind(':');

				return namespace_size == str_tmp.npos ? 0 : namespace_size + 1; //+1 to add second ':'
			}

		public:
			template <typename E, typename Tr, typename A>
			_CONSTEXPR20_CONTAINER auto operator()(std::basic_string<E, Tr, A>&& str) const
			{
				auto drop = drop_count(str);
				if (drop > 0)
					str.erase((0), drop);
				return std::move(str);
			}

			template <typename E, typename Tr, typename A>
			_CONSTEXPR20_CONTAINER auto operator()(const std::basic_string<E, Tr, A>& str) const
			{
				return std::invoke(*this, std::basic_string_view<E, Tr>(str));
			}

			template <typename E, typename Tr>
			constexpr auto operator()(std::basic_string_view<E, Tr> str) const
			{
				auto drop = drop_count(str);
				if (drop > 0)
					str.remove_prefix(drop);
				return str;
			}
		};
	}

	_INLINE_VAR constexpr auto drop_namespaces = detail::drop_namespaces_impl( );

#if 0
	template <typename E, typename Tr>
	constexpr auto drop_namespaces(const std::basic_string_view<E, Tr>& str)
	{
		const auto template_start = str.find('<');
		const auto str_tmp        = template_start != str.npos ? str.substr(0, template_start) : str;
		const auto namespace_size = str_tmp.rfind(':');

		auto copy = str;

		if (namespace_size != str_tmp.npos)
			copy.remove_prefix(namespace_size + 1); //+1 to add last ':'

		return copy;
	}
#endif
}
