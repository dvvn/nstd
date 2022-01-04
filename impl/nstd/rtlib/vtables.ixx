module;

#include "nstd/mem/address_includes.h"
#include "cache_includes.h"

export module nstd.rtlib:vtables;
export import :cache;
export import nstd.mem;

export namespace nstd::rtlib
{
	struct vtables_data 
	{
		mem::address addr;
	};

	struct vtables :cache<vtables_data>
	{
	protected:
		create_result create(const key_type& entry) override;
	};
}
