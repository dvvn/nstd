#pragma once

#ifdef _DEBUG
#include "runtime_assert.h"
#define NSTD_WIDE(x) NSTD_CONCAT(L, x)

// ReSharper disable CppInconsistentNaming
#define runtime_assert(_ARG_, ...)\
	nstd::rt_assert_invoker(\
		nstd::detail::detect_msg<decltype(_ARG_)> ? false : !!(_ARG_),\
		nstd::detail::detect_msg<decltype(_ARG_)> ? (void*)nullptr : NSTD_WIDE(#_ARG_),\
		nstd::detail::expr_or_msg(NSTD_WIDE(#_ARG_), ##__VA_ARGS__),\
		NSTD_WIDE(__FILE__), NSTD_WIDE(__FUNCSIG__), __LINE__)

#else
#define runtime_assert(...) (void)0
#endif
// ReSharper restore CppInconsistentNaming
