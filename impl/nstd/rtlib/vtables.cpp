module;

#include "info_includes.h"

#include "nstd/runtime_assert.h"
#include "nstd/mem/block_includes.h"

#include <optional>

module nstd.rtlib:vtables;
import :info;
import nstd.mem;

using namespace nstd;
using namespace mem;
using namespace rtlib;

//todo: add x64 support
static std::optional<vtable_data> _Load_vtable(const section_data& dot_rdata, const section_data& dot_text, address type_descriptor)
{
	/*const auto all_blocks = [&]
	{
		std::vector<block> storage;
		auto from = dot_rdata.block;
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
	};*/

	auto from = dot_rdata.block;
	auto search = make_signature(type_descriptor);

	for (;;)
	{
		auto block = from.find_block(search);
		if (block.empty( ))
			break;
		from = from.shift_to(block._Unchecked_end( ));

		const basic_address xr = block.data( );

		// so if it's 0 it means it's the class we need, and not some class it inherits from
		const uintptr_t vtable_offset = *xr.remove(sizeof(uintptr_t) * 2).pointer;
		if (vtable_offset != 0)
			continue;

		// get the object locater

		const auto vtable_address = [&]
		{
			const auto object_locator = xr.remove(sizeof(uintptr_t) * 3);
			const auto sig = make_signature(object_locator);
			const auto found = dot_rdata.block.find_block(sig);
			const basic_address addr = found.data( );
			return addr + sizeof(uintptr_t);
		}();

		// check is valid offset
		if (vtable_address <= sizeof(uintptr_t))
			continue;

		// get a pointer to the vtable

		// convert the vtable address to an ida pattern
		const auto temp_result = [&]
		{
			const auto sig = make_signature(vtable_address);
			return dot_text.block.find_block(sig);
		}();

		if (!temp_result.empty( ))
			return vtable_data(temp_result.data( ));
	}

	return {};
}

static std::string _Make_class_name(const std::string_view& class_name)
{
	constexpr std::string_view prefix = ".?AV";
	constexpr std::string_view postfix = "@@";

	std::string tmp;
	tmp.reserve(prefix.size( ) + class_name.size( ) + postfix.size( ));
	tmp += prefix;
	tmp += class_name;
	tmp += postfix;
	return tmp;
}

auto vtables_storage::create(const key_type& entry) -> create_result
{
	const auto real_name = _Make_class_name(entry);

	auto info_ptr = this->root_class( );

	const auto bytes = info_ptr->mem_block( );
	const auto target_block = bytes.find_block(make_signature(real_name.begin( ), real_name.end( ), signature_direct( )));
	//class descriptor
	runtime_assert(!target_block.empty( ));

	// get rtti type descriptor
	basic_address type_descriptor = target_block.data( );
	// we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the string
	type_descriptor -= sizeof(uintptr_t) * 2;

	auto& sections = info_ptr->sections( );

	using namespace std::string_view_literals;
	const auto& dot_rdata = sections.at(".rdata"sv);
	const auto& dot_text = sections.at(".text"sv);

	auto result = _Load_vtable(dot_rdata, dot_text, type_descriptor);
	runtime_assert(result.has_value( ));
	return *result;
}
