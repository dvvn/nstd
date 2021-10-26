#include "vtables mgr.h"

#include "cache_impl.h"

#include <nstd/runtime_assert_fwd.h>
#include <nstd/os/module info.h>

using namespace nstd::os;

//todo: add x64 support
static std::optional<vtable_info> _Load_vtable_info(const section_info& dot_rdata, const section_info& dot_text, nstd::address type_descriptor)
{
	for (const auto& block: dot_rdata.block.find_all_blocks(type_descriptor.value( )))
	{
		const auto xr = block.addr( );

		// so if it's 0 it means it's the class we need, and not some class it inherits from
		if (const auto vtable_offset = xr.remove(sizeof(uintptr_t) * 2).ref<uintptr_t>( ); vtable_offset != 0)
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

	return {};
}

NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP(vtables_mgr, vtable_info)
{
	constexpr std::string_view prefix  = ".?AV";
	constexpr std::string_view postfix = "@@";

	const auto& class_name = entry;

	const auto real_name = [&]
	{
		std::string tmp;
		tmp.reserve(prefix.size( ) + postfix.size( ) + class_name.size( ));
		tmp += prefix;
		tmp += class_name;
		tmp += postfix;
		return tmp;
	}( );

	const auto bytes        = module_info_ptr->mem_block( );
	const auto target_block = bytes.find_block(real_name); //class descriptor
	runtime_assert(target_block.has_value( ));

	// get rtti type descriptor
	auto type_descriptor = target_block->addr( );
	// we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the string
	type_descriptor -= sizeof(uintptr_t) * 2;

	auto& sections = module_info_ptr->sections( );

	using namespace std::string_view_literals;
	const auto& dot_rdata = sections.at(".rdata"sv);
	const auto& dot_text  = sections.at(".text"sv);

	auto result = _Load_vtable_info(dot_rdata, dot_text, type_descriptor);
	runtime_assert(result.has_value( ));

	return {*result, true};
}
