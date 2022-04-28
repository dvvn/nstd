#pragma once

#include <vector>

namespace nstd
{
	template<typename T, class Base = std::vector<T>>
	struct private_vector :protected Base
	{
		private_vector( )
			:Base( )
		{
		}

		using Base::begin;
		using Base::end;
		using Base::empty;
		using Base::size;

		using Base::value_type;
	};
}