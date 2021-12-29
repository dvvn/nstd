#pragma once

#include <type_traits>

#if defined(__cpp_lib_format)
#include <format>
#elif __has_include(<fmt/format.h>)
#include <fmt/format.h>
namespace std
{
	using namespace fmt;
}
#endif