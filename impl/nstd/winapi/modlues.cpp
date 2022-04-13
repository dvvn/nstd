module;

#include <nstd/runtime_assert.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>
#include <functional>

module nstd.winapi.modules;
import nstd.mem.address;

template<typename T>
class string_converter
{
	using value_type = std::basic_string_view<T>;

	value_type str_;

	template<typename To>
	bool directly_convertible( ) const noexcept
	{
		static_assert(sizeof(To) < sizeof(T));

		for (const auto chr : str_)
		{
			const auto tmp = static_cast<To>(chr);
			const auto chr1 = static_cast<T>(tmp);

			if (chr != chr1)
				return false;
		}
		return true;
	}

public:
	string_converter(const value_type str)
		:str_(str)
	{
	}

	template<typename To>
	auto convert( ) const noexcept
	{
		if constexpr (std::same_as<To, T>)
		{
			return str_;
		}
		else
		{
#ifdef __cpp_lib_char8_t
			static_assert(!std::same_as<char8_t, To> && !std::same_as<char8_t, T>, __FUNCSIG__": char8_t conversion not implemented");
#endif
			if constexpr (sizeof(To) == sizeof(T))
			{
				const std::basic_string_view<To> buff = {reinterpret_cast<To*>(str_.data( )),str_.size( )};
				return buff;
			}
			else
			{
				if constexpr (sizeof(To) < sizeof(T))
					runtime_assert(this->directly_convertible<To>( ));

				std::basic_string<To> buff;
				buff.reserve(str_.size( ));
				for (const auto c : str_)
					buff.push_back(static_cast<To>(c));
				return buff;
			}
		}
	}

	template<typename To>
	operator std::basic_string_view<To>( ) const noexcept
	{
		const auto tmp = this->convert<To>( );
		static_assert(sizeof(decltype(tmp)) == sizeof(std::basic_string_view<To>));
		return tmp;
	}

	template<typename To>
	operator std::basic_string<To>( ) const noexcept
	{
		return this->convert<To>( );
	}
};

static string_converter<WCHAR> _Get_module_name(LDR_DATA_TABLE_ENTRY* const ldr_entry) noexcept
{
	const std::basic_string_view full_path = {ldr_entry->FullDllName.Buffer, ldr_entry->FullDllName.Length / sizeof(WCHAR)};
	const auto name_start = full_path.rfind('\\');
	runtime_assert(name_start != full_path.npos, "Unable to get the module name");
	return full_path.substr(name_start + 1);
}

template<typename To>
static auto _Get_module_name(LDR_DATA_TABLE_ENTRY* const ldr_entry) noexcept
{
	const auto name = _Get_module_name(ldr_entry);
	return name.convert<To>( );
}

#pragma region find_module

class module_finder
{
	auto _Get_ldr( ) const noexcept
	{
		const auto mem =
#if defined(_M_X64) || defined(__x86_64__)
			NtCurrentTeb( );
		runtime_assert(mem != nullptr, "Teb not found");
		return mem->ProcessEnvironmentBlock->Ldr;
#else
			reinterpret_cast<PEB*>(__readfsdword(0x30));
		runtime_assert(mem != nullptr, "Peb not found");
		return mem->Ldr;
#endif
	}

public:
	template<typename Fn>
	LDR_DATA_TABLE_ENTRY* operator()(const Fn comparer) const noexcept
	{
		const auto ldr = _Get_ldr( );
		// get module linked list.
		const auto list = std::addressof(ldr->InMemoryOrderModuleList);
		// iterate linked list.
		for (auto it = list->Flink; it != list; it = it->Flink)
		{
			// get current entry.
			const auto ldr_entry = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
			if (!ldr_entry)
				continue;

			//base address
			const basic_address<IMAGE_DOS_HEADER> _dos = ldr_entry->DllBase;
			// check for invalid DOS / DOS signature.
			if (!_dos || _dos->e_magic != IMAGE_DOS_SIGNATURE /* 'MZ' */)
				continue;

			const basic_address<IMAGE_NT_HEADERS> _nt = _dos + _dos->e_lfanew;
			// check for invalid NT / NT signature.
			if (!_nt || _nt->Signature != IMAGE_NT_SIGNATURE /* 'PE\0\0' */)
				continue;

			IMAGE_NT_HEADERS* const nt = _nt;
			IMAGE_DOS_HEADER* const dos = _dos;

			if (!std::invoke(comparer, ldr_entry, nt, dos))
				continue;

			return ldr_entry;
		}

		return nullptr;
	}

