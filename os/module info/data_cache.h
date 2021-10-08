#pragma once

//#include "nstd/containers.h"

#include <nlohmann/json_fwd.hpp>

// ReSharper disable CppInconsistentNaming
#ifdef _WIN64
struct _IMAGE_NT_HEADERS64;
using IMAGE_NT_HEADERS64 = _IMAGE_NT_HEADERS64;
using IMAGE_NT_HEADERS = IMAGE_NT_HEADERS64;
#else
struct _IMAGE_NT_HEADERS;
using IMAGE_NT_HEADERS32 = _IMAGE_NT_HEADERS;
using IMAGE_NT_HEADERS = IMAGE_NT_HEADERS32;
#endif
struct _LDR_DATA_TABLE_ENTRY;
struct _IMAGE_DOS_HEADER;
using LDR_DATA_TABLE_ENTRY = _LDR_DATA_TABLE_ENTRY;
using IMAGE_DOS_HEADER = _IMAGE_DOS_HEADER;
using DWORD = unsigned long;
// ReSharper restore CppInconsistentNaming

namespace std::filesystem
{
	class path;
}

namespace nstd
{
	class address;
}

namespace nstd::os
{
	class module_info;
}

namespace nstd::os::detail
{
	struct module_data_mgr_root_getter
	{
	protected:
		virtual ~module_data_mgr_root_getter( ) = default;

		virtual module_info*       root_class( ) =0;
		virtual const module_info* root_class( ) const =0;
	};

	using path_type = std::filesystem::path;
	using ptree_type = nlohmann::json;

	using path_fwd = std::unique_ptr<path_type>;
	path_fwd make_path( );
	bool     path_empty(const path_type& p);

	using ptree_fwd = std::unique_ptr<ptree_type>;
	ptree_fwd make_ptree( );
	/*ptree_type* make_ptree_manual( );
	void        destroy_ptree_manual(ptree_type*);*/
	std::size_t ptree_size(const ptree_type& tree);

	class module_data_mgr_base: public virtual module_data_mgr_root_getter
	{
	public:
		virtual bool load(const path_type& file) =0;
		virtual bool save_to_file(const path_type& file) const =0;

		//virtual path_type get_file_name( ) const =0;

	protected:
		address           base_addr( ) const;
		IMAGE_NT_HEADERS* nt_header( ) const;

		bool write_from_storage(const path_type& file, const ptree_type& storage) const;
		bool read_to_storage(const path_type& file, ptree_type& storage) const;
	};

	template <typename T>
	using module_data_cache = tsl::robin_map<std::string, T>;

	template <typename T>
	using module_data_cache_fwd = std::unique_ptr<module_data_cache<T>>;

	template <typename T>
	module_data_cache_fwd<T> make_cache_fwd( );
	template <typename T>
	bool cache_empty(const module_data_cache<T>& cache);
	template <typename T>
	void cache_reserve(module_data_cache<T>& cache, std::size_t count);

	template <typename T>
	class module_data_mgr: public module_data_mgr_base
	{
	public:
		using cache_type = module_data_cache<T>;
		using cache_fwd = module_data_cache_fwd<T>;

		module_data_mgr( ) = default;

		module_data_mgr(module_data_mgr&& other)            = default;
		module_data_mgr& operator=(module_data_mgr&& other) = default;

		cache_type& get_cache( )
		{
			return *cache_;
		}

		//not const at real, but we emulate non-pointer class member
		const cache_type& get_cache( ) const
		{
			return *cache_;
		}

	protected:
		//todo: do it in another way
		//virtual cache_fwd   make_cache( ) const =0;
		//virtual std::size_t cache_size(const cache_type& cache) const =0;

	public:
		bool load(const path_type& file/* = { }*/) final
		{
			if (cache_ == nullptr || cache_empty(*cache_))
			{
				ptree_fwd tree       = make_ptree( );
				cache_fwd temp_cache = make_cache_fwd<T>( );
				if (module_data_mgr_base::read_to_storage(file, *tree))
				{
					cache_reserve(*temp_cache, ptree_size(*tree));
					if (this->load_from_file(*temp_cache, std::move(*tree)))
					{
						cache_ = std::move(temp_cache);
						return true;
					}
				}
				if (!this->load_from_memory(*temp_cache))
					return false;
				cache_ = std::move(temp_cache);
			}
			if (path_empty(file))
				return true;
			return this->save_to_file(file);
		}

		bool save_to_file(const path_type& file) const final
		{
			if (path_empty(file))
				return false;
			if (cache_ == nullptr || cache_empty(*cache_))
				return false;
			auto tree = make_ptree( );
			if (!this->read_to_storage(*cache_, *tree))
				return false;
			return write_from_storage(file, *tree);
		}

		virtual bool load_from_memory(cache_type& cache) =0;
		virtual bool load_from_file(cache_type& cache, ptree_type&& storage) =0;
	protected:
		virtual bool read_to_storage(const cache_type& cache, ptree_type& storage) const =0;

	private:
		cache_fwd cache_;
	};
}
