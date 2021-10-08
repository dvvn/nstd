#include "runtime assert.h"

#include <algorithm>
#include <ranges>
#include <vector>

using namespace nstd;

void rt_assert_handler::handle(bool result, rt_assert_arg_t&& expression, rt_assert_arg_t&& message, rt_assert_arg_t&& file_name, rt_assert_arg_t&& function, unsigned __int64 line) noexcept
{
	if (result == true)
		return;
	this->handle_impl(expression, message, {std::move(file_name), std::move(function), line});
}

struct rt_assert_handler_ex::data_type: std::vector<element_type>
{
};

rt_assert_handler_ex::rt_assert_handler_ex( )
{
	data_ = std::make_unique<data_type>( );
}

rt_assert_handler_ex::~rt_assert_handler_ex( ) = default;

rt_assert_handler_ex::element_type::~element_type( )
{
	if (allocated_)
		delete handle_;
}

rt_assert_handler_ex::element_type::element_type(element_type&& other) noexcept
{
	handle_          = other.handle_;
	allocated_       = other.allocated_;
	other.allocated_ = false;
}

rt_assert_handler_ex::element_type& rt_assert_handler_ex::element_type::operator=(element_type&& other) noexcept
{
	std::swap(handle_, other.handle_);
	std::swap(allocated_, other.allocated_);

	return *this;
}

rt_assert_handler_ex::element_type::element_type(rt_assert_handler* handle, bool allocated)
	: handle_(handle), allocated_(allocated)
{
}

bool rt_assert_handler_ex::element_type::operator==(const rt_assert_handler* other) const
{
	return handle_ == other;
}

rt_assert_handler* rt_assert_handler_ex::element_type::operator->( ) const
{
	return handle_;
}

//rt_assert_handler_ex::rt_assert_handler_ex(rt_assert_handler_ex&& other) noexcept
//{
//	*this = std::move(other);
//}
//
//rt_assert_handler_ex& rt_assert_handler_ex::operator=(rt_assert_handler_ex&& other) noexcept
//{
//	std::swap(data_, other.data_);
//	return *this;
//}

// ReSharper disable once CppMemberFunctionMayBeConst
void rt_assert_handler_ex::add(rt_assert_handler* handler, bool allocated)
{
	data_->push_back({handler, allocated});
}

void rt_assert_handler_ex::remove(const rt_assert_handler* handler)
{
	auto pos = std::ranges::remove_if(*data_, [=](const element_type& el) { return el == handler; });
	data_->erase(pos.begin( ), pos.end( ));
}

void rt_assert_handler_ex::handle_impl(const rt_assert_arg_t& expression, const rt_assert_arg_t& message, const info_type& info) noexcept
{
	for (const auto& elem: *data_)
		elem->handle_impl(expression, message, info);
}
