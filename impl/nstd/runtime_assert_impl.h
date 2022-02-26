#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include "core.h"

#include <memory>
#include <source_location>

import nstd.one_instance;

namespace nstd
{
	struct _declspec(novtable) rt_assert_handler
	{
		virtual ~rt_assert_handler( ) = default;

		virtual void handle(const char* expression, const char* message, const std::source_location& location) noexcept = 0;
		virtual void handle(const char* message, const std::source_location& location) noexcept = 0;

		virtual size_t id( ) const { return reinterpret_cast<size_t>(this); };
	};

	class rt_assert_handler_root final
	{
	public:
		~rt_assert_handler_root( );
		rt_assert_handler_root( );

		using handler_unique = std::unique_ptr<rt_assert_handler>;
		using handler_shared = std::shared_ptr<rt_assert_handler>;
		using handler_ptr = rt_assert_handler*;

		void add(handler_unique&& handler);
		void add(const handler_shared& handler);
		void add(handler_shared&&) = delete;
		void add(handler_ptr handler) ;

		void remove(size_t id);

		void handle(bool expression_result,
					const char* expression,
					const char* message = nullptr
					, const std::source_location& location = std::source_location::current( )) const noexcept;
		void handle(const char* message,
					const char* unused1 = nullptr,
					const char* unused2 = nullptr
					, const std::source_location& location = std::source_location::current( )) const noexcept;

	private:
		struct entry;
		struct impl;
		std::unique_ptr<impl> impl_;
	};

	using rt_assert_object = one_instance<rt_assert_handler_root>;
}

// ReSharper disable CppInconsistentNaming
#define runtime_assert_call(_EXPRESSION_OR_MESSAGE_,...) \
	nstd::rt_assert_object::get_ptr()->handle(_EXPRESSION_OR_MESSAGE_,NSTD_STRINGIZE(_EXPRESSION_OR_MESSAGE_),##__VA_ARGS__)

#define runtime_assert_add_handler_impl(_HANDLER_) ::nstd::rt_assert_object::get( ).add(_HANDLER_)
#define runtime_assert_remove_handler_impl(_HANDLER_) ::nstd::rt_assert_object::get( ).remove(_HANDLER_)
// ReSharper restore CppInconsistentNaming
