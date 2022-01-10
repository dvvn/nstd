module;

#include "includes.h"

export module nstd.rtlib:all_infos;
export import :info;

export namespace nstd::rtlib
{
	using modules_storage_data = std::/*list*/vector<info>;
	class modules_storage : modules_storage_data
	{
	public:

		modules_storage( ) = default;

		using modules_storage_data::begin;
		using modules_storage_data::end;
		using modules_storage_data::size;

		modules_storage& update(bool force = false);

		const info& current( ) const;
		info& current( );
		const info& owner( )const;
		info& owner( );

	private:
		size_t current_index_ = static_cast<size_t>(-1);
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
