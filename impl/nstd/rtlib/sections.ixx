module;

#include "cache_includes.h"
#include <windows.h>

export module nstd.rtlib.info:sections;
export import :cache;
export import nstd.mem.block;

export namespace nstd::rtlib
{
	struct sections_data
	{
		mem::block block;
		IMAGE_SECTION_HEADER* data = nullptr;
	};

	struct sections :cache<sections_data>
	{
	protected:
		create_result create(const key_type& entry) override;
	};
}
