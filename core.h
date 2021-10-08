#pragma once

#ifndef _STRINGIZE
#define NSTD_STRINGIZEX(x)  #x
#define NSTD_STRINGIZE(x)   NSTD_STRINGIZEX(x)
#else
#define NSTD_STRINGIZE _STRINGIZE
#endif

#define NSTD_STRINGIZE_RAW(x) NSTD_CONCAT(R,NSTD_STRINGIZE(##(x)##))

#ifndef _CONCAT
#define NSTD_CONCATX(x, y) x##y
#define NSTD_CONCAT(x, y)  NSTD_CONCATX(x, y)
#else
#define NSTD_CONCAT(x, y) _CONCAT(x, y)
#endif