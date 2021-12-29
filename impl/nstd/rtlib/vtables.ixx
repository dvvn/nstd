module;

#include "nstd/address_includes.h"
#include "cache_includes.h"

export module nstd.rtlib.info:vtables;
export import :cache;
export import nstd.address;

export namespace nstd::rtlib
{
	struct vtables_data 
	{
		address addr;
	};

	struct vtables :cache<vtables_data>
	{
	protected:
		create_result create(const key_type& entry) override;
	};
}
