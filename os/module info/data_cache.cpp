#include "data_cache.h"

#include "nstd/checksum.h"
#include "nstd/os/module info.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using nstd::address;
using namespace nstd::os::detail;

//path_fwd::path_fwd( )
//	: std::unique_ptr(std::make_unique<element_type>( ))
//{
//}
//
//bool path_fwd::empty( ) const
//{
//	return get( )->empty( );
//}
//
//ptree_fwd::ptree_fwd( )
//	: std::unique_ptr(std::make_unique<element_type>( ))
//{
//}
//
//bool ptree_fwd::empty( ) const
//{
//	return get( )->empty( );
//}

path_fwd nstd::os::detail::make_path( )
{
	return std::make_unique<path_type>( );
}

bool nstd::os::detail::path_empty(const path_type& p)
{
	return p.empty( );
}

ptree_fwd nstd::os::detail::make_ptree( )
{
	return std::make_unique<ptree_type>( );
}

std::size_t nstd::os::detail::ptree_size(const ptree_type& tree)
{
	return tree.size( );
}

address module_data_mgr_base::base_addr( ) const
{
	return root_class( )->base( );
}

IMAGE_NT_HEADERS* module_data_mgr_base::nt_header( ) const
{
	return root_class( )->NT( );
}

bool module_data_mgr_base::write_from_storage(const path_type& file, const ptree_type& storage) const
{
	if (storage.empty( ))
		return false;
	if (!exists(file))
	{
		create_directories(file.parent_path( ));
		std::ofstream(file) << storage;
	}
	else
	{
		const auto buff = std::ostringstream( ) << storage;
		if (checksum(buff) != checksum(file))
			std::ofstream(file) << (buff.view( ));
	}
	return true;
}

bool module_data_mgr_base::read_to_storage(const path_type& file, ptree_type& storage) const
{
	if (!storage.empty( ))
		return false;
	if (!exists(file))
		return false;
	std::ifstream(file) >> storage;
	return true;
}
