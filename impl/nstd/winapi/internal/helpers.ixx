module;

#include <nstd/runtime_assert.h>

#include <windows.h>
#include <winternl.h>

export module nstd.winapi.helpers;
export import nstd.mem.address;

export namespace nstd::winapi
{
    struct dos_nt
    {
        dos_nt(LDR_DATA_TABLE_ENTRY* const ldr_entry);

        // base address
        mem::basic_address<IMAGE_DOS_HEADER> dos;
        mem::basic_address<IMAGE_NT_HEADERS> nt;
    };
} // namespace nstd::winapi

module :private;

nstd::winapi::dos_nt::dos_nt(LDR_DATA_TABLE_ENTRY* const ldr_entry)
{
    dos = ldr_entry->DllBase;
    // check for invalid DOS / DOS signature.
    runtime_assert(dos && dos->e_magic == IMAGE_DOS_SIGNATURE /* 'MZ' */);
    nt = dos + dos->e_lfanew;
    // check for invalid NT / NT signature.
    runtime_assert(nt && nt->Signature == IMAGE_NT_SIGNATURE /* 'PE\0\0' */);
}
