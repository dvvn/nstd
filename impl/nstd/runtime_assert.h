#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include "core.h"
#include "one_instance.h"

#include <source_location>

namespace nstd
{
	struct _declspec(novtable) rt_assert_handler
	{
		virtual ~rt_assert_handler( ) = default;

		virtual void handle(const std::source_location& location, bool expression_result, const char* expression, const char* message) _NOEXCEPT_FNPTR =0;
		virtual void handle(const std::source_location& location, const char* message) _NOEXCEPT_FNPTR =0;

		virtual size_t id( ) const = 0;
	};

	class rt_assert_handler_root final
	{
	public:
		~rt_assert_handler_root( );
		rt_assert_handler_root( );

		using handler_unique = std::unique_ptr<rt_assert_handler>;
		using handler_shared = std::shared_ptr<rt_assert_handler>;
		using handler_ref = std::reference_wrapper<rt_assert_handler>;

		void add(handler_unique&& handler);

		void add(const handler_shared& handler);

		void add(const handler_ref& handler);
		void add(rt_assert_handler* handler);

		void remove(size_t id);
		void remove(rt_assert_handler* handler);

		void handle(const std::source_location& location, bool expression_result, const char* expression, const char* message = 0) _NOEXCEPT_FNPTR;
		void handle(const std::source_location& location, const char* message, const char* unused1 = 0, const char* unused2 = 0) _NOEXCEPT_FNPTR;

	private:
		struct data_type;
		std::unique_ptr<data_type> data_;
	};

	using rt_assert_object = one_instance<rt_assert_handler_root>;
}

// ReSharper disable CppInconsistentNaming
#define runtime_assert_call(_EXPRESSION_OR_MESSAGE_,...) \
	nstd::rt_assert_object::get_ptr()->handle(std::source_location::current( ),_EXPRESSION_OR_MESSAGE_,_STRINGIZE(_EXPRESSION_OR_MESSAGE_),##__VA_ARGS__)
#define runtime_assert_add_handler_impl(_HANDLER_) nstd::rt_assert_object::get_ptr()->add(_HANDLER_)
#define runtime_assert_remove_handler_impl(_HANDLER_) nstd::rt_assert_object::get_ptr()->remove(_HANDLER_)
// ReSharper restore CppInconsistentNaming
