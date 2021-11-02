#pragma once

//#include "root class.h"

#include "cache_base.h"
#include "nstd/mem/block.h"

// ReSharper disable CppInconsistentNaming
struct _IMAGE_SECTION_HEADER;
using IMAGE_SECTION_HEADER = _IMAGE_SECTION_HEADER;
// ReSharper restore CppInconsistentNaming

namespace nstd::os
{
	struct section_info
	{
		mem::block block;
		IMAGE_SECTION_HEADER* data = nullptr;
	};

	NSTD_OS_MODULE_INFO_CACHE_IMPL_H(sections_mgr, section_info);
}
