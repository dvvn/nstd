#pragma once
#include "type_traits.h"

#include <algorithm>
#include <string>
#include <array>

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
			str.erase(pos, substr.size());
		}
#else
		auto itr = str.begin();
		for (;;)
		{
			itr = std::search(itr, str.end(), substr.begin(), substr.end());
			if (itr == str.end())
				return;
			itr = str.erase(itr, itr + substr.size());
		}
#endif
	}

	template<typename Chr, size_t Size>
	struct buffered_string
	{
		using array_type = std::array<Chr, Size>;

		array_type str;
		size_t str_size;

		constexpr buffered_string(const array_type& arr, size_t size) :str(arr), str_size(size) {}

		constexpr bool ideal() const { return str.size() - 1 == str_size; }
		constexpr auto view() const { return std::basic_string_view<Chr>(str._Unchecked_begin(), str_size); }
	};

	template<size_t StrSize, typename  ...Args>
	constexpr auto string_to_buffer(Args&& ...args)
	{
		auto to = std::array<char, StrSize + 1>();
		to.back() = 0;
		auto from = std::string_view(args...);
		std::ranges::copy(from, to.begin());
		return buffered_string(to, from.size());
	}

	namespace detail
	{
		template <typename T>
		constexpr decltype(auto) type_name_impl0() { return __FUNCSIG__; }

		template <typename T>
		constexpr auto type_name_impl()
		{
			constexpr std::string_view n0 = type_name_impl0<T>();

			constexpr auto start = n0.find('<') + 1;
			constexpr auto end = n0.rfind('>');
			constexpr auto name_size = end - start;
			const auto name_sv = n0.substr(start, name_size);
			auto name = std::string(name_sv);

			erase_substring(name, "struct ");
			erase_substring(name, "class ");
			erase_substring(name, "enum ");
			erase_substring(name, "union ");

			return  string_to_buffer<name_size>(name);
		}

		template <typename T>
		_INLINE_VAR constexpr auto type_name_holder = []
		{
			constexpr auto tmp = type_name_impl<T>();
			if constexpr (tmp.ideal())
				return tmp;
			else
				return string_to_buffer<tmp.str_size>(tmp.view());
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
				if (drop.back() == ':')//skip empty check
					drop_str += ':';
				else
					drop_str += "::";
				drop_sv = drop_str;
			}

			erase_substring(str, drop_sv);
		}
	}

	template <typename T>
	_INLINE_VAR constexpr auto type_name = detail::type_name_holder<T>.view();

	template<typename Str>
	constexpr auto drop_namespace(Str&& in, const std::string_view& drop)
	{
		auto out = std::string(std::forward<Str>(in));
		detail::drop_namespace_impl(out, drop);
		return out;
	}
}
