#pragma once

#include "root class.h"

#include "nstd/address.h"

#include <memory>
#include <string_view>

namespace nstd::os
{
	struct vtable_info
	{
		address addr;
	};

	class vtables_mgr: protected virtual detail::root_class_getter
	{
	public:
		struct storage_type;

		vtables_mgr( );
		~vtables_mgr( ) override;

		vtables_mgr(vtables_mgr&&) noexcept;
		vtables_mgr& operator=(vtables_mgr&&) noexcept;

		vtable_info at(const std::string_view& class_name) const;

	private:
		std::unique_ptr<storage_type> storage_;
	};
}
