#pragma once

#if __has_include(<veque.hpp>)
#include <string>
#include <veque.hpp>
#ifdef __cpp_modules
namespace std
{
	using size_t = ::size_t;
	using ptrdiff_t = ::ptrdiff_t;
}
#endif

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
