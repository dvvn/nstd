#pragma once

#ifdef NSTD_CUSTOM_UNORDERED_SET
#include <robin_hood.h>
#else
#include <unordered_set>
#endif

namespace nstd::containers
{
#ifdef NSTD_CUSTOM_UNORDERED_SET

	using robin_hood::unordered_set;
	using robin_hood::hash;
	using robin_hood::swap;
#else
	using std::unordered_set;
	using std::hash;
	using std::swap;
#endif
}
