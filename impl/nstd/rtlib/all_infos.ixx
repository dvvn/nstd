module;

#include "includes.h"

export module nstd.rtlib:all_infos;
export import :info;

export namespace nstd::rtlib
{
	using modules_storage_data = std::/*list*/vector<info>;
	class modules_storage : modules_storage_data
	{
		modules_storage& operator=(modules_storage_data&& other)noexcept;

	public:
		modules_storage( ) = default;

		using lock_fn = std::function<bool(const modules_storage_data&)>;

		using modules_storage_data::front;
		using modules_storage_data::back;
		using modules_storage_data::begin;
		using modules_storage_data::end;
		using modules_storage_data::rbegin;
		using modules_storage_data::rend;
		using modules_storage_data::size;
		using modules_storage_data::empty;
		using modules_storage_data::operator[];

		bool update(bool force = false);

		const info& current( ) const;
		info& current( );
		const info& owner( ) const;
		info& owner( );

		bool locked( )const;
		void unlock( );
		void set_locker(lock_fn&& fn);
		bool contains_locker( )const;

	private:
		void try_lock( );

		size_t current_index_ = static_cast<size_t>(-1);
		bool locked_ = false;
		lock_fn locker_;
	};

	using all_infos = one_instance<modules_storage>;
}
