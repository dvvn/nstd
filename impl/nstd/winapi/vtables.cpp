module;

#include <nstd/runtime_assert.h>
#include <windows.h>
#include <winternl.h>
#include <string_view>
#include <functional>

module nstd.winapi.vtables;
import nstd.winapi.sections;
import nstd.container.wrapper;
import nstd.mem.block;
import nstd.mem.address;
import nstd.mem.signature;

using namespace nstd;
using namespace mem;

//block = {dos + header->VirtualAddress, header->SizeOfRawData}

static block _To_block(const basic_address<IMAGE_DOS_HEADER> dos, IMAGE_SECTION_HEADER* section)
{
	uint8_t* const ptr = dos + section->VirtualAddress;
	return {ptr,section->SizeOfRawData};
}

//todo: add x64 support
static uint8_t* _Load_vtable(const block rdata_block, const block text_block, const address type_descriptor)
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

	//const block rdata_block = {dos + dot_rdata->VirtualAddress, dot_rdata->SizeOfRawData};
	//const block text_block = {dos + dot_text->VirtualAddress, dot_text->SizeOfRawData};

	auto from = rdata_block;
	const auto search = make_signature(type_descriptor);

	for (;;)
	{
		const auto block = from.find_block(search);
		if (block.empty( ))
			break;
		from = from.shift_to(block._Unchecked_end( ));

		const basic_address xr = block.data( );

		// so if it's 0 it means it's the class we need, and not some class it inherits from
		const uintptr_t vtable_offset = *xr - sizeof(uintptr_t) * 2;
		if (vtable_offset != 0)
			continue;

		// get the object locater

		const auto vtable_address = [&]
		{
			const auto object_locator = xr - sizeof(uintptr_t) * 3;
			const auto sig = make_signature(object_locator);
			const auto found = rdata_block.find_block(sig);
			const basic_address addr = found.data( );
			return addr + sizeof(uintptr_t);
		}();

		// check is valid offset
		if (vtable_address <= sizeof(uintptr_t))
			continue;

		// get a pointer to the vtable

		// convert the vtable address to an ida pattern
		const auto temp_result = text_block.find_block(make_signature(vtable_address));
		if (temp_result.empty( ))
			continue;

		return temp_result.data( );
	}

	return nullptr;
}

void* find_vtable_impl(LDR_DATA_TABLE_ENTRY* ldr_entry, const std::string_view name)
{
	//base address
	const basic_address<IMAGE_DOS_HEADER> dos = ldr_entry->DllBase;
	const basic_address<IMAGE_NT_HEADERS> nt = dos + dos->e_lfanew;

	using namespace std::string_view_literals;
	const auto real_name = nstd::append<std::string>(".?AV"sv, name, "@@"sv);

	const block bytes = {dos.get<uint8_t*>( ),nt->OptionalHeader.SizeOfImage};
	const auto target_block = bytes.find_block(make_signature(real_name.begin( ), real_name.end( ), signature_direct( )));
	//class descriptor
	runtime_assert(!target_block.empty( ));

	// get rtti type descriptor
	basic_address type_descriptor = target_block.data( );
	// we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the string
	type_descriptor -= sizeof(uintptr_t) * 2;

	const auto dot_rdata = winapi::find_section_impl(ldr_entry, ".rdata");
	const auto dot_text = winapi::find_section_impl(ldr_entry, ".text");

	const auto result = _Load_vtable(_To_block(dos, dot_rdata), _To_block(dos, dot_text), type_descriptor);
	runtime_assert(result != nullptr);
	return result;
}