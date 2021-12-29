module;

#include "info_includes.h"

#include "nstd/one_instance.h"

#include <functional>
#include <list>
#include <vector>

export module nstd.rtlib.all_infos;
export import nstd.rtlib.info;

export namespace nstd::rtlib
{
	using modules_storage_data = std::vector<info>;
	class modules_storage : modules_storage_data
	{
	public:

		modules_storage( ) = default;

		using modules_storage_data::begin;
		using modules_storage_data::end;
		using modules_storage_data::size;

		modules_storage& update(bool force = false);

		info& current( ) const;
		info& owner( );

	private:
		info* current_cached_ = 0;
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
