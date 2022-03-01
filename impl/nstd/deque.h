#pragma once

#if __has_include(<veque.hpp>)
#include <cstddef>
namespace std
{
	using size_t = ::size_t;
	using ptrdiff_t = ::ptrdiff_t;
}
#include <string>
#include <veque.hpp>
#else
#include <deque>
#endif

namespace nstd
{
#if __has_include(<veque.hpp>)
	template< typename T, typename Allocator = std::allocator<T> >
	using deque = ::veque::veque<T, ::veque::fast_resize_traits, Allocator>;
#else
	using std::deque;
#endif
}
