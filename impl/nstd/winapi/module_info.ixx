module;

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

    pointer operator->() const;
    reference operator*() const;

    std::wstring_view path() const;
    std::wstring_view name() const;

  private:
    pointer entry_;
};

export namespace nstd::winapi
{
    using ::module_info;
}
