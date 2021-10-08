#pragma once

#include "data_cache.h"
#include "sections_storage.h"

namespace std
{
	class mutex;
}

namespace nstd::os
{
	struct vtable_info
	{
		address addr;
	};

	class vtables_storage: public detail::module_data_mgr<vtable_info>
	{
	public:
		vtables_storage( );

		void lock( );
		void unlock( );

	protected:
		bool load_from_memory(cache_type& cache) override;
		bool load_from_file(cache_type& cache, detail::ptree_type&& storage) override;
		bool read_to_storage(const cache_type& cache, detail::ptree_type& storage) const override;

	private:
		sections_storage& derived_sections( );
		memory_block      derived_mem_block( ) const;

		std::shared_ptr<std::mutex> lock_;
	};
}
