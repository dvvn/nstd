#include "runtime_assert.h"

#include "overload.h"

#include <algorithm>
#include <stdexcept>
#include <variant>
#include <vector>

using namespace nstd;

using r = rt_assert_handler_root;

static constexpr auto _Get_handler_ptr =
		overload([](const r::handler_shared& h) { return h.get( ); }
			   , [](const r::handler_unique& h) { return h.get( ); }
			   , [](const r::handler_ref& h) { return std::addressof(h.get( )); }
			   , [](rt_assert_handler* const h) { return h; }
			   , [](rt_assert_handler& h) { return std::addressof(h); });

struct root_handler_element
{
	rt_assert_handler* get( ) const
	{
		return std::visit(_Get_handler_ptr, data_);
	}

	rt_assert_handler* operator->( ) const
	{
		return get( );
	}

	template <typename T>
	root_handler_element(T&& arg)
		: data_(std::forward<T>(arg))
	{
	}

private:
	std::variant<r::handler_unique, r::handler_shared, r::handler_ref> data_;
};

//void log(const std::string_view message,
//	const std::source_location location =
//	std::source_location::current())
//{
//	std::cout << "file: "
//		<< location.file_name() << "("
//		<< location.line() << ":"
//		<< location.column() << ") `"
//		<< location.function_name() << "`: "
//		<< message << '\n';
//}

struct rt_assert_handler_root::data_type : std::vector<root_handler_element>
{
};

rt_assert_handler_root::rt_assert_handler_root( )
{
	data_ = std::make_unique<data_type>( );
}

rt_assert_handler_root::~rt_assert_handler_root( ) = default;

template <typename T, typename S>
static void _Validate_id(T&& h, S&& data)
{
	if (data.empty( ))
		return;
	const size_t id = _Get_handler_ptr(h)->id( );
	for (const root_handler_element& el: data)
	{
		if (el->id( ) == id)
			throw std::logic_error("Handler with given id already exists!");
	}
}

void rt_assert_handler_root::add(handler_unique&& handler) const
{
	auto& d = *data_;
	_Validate_id(handler, d);
	d.push_back(std::move(handler));
}

void rt_assert_handler_root::add(const handler_shared& handler) const
{
	auto& d = *data_;
	_Validate_id(handler, d);
	d.push_back(handler);
}

void rt_assert_handler_root::add(const handler_ref& handler) const
{
	auto& d = *data_;
	_Validate_id(handler, d);
	d.push_back(handler);
}

void rt_assert_handler_root::add(rt_assert_handler* handler) const
{
#if _HAS_CXX20
	add(reinterpret_cast<handler_ref&>(handler));
#else
	add(std::ref(*handler));
#endif
}

void rt_assert_handler_root::remove(size_t id) const
{
	auto& d = *data_;
	for (auto itr = d.begin( ); itr != d.end( ); ++itr)
	{
		if (id == (*itr)->id( ))
		{
			d.erase(itr);
			break;
		}
	}
}

void rt_assert_handler_root::remove(const rt_assert_handler* handler) const
{
	remove(handler->id( ));
}

void rt_assert_handler_root::handle(bool expression_result, const char* expression, const char* message, const std::source_location& location) const noexcept
{
	for (const auto& el: *data_)
		el->handle(expression_result, expression, message, location);
}

void rt_assert_handler_root::handle(const char* message,
									[[maybe_unused]] const char* unused1, [[maybe_unused]] const char* unused2
								  , const std::source_location& location) const noexcept
{
	for (const auto& el: *data_)
		el->handle(message, location);
}