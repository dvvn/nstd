module;

#include "nstd/mem/address_includes.h"
#include "cache_includes.h"

export module nstd.rtlib:exports;
export import :cache;
export import nstd.mem;

export namespace nstd::rtlib
{
	struct export_data
	{
		mem::address addr;
	};

	struct exports :rtlib::cache<export_data>
	{
	protected:
		create_result create(const key_type& entry) override;
	};
}
