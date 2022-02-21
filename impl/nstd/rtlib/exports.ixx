module;

#include "nstd/mem/address_includes.h"
#include "cache_includes.h"

export module nstd.rtlib:exports;
export import :cache;
export import nstd.mem.address;

export namespace nstd::rtlib
{
	struct export_data
	{
		mem::address addr;
	};

	class exports_storage :protected cache<export_data, true>
	{
		create_result create(const key_type& entry) final;

	public:
		cache& exports( ) { return *this; }
		const cache& exports( )const { return *this; }
	};

	void swap(exports_storage& l, exports_storage& r)
	{
		l.exports( ).swap(r.exports( ));
	}
}
