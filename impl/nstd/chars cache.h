#pragma once
#include <array>
#include <string_view>

namespace nstd
{
	//use it for as template parameter

	struct chars_cache_tag
	{
	};

	template <std::size_t Size, typename Chr>
	struct chars_cache : chars_cache_tag
	{
		std::array<Chr, Size> cache;

		using value_type = Chr;
		using view_type = std::basic_string_view<Chr>;

		static constexpr size_t size = Size;

		template <size_t...Idx>
		constexpr chars_cache(const Chr* arr, std::index_sequence<Idx...>)
			: cache{ arr[Idx]... }
		{
		}

		template <size_t...Idx>
		constexpr chars_cache(const Chr* arr)
			: chars_cache((arr), std::make_index_sequence<Size>())
		{
		}

		constexpr chars_cache(const Chr(&arr)[Size])
			: chars_cache((arr), std::make_index_sequence<Size>())
		{
		}

		template <size_t...Idx>
		constexpr chars_cache(const view_type& view, std::index_sequence<Idx...>)
			: cache{ Idx < view.size() ? view[Idx] : static_cast<Chr>('\0')... }
		{
			if (view.size() >= Size)
				throw std::_Xruntime_error("incorrect string_view size!");
		}

		constexpr chars_cache(const view_type& view)
			: chars_cache(view, std::make_index_sequence<Size>())
		{
		}

		constexpr chars_cache(const std::array<Chr, Size>& arr)
			: chars_cache(arr._Unchecked_begin(), std::make_index_sequence<Size>())
		{
		}

		template <size_t Size2>
			requires(Size2 < Size)
		constexpr chars_cache(const std::array<Chr, Size2>& arr)
			: chars_cache(arr._Unchecked_begin(), std::make_index_sequence<Size2>())
		{
			if (arr.back() != 0)
				cache[Size2] = '\0';
		}

		template <size_t Size2>
			requires(Size2 < Size)
		constexpr chars_cache(const chars_cache<Size2, Chr>& arr)
			: chars_cache(arr.cache)
		{
		}

		constexpr bool empty() const
		{
			return cache.front() == static_cast<Chr>('\0');
		}

		constexpr view_type view() const
		{
			return { cache._Unchecked_begin(), (size - 1) };
		}
	};
}
