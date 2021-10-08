#pragma once

#include "data_cache.h"

namespace nstd::os
{
	class exports_storage: public detail::module_data_mgr<address>
	{
	protected:
		bool load_from_memory(cache_type& cache) override;
		bool load_from_file(cache_type& cache, detail::ptree_type&& storage) override;
		bool read_to_storage(const cache_type& cache, detail::ptree_type& storage) const override;
	};
}
