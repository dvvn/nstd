#pragma once
#include <array>
#include <string_view>

namespace nstd
{
	//use it for as template parameter

	template <std::size_t N, typename Chr>
	struct chars_cache
	{
		std::array<Chr, N> cache;

		using value_type = Chr;
		using size_type = std::size_t;

		template <size_type...Is>
		constexpr chars_cache(const Chr (&arr)[N], std::index_sequence<Is...>)
			: cache{arr[Is]...}
		{
		}

		constexpr chars_cache(const Chr (&arr)[N])
			: chars_cache(arr, std::make_index_sequence<N>( ))
		{
		}

		constexpr bool empty( ) const
		{
			return cache.front( ) == static_cast<Chr>('\0');
		}

		static constexpr size_type size = N;

		constexpr std::basic_string_view<Chr> view( ) const
		{
			return {cache._Unchecked_begin( ), N - 1};
		}
	};
}
