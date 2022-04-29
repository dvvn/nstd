#pragma once

#include <vector>

namespace nstd
{
	template<typename T, class Alloc = std::allocator<T>, class Base = std::vector<T, Alloc>>
	struct private_vector :protected Base
	{
		private_vector( )
			:Base( )
		{
		}

		using Base::begin;
		using Base::end;
		using Base::cbegin;
		using Base::cend;
		using Base::rbegin;
		using Base::rend;
		using Base::data;
		using Base::operator[];
		using Base::empty;
		using Base::size;

		using Base::value_type;
	};
}