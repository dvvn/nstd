#pragma once

#include "cache_base.h"
#include "nstd/address.h"

#include <memory>
#include <string>

namespace nstd::os
{
	struct export_info
	{
		address addr;
	};

	NSTD_OS_MODULE_INFO_CACHE_IMPL_H(exports_mgr, export_info);
}
