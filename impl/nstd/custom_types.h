#pragma once

#if __has_include(<robin_hood.h>)
#define NSTD_UNORDERED_MAP robin_hood::unordered_map
#define NSTD_UNORDERED_SET robin_hood::unordered_set
#define NSTD_UNORDERED_MAP_INCLUDE <robin_hood.h>
#define NSTD_UNORDERED_SET_INCLUDE <robin_hood.h>
#define NSTD_UNORDERED_HASH robin_hood::hash
#define NSTD_UNORDERED_TRANSPARENT_GAP ,robin_hood::is_transparent_tag{}
#else
#define NSTD_UNORDERED_MAP std::unordered_map
#define NSTD_UNORDERED_SET std::unordered_set
#define NSTD_UNORDERED_MAP_INCLUDE <unordered_map>
#define NSTD_UNORDERED_SET_INCLUDE <unordered_set>
#define NSTD_UNORDERED_HASH std::hash
#define NSTD_UNORDERED_TRANSPARENT_GAP
#endif

