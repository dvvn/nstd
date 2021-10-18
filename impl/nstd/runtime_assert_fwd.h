#pragma once
// ReSharper disable CppInconsistentNaming

#ifdef _DEBUG
#include "runtime_assert.h"
#define runtime_assert runtime_assert_call
#define runtime_assert_add_handler runtime_assert_add_handler_impl
#define runtime_assert_remove_handler runtime_assert_remove_handler_impl
#else
#define runtime_assert(...) (void)0
#define runtime_assert_add_handler(...) (void)0
#define runtime_assert_remove_handler(...) (void)0
#endif

// ReSharper restore CppInconsistentNaming
