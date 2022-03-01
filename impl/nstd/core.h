#pragma once

#ifndef _CONCAT
#define NSTD_CONCATX(x, y) x##y
#define NSTD_CONCAT(x, y)  NSTD_CONCATX(x, y)
#else
#define NSTD_CONCAT _CONCAT
#endif

#ifndef _STRINGIZE
#define NSTD_STRINGIZEX(x)  #x
#define NSTD_STRINGIZE(x)   NSTD_STRINGIZEX(x)
#else
#define NSTD_STRINGIZE _STRINGIZE
#endif

#ifndef _CRT_WIDE
#define NSTD_STRINGIZE_WIDE(x) NSTD_CONCAT(L,NSTD_STRINGIZE(x))
#else
#define NSTD_STRINGIZE_WIDE _CRT_WIDE
#endif

#define NSTD_STRINGIZE_RAW(x) NSTD_CONCAT(R,NSTD_STRINGIZE(##(x)##))
#define NSTD_STRINGIZE_RAW_WIDE(x) NSTD_CONCAT(L,NSTD_STRINGIZE_RAW(x))

