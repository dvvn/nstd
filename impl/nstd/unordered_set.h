#pragma once

#if __has_include(<robin_hood.h>)
#include <robin_hood.h>
#else
#include <unordered_set>
#endif

namespace nstd
{
#if __has_include(<robin_hood.h>)
	using robin_hood::unordered_set;
	using robin_hood::hash;
	using robin_hood::swap;
#else
	using std::unordered_set;
	using std::hash;
	using std::swap;
#endif
}
