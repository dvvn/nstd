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

static block _Section_to_rng(const basic_address<IMAGE_DOS_HEADER> dos, IMAGE_SECTION_HEADER* section)
{
	uint8_t* const ptr = dos + section->VirtualAddress;
	return {ptr,section->SizeOfRawData};
}

//template<typename T>
//static block _Address_to_rng(const basic_address<T> addr)
//{
//	return {addr.get<uint8_t*>( ), sizeof(uintptr_t)};
//}

//todo: add x64 support
static uint8_t* _Load_vtable(const block dot_rdata, const block dot_text, const basic_address<void> type_descriptor)
{
	auto from = dot_rdata;
	const auto search = make_signature(type_descriptor);

	for (;;)
	{
		auto block = from.find_block({search.begin( ),search.size( )});
		if (block.empty( ))
			break;
		from = from.shift_to(block.data( ) + block.size( ));

		//-------------

		const basic_address<void> xr = block.data( );

		// so if it's 0 it means it's the class we need, and not some class it inherits from
		if (const uintptr_t vtable_offset = xr.minus(sizeof(uintptr_t) * 2).deref<1>( ); vtable_offset != 0)
			continue;

		// get the object locator

		const auto vtable_address = [&]
		{
			const auto object_locator = xr.minus(sizeof(uintptr_t) * 3);
			const auto sig = make_signature(object_locator);
			const auto found = dot_rdata.find_block({sig.begin( ),sig.size( )});
			const basic_address<void> addr = found.data( );
			return addr + sizeof(uintptr_t);
		}();

		// check is valid offset
		if (vtable_address.value <= sizeof(uintptr_t))
			continue;

		// get a pointer to the vtable

		// convert the vtable address to an ida pattern
		const auto temp_result = [&]
		{
			const auto sig = make_signature(vtable_address);
			return dot_text.find_block({sig.begin( ),sig.size( )});
		}();

		if (!temp_result.empty( ))
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
	basic_address<void> type_descriptor = target_block.data( );
	// we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the string
	type_descriptor -= sizeof(uintptr_t) * 2;

	const auto dot_rdata = winapi::find_section_impl(ldr_entry, ".rdata");
	const auto dot_text = winapi::find_section_impl(ldr_entry, ".text");

	const auto result = _Load_vtable(_Section_to_rng(dos, dot_rdata), _Section_to_rng(dos, dot_text), type_descriptor);
	runtime_assert(result != nullptr);
	return result;
}