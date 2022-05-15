#pragma once

#include <version>

#ifdef __cpp_lib_ranges
#include <ranges>
#else
#include <range/v3/all.hpp>
namespace std
{
	using namespace ::ranges;
	using namespace ::ranges::views;
}
#endif
