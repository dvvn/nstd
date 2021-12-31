module;

#include "includes.h"

export module nstd.rtlib:all_infos;
export import :info;

export namespace nstd::rtlib
{
	using modules_storage_data = std::list<info>;
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

		template<typename Pred>
		info* find(Pred&& pred)
		{
			for (info& i : *this)
			{
				if (std::invoke(pred, i))
					return std::addressof(i);
			}

			return nullptr;
		}

		template<typename Pred>
		info* rfind(Pred&& pred)
		{
			const auto end = this->rend( );
			for (auto it = this->rbegin( ); it != end; ++it)
			{
				auto& i = *it;
				if (std::invoke(pred, i))
					return std::addressof(i);
			}

			return nullptr;
		}

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
