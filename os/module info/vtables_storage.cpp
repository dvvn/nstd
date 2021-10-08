#include "vtables_storage.h"

#include "nstd/signature.h"
#include "nstd/os/module info.h"
#include "nstd/os/threads.h"

#include <nlohmann/json.hpp>
#include <thread_pool.hpp>

#include <filesystem>

using nstd::address;
using nstd::os::vtable_info;
using nstd::os::section_info;
using namespace nstd::os::detail;

template < >
module_data_cache_fwd<vtable_info> nstd::os::detail::make_cache_fwd<vtable_info>( )
{
	return std::make_unique<module_data_cache<vtable_info>>( );
}

template < >
bool nstd::os::detail::cache_empty<vtable_info>(const module_data_cache<vtable_info>& cache)
{
	return cache.empty( );
}

template < >
void nstd::os::detail::cache_reserve<vtable_info>(module_data_cache<vtable_info>& cache, std::size_t count)
{
	cache.reserve(count);
}

//todo: add x64 support
static std::optional<vtable_info> _Load_vtable_info(const section_info& dot_rdata, const section_info& dot_text, address type_descriptor)
{
	for (const auto& block: dot_rdata.block.find_all_blocks(type_descriptor.value( )))
	{
		const auto xr = block.addr( );

		// so if it's 0 it means it's the class we need, and not some class it inherits from
		if (const auto vtable_offset = xr.remove(0x8).ref( ); vtable_offset != 0)
			continue;

		//NOTE1: find_block function rewritten, now it atomatically convert data to bytes if wanted
		//NOTE2: signature function rewritten, now if prefer span instead of vector
		//NOTE SUMMARY:
		//without signature we select hightest possible integral data type and compare with it
		//with signature we compare byte-to-byte or by memcmp

		// get the object locator
		const auto object_locator = xr - 0xC;
		const auto vtable_address = dot_rdata.block.find_block(object_locator)->addr( ) + 0x4;

		// check is valid offset
		if (vtable_address <= 4U)
			continue;

		// get a pointer to the vtable

		// convert the vtable address to an ida pattern
		const auto temp_result = dot_text.block.find_block(/*signature*/vtable_address);

		//address not found
		if (!temp_result.has_value( ))
			continue;

		vtable_info info;
		info.addr = temp_result->addr( );
		return info;
	}

	return { };
}

using nstd::os::vtables_storage;

vtables_storage::vtables_storage( )
{
	lock_ = std::make_shared<std::mutex>( );
}

void vtables_storage::lock( )
{
	lock_->lock( );
	(void)this;
}

void vtables_storage::unlock( )
{
	lock_->unlock( );
	(void)this;
}

