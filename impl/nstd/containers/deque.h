#pragma once

#if __has_include(<veque.hpp>)
#include <veque.hpp>
#define NSTD_CONTAINERS_DEQUE
#else
#include <deque>
#endif

namespace nstd::containers
{
#ifdef NSTD_CONTAINERS_DEQUE
	template <typename T, typename Allocator = std::allocator<T>>
	using deque = ::veque::veque<T, ::veque::fast_resize_traits, Allocator>;
#else
	using std::deque;
#endif
}
