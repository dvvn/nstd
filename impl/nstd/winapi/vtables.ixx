module;

#include "internal/arguments_protect.h"
#include "internal/msg_invoke.h"

#include <windows.h>
#include <winternl.h>

//#include <functional>
#include <string_view>

export module nstd.winapi.vtables;
export import nstd.winapi.modules;

export namespace nstd::winapi
{
    void* find_vtable(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name) runtime_assert_noexcept;

    template <typename T>
    struct found_vtable
    {
        using value_type = T*;

        value_type ptr;

        found_vtable(void* ptr) : ptr(static_cast<value_type>(ptr))
        {
        }
    };

    template <typename Msg = void*>
    void* find_vtable(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::wstring_view module_name, const std::string_view vtable_name) runtime_assert_noexcept
    {
        const auto found = find_vtable(ldr_entry, vtable_name);
        _Invoke_msg<Msg, found_vtable<void*>>(found, module_name, vtable_name);
        return found;
    }

    template <typename T, text::chars_cache Module, text::chars_cache Class, typename Msg = void*>
    T* find_vtable() runtime_assert_noexcept
    {
        static const found_vtable<T> found = find_vtable<Msg>(find_module<Module, Msg>(), Module, Class);
        _Protect_args<Module, Class>(__FUNCSIG__);
        return found.ptr;
    }
} // namespace nstd::winapi

module :private;
import nstd.winapi.sections;
import nstd.winapi.helpers;
import nstd.mem.address;
import nstd.mem.block;

using nstd::mem::basic_address;
using nstd::mem::block;

// block = {dos + header->VirtualAddress, header->SizeOfRawData}

// template<typename T>
// static block _Address_to_rng(const basic_address<T> addr)
//{
//	return {addr.get<uint8_t*>( ), sizeof(uintptr_t)};
// }

namespace nstd::mem
{
    template <typename T>
    block make_signature(basic_address<T>&& addr) = delete;

    template <typename T>
    block make_signature(const basic_address<T>& addr)
    {
        // return {addr.get<uint8_t*>( ), sizeof(uintptr_t)};
        return {(uint8_t*)std::addressof(addr), sizeof(uintptr_t)};
    }
} // namespace nstd::mem

// todo: add x64 support
static uint8_t* _Load_vtable(const block dot_rdata, const block dot_text, const basic_address<void> type_descriptor)
{
    auto from = dot_rdata;
    const auto search = make_signature(type_descriptor);

    for (;;)
    {
        auto block = from.find_block(search);
        if (block.empty())
            break;
        from = from.shift_to(block.data() + block.size());

        //-------------

        const basic_address<void> xr = block.data();

        // so if it's 0 it means it's the class we need, and not some class it inherits from
        if (const uintptr_t vtable_offset = xr.minus(sizeof(uintptr_t) * 2).deref<1>(); vtable_offset != 0)
            continue;

        // get the object locator

        const auto vtable_address = [&] {
            const auto object_locator = xr.minus(sizeof(uintptr_t) * 3);
            const auto sig = make_signature(object_locator);
            const auto found = dot_rdata.find_block(sig);
            const basic_address<void> addr = found.data();
            return addr + sizeof(uintptr_t);
        }();

        // check is valid offset
        if (vtable_address.value <= sizeof(uintptr_t))
            continue;

        // get a pointer to the vtable

        // convert the vtable address to an ida pattern
        const auto temp_result = [&] {
            const auto sig = make_signature(vtable_address);
            return dot_text.find_block(sig);
        }();

        if (!temp_result.empty())
            return temp_result.data();
    }

    return nullptr;
}

static auto _Make_vtable_name(const std::string_view name)
{
    constexpr std::string_view prefix = ".?AV";
    constexpr std::string_view suffix = "@@";

    std::basic_string<uint8_t> buff;
    buff.reserve(prefix.size() + name.size() + suffix.size());
    buff.append(prefix.begin(), prefix.end());
    buff.append(name.begin(), name.end());
    buff.append(suffix.begin(), suffix.end());
    return buff;
}

static block _Section_to_rng(const basic_address<IMAGE_DOS_HEADER> dos, IMAGE_SECTION_HEADER* const section)
{
    uint8_t* const ptr = dos + section->VirtualAddress;
    return {ptr, section->SizeOfRawData};
}

void* nstd::winapi::find_vtable(LDR_DATA_TABLE_ENTRY* const ldr_entry, const std::string_view name) runtime_assert_noexcept
{
    const auto [dos, nt] = dos_nt(ldr_entry);

    const auto real_name = _Make_vtable_name(name);

    const block bytes = {dos.get<uint8_t*>(), nt->OptionalHeader.SizeOfImage};
    const block target_block = bytes.find_block({real_name.data(), real_name.size()});
    // class descriptor
    runtime_assert(!target_block.empty());

    // get rtti type descriptor
    basic_address<void> type_descriptor = target_block.data();
    // we're doing - 0x8 here, because the location of the rtti typedescriptor is 0x8 bytes before the string
    type_descriptor -= sizeof(uintptr_t) * 2;

    const auto dot_rdata = find_section(ldr_entry, ".rdata");
    const auto dot_text = find_section(ldr_entry, ".text");

    const auto result = _Load_vtable(_Section_to_rng(dos, dot_rdata), _Section_to_rng(dos, dot_text), type_descriptor);
    runtime_assert(result != nullptr);
    return result;
}