//currently only for x86
bool vtables_storage::load_from_memory(cache_type& cache)
{
#ifdef UTILS_X64
	throw std::runtime_error("todo: x64");
	///look:
	///https://github.com/samsonpianofingers/RTTIDumper/blob/master/
	///https://www.google.com/search?q=rtti+typedescriptor+x64
#endif
	auto& sections = this->derived_sections( );
	if (!sections.load({ }))
		return false;

	const auto& sections_cache = sections.get_cache( );
	const auto& dot_rdata      = sections_cache.at(".rdata");
	const auto& dot_text       = sections_cache.at(".text");

	static const/*expr*/ auto part_before_sig = make_signature(".?AV");
	static const/*expr*/ auto part_after_sig  = make_signature("@@");
	static const/*expr*/ auto part_before     = part_before_sig.get_known( );
	static const/*expr*/ auto part_after      = part_after_sig.get_known( );

	// type descriptor names look like this: .?AVXXXXXXXXXXXXXXXXX@@ (so: ".?AV" + szTableName + "@@")

	auto bytes = this->derived_mem_block( );

	//pause all other threads. we want all the power here;
	const auto frozen = frozen_threads_storage(true);
	(void)frozen;

	constexpr auto bad_byte = [](char c)
	{
		switch (c)
		{
			case ' ':
			case '\0':
			case '@':
			case '?':
			case '$':
			case '<':
				return true;
			default:
				return false;
		}
	};

#if 0
	//coroutine version of this slower, probably runs on single core, idk why
	auto pool = cppcoro::static_thread_pool( );
	(void)pool;

	using generator_result = std::pair<std::string, vtable_info>;;
	auto generator = [&]( )-> cppcoro::async_generator<generator_result>
	{
		for (;;)
		{
			co_await pool.schedule( );
			const auto block_start = bytes.find_block(part_before);
			if (!block_start.has_value( ))
				break;

			bytes = bytes.shift_to_end(*block_start);
			if (bad_byte(*block_start->_Unchecked_end( )))
				continue;

			const auto block_end = bytes.find_block(part_after);
			if (!block_end.has_value( ))
				break;

			bytes = bytes.shift_to_end(*block_end);

			//--------------

			const auto block_start_ptr = block_start->_Unchecked_begin( );
			const auto block_end_ptr   = block_end->_Unchecked_end( );
			const auto block_size_raw  = std::distance(block_start_ptr, block_end_ptr);

			const auto class_descriptor = std::string_view(reinterpret_cast<const char*>(block_start_ptr), block_size_raw);

#if 0
		//skip namespaces
		if(class_name.rfind(static_cast<uint8_t>('@')) != class_name.npos)
			continue;
#endif

			// get rtti type descriptor
			address type_descriptor = class_descriptor.data( );
			// we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the std::string
			type_descriptor -= 0x8;

			co_await pool.schedule( );
			auto result = _Load_vtable_info(dot_rdata, dot_text, type_descriptor);
			if (!result.has_value( ))
				continue;

			auto class_name = class_descriptor;
			class_name.remove_prefix(part_before.size( ));
			class_name.remove_suffix(part_after.size( ));

			co_yield generator_result(std::string(class_name), std::move(*result));
		}
	};

	auto consumer = [&]( )-> cppcoro::task
	{
		auto gen = generator( );

		for (auto itr = co_await gen.begin( ); itr != gen.end( ); co_await ++itr)
		{
			auto& [name, vtable] = *itr;
			cache.emplace(std::move(name), std::move(vtable));
		}
	};

	sync_wait(consumer( ));

#else

	auto pool = thread_pool( );
	using task_result = std::optional<std::pair<std::string, vtable_info>>;
	using task_type = std::future<task_result>;
	auto tasks = std::vector<task_type>( );

	for (;;)
	{
		const auto block_start = bytes.find_block(part_before);
		if (!block_start.has_value( ))
			break;
		bytes = bytes.shift_to_end(*block_start);
		if (bad_byte(*block_start->bytes_range( )._Unchecked_end( )))
			continue;

		const auto block_end = bytes.find_block(part_after);
		if (!block_end.has_value( ))
			break;

		bytes = bytes.shift_to_end(*block_end);

		auto generator = [block_start, block_end, &dot_rdata, &dot_text]( )-> task_result
		{
			const auto block_start_ptr = block_start->bytes_range( )._Unchecked_begin( );
			const auto block_end_ptr   = block_end->bytes_range( )._Unchecked_end( );
			const auto block_size_raw  = std::distance(block_start_ptr, block_end_ptr);

			const auto class_descriptor = std::string_view(reinterpret_cast<const char*>(block_start_ptr), block_size_raw);

#if 0
		//skip namespaces
		if(class_name.rfind(static_cast<uint8_t>('@')) != class_name.npos)
			continue;
#endif

			// get rtti type descriptor
			address type_descriptor = class_descriptor.data( );
			// we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the std::string
			type_descriptor -= 0x8;

			auto result = _Load_vtable_info(dot_rdata, dot_text, type_descriptor);
			if (!result.has_value( ))
				return { };

			auto class_name = class_descriptor;
			class_name.remove_prefix(part_before.size( ));
			class_name.remove_suffix(part_after.size( ));

			return std::make_pair(std::string(class_name), std::move(*result));
		};

		tasks.push_back(pool.submit(generator));
	}

	for (auto& t: tasks)
	{
		auto val = t.get( );
		if (!val.has_value( ))
			continue;
		auto& [name,vtable] = *val;
		cache.emplace(std::move(name), std::move(vtable));
	}

#endif

	return true;
}

bool vtables_storage::load_from_file(cache_type& cache, detail::ptree_type&& storage)
{
	const auto base_address = this->base_addr( );

	for (auto& [name, child]: storage.items( ))
	{
		const auto offset = child.get<uintptr_t>( );
		//const auto size   = child.get<size_t>("size");

		vtable_info info;
		info.addr = base_address + offset;
		cache.emplace(const_cast<std::string&&>(name), std::move(info));
	}

	return true;
}

bool vtables_storage::read_to_storage(const cache_type& cache, detail::ptree_type& storage) const
{
	const auto base_address = this->base_addr( );

	for (const auto& [name, info]: cache)
	{
		storage[name] = info.addr.remove(base_address).value( );
	}

	return true;
}

nstd::os::sections_storage& vtables_storage::derived_sections( )
{
	return root_class( )->sections( );
}

nstd::memory_block vtables_storage::derived_mem_block( ) const
{
	return root_class( )->mem_block( );
}

//void vtables_storage::Change_base_address_impl(address new_addr)
//{
//	for (auto itr = data_cache.begin( ); itr != data_cache.end( ); ++itr)
//	{
//		auto& val = itr.value( ).addr;
//		val -= (base_address);
//		val += new_addr;
//	}
//}
