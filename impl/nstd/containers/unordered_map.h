#pragma once

#if __has_include(<robin_hood.h>)
#include <robin_hood.h>
#define NSTD_CONTAINERS_UNORDERED_MAP
#else
#include <unordered_map>
#endif

namespace nstd
{
#ifdef NSTD_CONTAINERS_UNORDERED_MAP
	namespace containers
	{
		using robin_hood::unordered_map;
	}
	using robin_hood::hash;
	using robin_hood::swap;
#else
	namespace containers
	{
		using std::unordered_map
	};
	using std::hash;
	using std::swap;
#endif
}
