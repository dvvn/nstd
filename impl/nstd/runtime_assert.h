#pragma once

#ifdef _DEBUG
#include <nstd/runtime_assert_impl.h>
#define runtime_assert runtime_assert_call
#define runtime_assert_add_handler runtime_assert_add_handler_impl
#define runtime_assert_remove_handler runtime_assert_remove_handler_impl
#define runtime_assert_unreachable runtime_assert_call
#else
#include <nstd/core_utils.h>
#define runtime_assert(...) (void)0
#define runtime_assert_add_handler(...) (void)0
#define runtime_assert_remove_handler(...) (void)0
#define runtime_assert_unreachable(...) nstd::unreachable()
#endif
