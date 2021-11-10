#pragma once
#include "info.h"

#include <nstd/one_instance.h>

namespace nstd::module
{
	class modules_storage
	{
	public:
		struct storage_type;

		modules_storage(const modules_storage&)            = delete;
		modules_storage& operator=(const modules_storage&) = delete;

		modules_storage(modules_storage&&) noexcept;
		modules_storage& operator=(modules_storage&&) noexcept;

		modules_storage( );
		~modules_storage( );

		modules_storage& update(bool force = false);

		info& current( ) const;
		info& owner( );

		using find_fn = std::function<bool(const info &)>;
		info* find(const find_fn& fn);
		info* rfind(const find_fn& fn);

	private:
		std::unique_ptr<storage_type> storage_;
		info* current_cached_ = nullptr;
	};

	namespace detail
	{
		struct all_infos_impl : modules_storage
		{
			all_infos_impl( )
			{
				this->update( );
			}
		};
	}

	using all_infos = one_instance<detail::all_infos_impl>;
}
