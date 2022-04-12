#pragma once

#include <nstd/core.h>

#include <memory>
#include <source_location>

#ifdef _DEBUG
#define runtime_assert_noexcept
#else
#define runtime_assert_noexcept noexcept
#endif

namespace nstd
{
	struct _declspec(novtable) rt_assert_handler
	{
		using unique = std::unique_ptr<rt_assert_handler>;
		using shared = std::shared_ptr<rt_assert_handler>;
		using raw = rt_assert_handler*;

		virtual ~rt_assert_handler( ) = default;

		virtual void handle(const char* expression, const char* message, const std::source_location& location) noexcept = 0;
		virtual void handle(const char* message, const std::source_location& location) noexcept = 0;

		virtual size_t id( ) const noexcept { return reinterpret_cast<size_t>(this); };
	};

	void _Rt_assert_add(rt_assert_handler::unique&& handler) runtime_assert_noexcept;
	void _Rt_assert_add(const rt_assert_handler::shared& handler) runtime_assert_noexcept;
	void _Rt_assert_add(rt_assert_handler::raw handler) runtime_assert_noexcept;

	void _Rt_assert_remove(const size_t id) noexcept;

	void _Rt_assert_handle(bool expression_result,
						   const char* expression,
						   const char* message = nullptr
						   , const std::source_location& location = std::source_location::current( )) noexcept;
	[[noreturn]]
	void _Rt_assert_handle(const char* message,
						   const char* unused1 = nullptr,
						   const char* unused2 = nullptr
						   , const std::source_location& location = std::source_location::current( )) noexcept;
}

// ReSharper disable CppInconsistentNaming
#define runtime_assert_call(_EXPRESSION_OR_MESSAGE_,...) \
	nstd::_Rt_assert_handle(_EXPRESSION_OR_MESSAGE_,NSTD_STRINGIZE(_EXPRESSION_OR_MESSAGE_),##__VA_ARGS__)

#define runtime_assert_add_handler_impl(_HANDLER_) nstd::_Rt_assert_add(_HANDLER_)
#define runtime_assert_remove_handler_impl(_ID_) nstd::_Rt_assert_remove(_ID_)
// ReSharper restore CppInconsistentNaming
