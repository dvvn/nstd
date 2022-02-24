module;

#include "includes.h"

export module nstd.rtlib:all_infos;
export import :info;

namespace nstd::rtlib
{
	using modules_storage_data = std::/*list*/vector<info>;
	export class modules_storage : modules_storage_data
	{
		modules_storage& operator=(modules_storage_data&& other)noexcept;

	public:
		modules_storage( ) = default;

		using modules_storage_data::begin;
		using modules_storage_data::end;
		using modules_storage_data::size;
		using modules_storage_data::operator[];

		bool update(bool force = false);

		const info& current( ) const;
		info& current( );
		const info& owner( )const;
		info& owner( );

	private:
		size_t current_index_ = static_cast<size_t>(-1);
	};

	export using all_infos = one_instance<modules_storage>;
}
