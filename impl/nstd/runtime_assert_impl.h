#pragma once

#include <nstd/core_utils.h>

#include <memory>
#include <source_location>

namespace nstd
{
    struct _declspec(novtable) rt_assert_handler
    {
        virtual ~rt_assert_handler() = default;

        virtual void handle(const char* expression, const char* message, const std::source_location& location) = 0;
        virtual void handle(const char* message, const std::source_location& location) = 0;
    };

    void _Rt_assert_add(rt_assert_handler* const handler);
    void _Rt_assert_remove(rt_assert_handler* const handler);

    constexpr bool _Rt_assert_can_invoke(const char*)
    {
        return true;
    }

    constexpr bool _Rt_assert_can_invoke(const bool val)
    {
        return val == false;
    }

    [[noreturn]] void _Rt_assert_invoke(const char* expression, const char* message = nullptr, const std::source_location& location = std::source_location::current());

} // namespace nstd

// ReSharper disable CppInconsistentNaming
#define runtime_assert_call(_EXPRESSION_OR_MESSAGE_, ...)                                    \
    {                                                                                        \
        if (nstd::_Rt_assert_can_invoke(_EXPRESSION_OR_MESSAGE_))                            \
            nstd::_Rt_assert_invoke(NSTD_STRINGIZE(_EXPRESSION_OR_MESSAGE_), ##__VA_ARGS__); \
    }

#define runtime_assert_add_handler_impl(_HANDLER_) nstd::_Rt_assert_add(_HANDLER_)
#define runtime_assert_remove_handler_impl(_HANDLER_) nstd::_Rt_assert_remove(_HANDLER_)
  // ReSharper restore CppInconsistentNaming
