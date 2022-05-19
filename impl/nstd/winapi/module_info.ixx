module;

#include <nstd/runtime_assert.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>

export module nstd.winapi.module_info;

class module_info
{
  public:
    using pointer = LDR_DATA_TABLE_ENTRY*;
    using reference = LDR_DATA_TABLE_ENTRY&;

    module_info(pointer const entry);

    pointer operator->() const noexcept;
    reference operator*() const noexcept;

    std::wstring_view path() const noexcept;
    std::wstring_view name() const noexcept;

  private:
    pointer entry_;
};

export namespace nstd::winapi
{
    using ::module_info;
}

module :private;

module_info::module_info(LDR_DATA_TABLE_ENTRY* const entry) : entry_(entry)
{
}

auto module_info::operator->() const noexcept -> pointer
{
    return entry_;
}

auto module_info::operator*() const noexcept -> reference
{
    return *entry_;
}

std::wstring_view module_info::path() const noexcept
{
    return {entry_->FullDllName.Buffer, entry_->FullDllName.Length / sizeof(WCHAR)};
}

std::wstring_view module_info::name() const noexcept
{
    const auto full_path = this->path();
    const auto name_start = full_path.rfind('\\');
    runtime_assert(name_start != full_path.npos, "Unable to get the module name");
    return full_path.substr(name_start + 1);
}
