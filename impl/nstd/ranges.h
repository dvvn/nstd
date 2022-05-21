#pragma once

#ifndef NSTD_LIB_RANGES
#error NSTD_LIB_RANGES not found
#else
#include <version>

#ifdef __cpp_lib_ranges
#include <ranges>
#else
#include <range/v3/all.hpp>
#endif

namespace nstd
{
#ifdef __cpp_lib_ranges
    namespace ranges = std::ranges;
    namespace views = std::ranges::views;
#else
    namespace ranges = ::ranges;
    namespace views = ::ranges::views;
#endif
} // namespace nstd

#endif
