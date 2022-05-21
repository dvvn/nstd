#pragma once

#ifndef NSTD_LIB_FORMAT
#error NSTD_LIB_FORMAT not found
#else
#include <version>

#ifdef __cpp_lib_format
#include <format>
#define _FMT std
#else
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/xchar.h>
#define _FMT fmt
#endif

namespace nstd
{
    using _FMT::format;
    using _FMT::format_to;
    using _FMT::vformat;

    using _FMT::formatter;

    using _FMT::make_format_args;
    using _FMT::make_wformat_args;

} // namespace nstd

#undef _FMT
#endif
