#pragma once

#ifdef __cpp_lib_ranges
#include <ranges>
#else
#include <cstddef>
namespace std
{
	using ::size_t;
}
#include <range/v3/all.hpp>
namespace std
{
	using namespace ::ranges;
	using namespace ::ranges::views;
}
#endif
