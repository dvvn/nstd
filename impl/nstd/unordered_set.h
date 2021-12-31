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
#else
	using std::unordered_set;
	using std::hash;
#endif
}
