#pragma once

#ifndef NSTD_CUSTOM_DEQUE
#include <veque.hpp>
#else
#include <deque>
#endif

namespace nstd::containers
{
#ifdef NSTD_CUSTOM_DEQUE
	template <typename T, typename Allocator = std::allocator<T>>
	using deque = ::veque::veque<T, ::veque::fast_resize_traits, Allocator>;
#else
	using std::deque;
#endif
}
