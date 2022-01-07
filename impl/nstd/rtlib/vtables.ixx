module;

#include "nstd/mem/address_includes.h"
#include "cache_includes.h"

export module nstd.rtlib:vtables;
export import :cache;
export import nstd.mem;

export namespace nstd::rtlib
{
	struct vtable_data 
	{
		mem::address addr;
	};

	struct vtables :cache<vtable_data>
	{
	protected:
		create_result create(const key_type& entry) override;
	};
}
