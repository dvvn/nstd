#pragma once

#ifndef NSTD_CUSTOM_RANGES
#include <ranges>
#else
#include <range/v3/all.hpp>
namespace std
{
	using namespace ::ranges;
	using namespace ::ranges::views;
}
#endif
