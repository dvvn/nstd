#include "runtime_assert_impl.h"
#include "overload.h"

#include <algorithm>
#include <stdexcept>
#include <variant>
#include <vector>
#undef NDEBUG
#include <assert.h>

using namespace nstd;

static void _Assert(const char* expression, const char* message, const std::source_location& location) noexcept
{
	constexpr auto make_wstring = [](const char* str)
	{
		return std::wstring(str, str + std::char_traits<char>::length(str));
	};

	std::wstring msg;
	if (!expression)
	{
		msg = make_wstring(message);
	}
	else if (!message)
	{
		msg = make_wstring(expression);
	}
	else
	{
		msg += make_wstring(message);
		msg += L" (";
		msg += make_wstring(expression);
		msg += L')';
	}

	return _wassert(msg.c_str( ), make_wstring(location.file_name( )).c_str( ), location.line( ));
}

struct default_assert_handler final : rt_assert_handler
{
	void handle(bool expression_result, const char* expression, const char* message, const std::source_location& location) noexcept override
	{
		if (expression_result)
			return;
		_Assert(expression, message, location);
	}

	void handle(const char* message, const std::source_location& location) noexcept override
	{
		_Assert(nullptr, message, location);
	}
};

static constexpr auto _Get_handler_ptr =
		overload([](const rt_assert_handler_root::handler_shared& h) { return h.get( ); }
			   , [](const rt_assert_handler_root::handler_unique& h) { return h.get( ); }
			   , [](const rt_assert_handler_root::handler_ref& h) { return std::addressof(h.get( )); }
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
	std::variant<rt_assert_handler_root::handler_unique, rt_assert_handler_root::handler_shared, rt_assert_handler_root::handler_ref> data_;
};

struct rt_assert_handler_root::data_type : std::vector<root_handler_element>
{
};

template <typename T>
static auto _Add_wrapper(rt_assert_handler_root* _this, T obj)
{
	_this->add(std::forward<T>(obj));
}

rt_assert_handler_root::rt_assert_handler_root( )
{
	data_ = std::make_unique<data_type>( );
	this->add(std::make_unique<default_assert_handler>( ));
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
#ifdef _DEBUG
	_Validate_id(handler, d);
#endif
	d.push_back(std::move(handler));
}

//void rt_assert_handler_root::add(const handler_shared& handler) const
//{
//	auto& d = *data_;
//	_Validate_id(handler, d);
//	d.push_back(handler);
//}

void rt_assert_handler_root::add(const handler_ref& handler) const
{
	auto& d = *data_;
#ifdef _DEBUG
	_Validate_id(handler, d);
#endif
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
