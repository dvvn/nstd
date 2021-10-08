#pragma once

#include "root class.h"

#include "nstd/address.h"

#include <memory>
#include <string_view>

namespace nstd::os
{
	struct export_info
	{
		address addr;
	};

	class exports_mgr: protected virtual detail::root_class_getter
	{
	public:
		exports_mgr( );
		~exports_mgr( ) override;

		exports_mgr(exports_mgr&&) noexcept;
		exports_mgr& operator=(exports_mgr&&) noexcept;

		struct storage_type;

		export_info at(const std::string_view& name) const;

	private:
		std::unique_ptr<storage_type> storage_;
	};
}
