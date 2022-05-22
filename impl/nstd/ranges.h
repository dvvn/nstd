#pragma once

#include <version>

#ifdef __cpp_lib_ranges
#include <ranges>
#define _RNG std::ranges
#else
#include <range/v3/all.hpp>
#define _RNG ::ranges
#endif

namespace nstd
{
    namespace ranges = _RNG;
    namespace views = _RNG::views;

} // namespace nstd

#undef _RNG
