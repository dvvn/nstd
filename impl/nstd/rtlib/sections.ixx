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

	struct sections :cache<section_data>
	{
	protected:
		create_result create(const key_type& entry) override;
	};
}