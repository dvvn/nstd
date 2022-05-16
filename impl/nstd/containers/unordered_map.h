#pragma once

#ifdef NSTD_CUSTOM_UNORDERED_MAP
#include <robin_hood.h>
#else
#include <unordered_map>
#endif

namespace nstd::containers
{
#ifdef NSTD_CUSTOM_UNORDERED_MAP

	using robin_hood::unordered_map;

	using robin_hood::hash;
	using robin_hood::swap;
#else
	using std::unordered_map;
	using std::hash;
	using std::swap;
#endif
}
