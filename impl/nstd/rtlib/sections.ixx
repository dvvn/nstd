module;

#include "cache_includes.h"
#include <windows.h>

export module nstd.rtlib:sections;
export import :cache;
export import nstd.mem;

export namespace nstd::rtlib
{
	struct section_data
	{
		mem::block block;
		IMAGE_SECTION_HEADER* data = nullptr;
	};

	class sections_storage :protected cache<section_data, true>
	{
		create_result create(const key_type& entry) final;

	public:
		cache& sections( ) { return *this; }
		const cache& sections( )const { return *this; }
	};

	void swap(sections_storage& l, sections_storage& r)
	{
		l.sections( ).swap(r.sections( ));
	}
}
