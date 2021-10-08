#pragma once

#include "data_cache.h"

#include "nstd/memory block.h"

// ReSharper disable CppInconsistentNaming
struct _IMAGE_SECTION_HEADER;
using IMAGE_SECTION_HEADER = _IMAGE_SECTION_HEADER;
// ReSharper restore CppInconsistentNaming

namespace nstd::os
{
	struct section_info
	{
		memory_block          block;
		IMAGE_SECTION_HEADER* data = nullptr;
	};

	class sections_storage: public detail::module_data_mgr<section_info>
	{
	protected:
		bool load_from_memory(cache_type& cache) override;
		bool load_from_file(cache_type& cache, detail::ptree_type&& storage) override;
		bool read_to_storage(const cache_type& cache, detail::ptree_type& storage) const override;
	};
}
