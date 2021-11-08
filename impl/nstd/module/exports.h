#pragma once

#include "cache_base.h"
#include "nstd/address.h"

namespace nstd::module
{
	NSTD_OS_MODULE_INFO_CACHE_IMPL_DATA(export)
	{
		address addr;
	};

	NSTD_OS_MODULE_INFO_CACHE_IMPL_H(export);
}