	template<typename T>
	LDR_DATA_TABLE_ENTRY* operator()(const std::basic_string<T>& name, const bool check_whole_path) const noexcept
	{
		const std::basic_string_view namesv = {name.data( ),name.size( )};
		return std::invoke(*this, namesv, check_whole_path);
	}

	template<typename T>
	LDR_DATA_TABLE_ENTRY* operator()(const std::basic_string_view<T> name, const bool check_whole_path) const noexcept
	{
		const string_converter cvt = name;
		return std::invoke(*this, cvt.convert<WCHAR>( ), check_whole_path);
	}

	template<>
	LDR_DATA_TABLE_ENTRY* operator()(const std::basic_string_view<WCHAR> name, const bool check_whole_path) const noexcept
	{
		const auto comparer = [=](LDR_DATA_TABLE_ENTRY* const ldr_entry, IMAGE_NT_HEADERS*, IMAGE_DOS_HEADER*) noexcept
		{
			const std::basic_string_view full_path = {ldr_entry->FullDllName.Buffer, ldr_entry->FullDllName.Length / sizeof(WCHAR)};
			if (check_whole_path)
				return full_path == name;
			if (!full_path.ends_with(name) || *std::next(full_path.rbegin( ), name.size( )) != '\\')
				return false;
			return true;
		};

		return std::invoke(*this, std::ref(comparer));
	}
};

constexpr module_finder _Find_module;

//block = {(uint8_t*)DOS( ), NT( )->OptionalHeader.SizeOfImage};

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::wstring_view name, const bool check_whole_path) noexcept
{
	return _Find_module(name, check_whole_path);
}

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::string_view name, const bool check_whole_path) noexcept
{
	return _Find_module(name, check_whole_path);
}

#pragma endregion

#pragma region current_module_data

current_module_data::current_module_data(LDR_DATA_TABLE_ENTRY* const entry)
	:entry_(entry)
{
}

auto current_module_data::operator->( ) const noexcept -> pointer
{
	return entry_;
}

auto current_module_data::operator*( ) const noexcept -> reference
{
	return *entry_;
}

auto current_module_data::operator[](ptrdiff_t offset) const noexcept -> reference
{
	return entry_[offset];
}

current_module_data::operator std::wstring_view( ) const noexcept
{
	static const auto name = _Get_module_name<wchar_t>(entry_);
	return name;
}

current_module_data::operator std::string_view( ) const noexcept
{
	static const auto name = _Get_module_name<char>(entry_);
	return name;
}

#pragma endregion

static DECLSPEC_NOINLINE HMODULE _Get_current_module_handle( ) noexcept
{
	MEMORY_BASIC_INFORMATION info;
	constexpr SIZE_T info_size = sizeof(MEMORY_BASIC_INFORMATION);

	//todo: is this is dll, try to load this function from inside
	[[maybe_unused]]
	const auto len = VirtualQueryEx(GetCurrentProcess( ), _Get_current_module_handle, std::addressof(info), info_size);
	runtime_assert(len == info_size, "Wrong size");
	return static_cast<HMODULE>(info.AllocationBase);
}

using namespace nstd;

current_module_data winapi::current_module( ) noexcept
{
	static const auto ret = _Find_module([base_address = _Get_current_module_handle( )](LDR_DATA_TABLE_ENTRY*, IMAGE_NT_HEADERS*, IMAGE_DOS_HEADER* const dos) noexcept
	{
		return base_address == reinterpret_cast<HMODULE>(dos);
	});
	return ret;
}

module_name_str winapi::get_module_name(LDR_DATA_TABLE_ENTRY* const ldr_entry) noexcept
{
	return _Get_module_name(ldr_entry);
}


