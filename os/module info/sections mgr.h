#pragma once

#include "root class.h"

#include "nstd/memory block.h"

#include <memory>

namespace nstd::os
{
	struct section_info
	{
		memory_block          block;
		IMAGE_SECTION_HEADER* data = nullptr;
	};

	class sections_mgr: protected virtual detail::root_class_getter
	{
	public:
		sections_mgr( );
		~sections_mgr( ) override;

		sections_mgr(sections_mgr&&) noexcept;
		sections_mgr& operator=(sections_mgr&&) noexcept;

		struct storage_type;

		section_info at(const std::string_view& name) const;

	private:
		std::unique_ptr<storage_type> storage_;
	};
}
