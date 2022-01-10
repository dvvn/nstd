#pragma once

#if __has_include(<robin_hood.h>)
#include <robin_hood.h>
#else
#include <unordered_map>
#endif

namespace nstd
{
#if __has_include(<robin_hood.h>)
	using robin_hood::unordered_map;
	using robin_hood::hash;
	using robin_hood::swap;
#else
	using std::unordered_map;
	using std::hash;
	using std::swap;
#endif
}
