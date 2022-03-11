#pragma once

#include <type_traits>

#if defined(__cpp_lib_format)
#include <format>
//todo :formatter for ranges
#elif __has_include(<fmt/format.h>)
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/ranges.h>
#include <fmt/ostream.h>
namespace std
{
	using namespace fmt;
}
#endif