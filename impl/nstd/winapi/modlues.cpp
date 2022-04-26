module;

#include <nstd/runtime_assert.h>

#include <windows.h>
#include <winternl.h>

#include <string_view>
#include <functional>

module nstd.winapi.modules;
import nstd.winapi.module_info;
import nstd.mem.address;

using namespace nstd;

static auto _Get_ldr( ) noexcept
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

template<typename Fn>
class partial_invoke
{
	template<class Tpl, size_t ...I>
	static constexpr auto _Get_valid_seq(const std::index_sequence<I...> seq) noexcept
	{
		if constexpr(std::invocable<Fn, std::tuple_element_t<I, Tpl>...>)
			return seq;
		else if constexpr(seq.size( ) > 0)
			return _Get_valid_seq<Tpl>(std::make_index_sequence<seq.size( ) - 1>( ));
		else
			return std::false_type( );
	}

	template< class Tpl, size_t I1 = -1, size_t ...I>
	static constexpr auto _Get_valid_seq_reversed(const std::index_sequence<I1, I...> seq = {}) noexcept
	{
		if constexpr(I1 == -1)
			return std::false_type( );
		else if constexpr(std::invocable<Fn, std::tuple_element_t<I1, Tpl>, std::tuple_element_t<I, Tpl>...>)
			return seq;
		else
			return _Get_valid_seq_reversed<Tpl, I...>( );
	}

	template<class Tpl, size_t ...I>
	auto apply(Tpl& tpl, const std::index_sequence<I...>) const noexcept
	{
		return std::invoke(fn_, std::get<I>(tpl)...);
	}

	template<class Tpl, class SeqFwd, class SeqBack>
	auto apply(Tpl& tpl, const SeqFwd seq_fwd, const SeqBack seq_back) const noexcept
	{
		constexpr auto fwd_ok = !std::same_as<SeqFwd, std::false_type>;
		constexpr auto bk_ok = !std::same_as<SeqBack, std::false_type>;

		static_assert((fwd_ok || bk_ok) && !(fwd_ok && bk_ok), "Unable to choice the sequence!");

		if constexpr(fwd_ok)
			return this->apply<Tpl>(tpl, seq_fwd);
		else
			return this->apply<Tpl>(tpl, seq_back);
	}

public:

	template<typename FnRef>
	partial_invoke(FnRef&& fn)
		:fn_(std::forward<FnRef>(fn))
	{
	}

	template<typename ...Args>
	auto operator()(Args&&...args) const noexcept
	{
		auto tpl = std::forward_as_tuple(std::forward<Args>(args)...);
		using tpl_t = decltype(tpl);

		constexpr std::make_index_sequence<sizeof...(Args)> seq_def;
		constexpr auto seq = _Get_valid_seq<tpl_t>(seq_def);
		constexpr auto seq_reversed = _Get_valid_seq_reversed<tpl_t>(seq_def);

		return this->apply(tpl, seq, seq_reversed);
	}

private:
	Fn fn_;
};

template<typename Fn>
partial_invoke(Fn&&)->partial_invoke<std::remove_cvref_t<Fn>>;

template<typename Fn>
static LDR_DATA_TABLE_ENTRY* _Find_module(Fn comparer) noexcept
{
	const partial_invoke invoker = std::move(comparer);
	const auto ldr = _Get_ldr( );
	// get module linked list.
	const auto list = std::addressof(ldr->InMemoryOrderModuleList);
	// iterate linked list.
	for(auto it = list->Flink; it != list; it = it->Flink)
	{
		// get current entry.
		const auto ldr_entry = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if(!ldr_entry)
			continue;

		//base address
		const basic_address<IMAGE_DOS_HEADER> _dos = ldr_entry->DllBase;
		// check for invalid DOS / DOS signature.
		if(!_dos || _dos->e_magic != IMAGE_DOS_SIGNATURE /* 'MZ' */)
			continue;

		const basic_address<IMAGE_NT_HEADERS> _nt = _dos + _dos->e_lfanew;
		// check for invalid NT / NT signature.
		if(!_nt || _nt->Signature != IMAGE_NT_SIGNATURE /* 'PE\0\0' */)
			continue;

		if(!invoker(ldr_entry, _nt, _dos))
			continue;

		return ldr_entry;
	}

	return nullptr;
}

static LDR_DATA_TABLE_ENTRY* _Find_module(const std::wstring_view name, const bool check_whole_path) noexcept
{
	return _Find_module([=](const winapi::module_info info) noexcept
	{
		const auto full_path = info.path( );
		if(check_whole_path)
			return full_path == name;
		if(!full_path.ends_with(name) || *std::next(full_path.rbegin( ), name.size( )) != '\\')
			return false;
		return true;
	});
}

LDR_DATA_TABLE_ENTRY* find_module_impl(const std::wstring_view name, const bool check_whole_path) noexcept
{
	return _Find_module(name, check_whole_path);
}

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

LDR_DATA_TABLE_ENTRY* winapi::current_module( ) noexcept
{
	static const auto ret = _Find_module([base_address = _Get_current_module_handle( )](IMAGE_DOS_HEADER* const dos) noexcept
	{
		return base_address == reinterpret_cast<HMODULE>(dos);
	});
	return ret;
}

