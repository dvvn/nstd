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

	class vtables_storage :protected cache<vtable_data, false>
	{
		create_result create(const key_type& entry) final;

	public:
		cache& vtables( ) { return *this; }
		const cache& vtables( )const { return *this; }
	};

	void swap(vtables_storage& l, vtables_storage& r)
	{
		l.vtables( ).swap(r.vtables( ));
	}
}