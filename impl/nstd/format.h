#pragma once

#ifndef NSTD_CUSTOM_FORMAT
#include <format>
#else
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/ranges.h>
#include <fmt/ostream.h>
namespace std
{
	using namespace fmt;
}
#endif