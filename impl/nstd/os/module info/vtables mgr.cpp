#include "vtables mgr.h"
#include "cache_impl.h"

#include "nstd/runtime_assert_fwd.h"
#include "nstd/os/module info.h"
#include "nstd/signature.h"

using namespace nstd;
using namespace os;

//todo: add x64 support
static std::optional<vtable_info> _Load_vtable_info(const section_info& dot_rdata, const section_info& dot_text, address type_descriptor)
{
	const auto all_blocks = [&]
	{
		auto storage = std::vector<mem::block>( );
		auto from    = dot_rdata.block;

		const auto block = make_signature(type_descriptor);
		for (;;)
		{
			auto found_block = from.find_block(block);
			if (found_block.empty( ))
				break;
			from = from.shift_to(found_block._Unchecked_end( ));
			storage.push_back(std::move(found_block));
		}
		return storage;
	}( );

	for (const auto& block: all_blocks)
	{
		const address xr = block._Unchecked_begin( );

		// so if it's 0 it means it's the class we need, and not some class it inherits from
		if (const uintptr_t vtable_offset = xr.remove(sizeof(uintptr_t) * 2).ref( ); vtable_offset != 0)
			continue;

		// get the object locator

		const auto vtable_address = [&]
		{
			const auto object_locator = xr.remove(sizeof(uintptr_t) * 3);
			const auto sig            = make_signature(object_locator);
			const auto found          = dot_rdata.block.find_block(sig);
			const address addr        = found._Unchecked_begin( );
			return addr + sizeof(uintptr_t);
		}( );

		// check is valid offset
		if (vtable_address.value( ) <= sizeof(uintptr_t))
			continue;

		// get a pointer to the vtable

		// convert the vtable address to an ida pattern
		const auto temp_result = [&]
		{
			const auto sig = make_signature(vtable_address);
			return dot_text.block.find_block(sig);
		}( );

		if (!temp_result.empty( ))
			return vtable_info{temp_result._Unchecked_begin( )};
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

	const auto bytes = module_info_ptr->mem_block( );

	const auto target_block = bytes.find_block(nstd::make_signature<make_signature_tag_direct{}>(real_name));
	//class descriptor
	runtime_assert(!target_block.empty( ));

	// get rtti type descriptor
	address type_descriptor = target_block._Unchecked_begin( );
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
