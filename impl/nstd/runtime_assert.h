#pragma once
// ReSharper disable CppInconsistentNaming

#if 0
//overcomplicated
#define _runtime_assert_first_argument_impl(_MACRO_,_ARG1_,...) _MACRO_##_ARG1_
#define _runtime_assert_first_argument(_MACRO_,_DEFAULT_,...) _runtime_assert_first_argument_impl(_MACRO_,##__VA_ARGS__,_DEFAULT_)

#define _runtime_assert_add_handler_1(_HANDLER_) runtime_assert_add_handler_impl(_HANDLER_)
#define _runtime_assert_add_handler_0(...) (void)0
#define runtime_assert_add_handler(_HANDLER_,...) \
	_runtime_assert_first_argument(_runtime_assert_add_handler_,_DEBUG,__VA_ARGS__)(_HANDLER_)
#endif

#ifdef _DEBUG
#include "runtime_assert_impl.h"
#define runtime_assert runtime_assert_call
#define runtime_assert_add_handler runtime_assert_add_handler_impl
#define runtime_assert_remove_handler runtime_assert_remove_handler_impl
#else
#define runtime_assert(...) (void)0
#define runtime_assert_add_handler(...) (void)0
#define runtime_assert_remove_handler(...) (void)0
#endif

// ReSharper restore CppInconsistentNaming
