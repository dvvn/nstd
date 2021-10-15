#include <nstd/os/threads.h>
#include <nstd/runtime_assert_fwd.h>

// ReSharper disable CppWrongIncludesOrder
#include <Windows.h>
#include <TlHelp32.h>
// ReSharper restore CppWrongIncludesOrder

#include <vector>

using namespace nstd::os;

template <typename T>
static void _Open_assert([[maybe_unused]] const T* ptr)
{
	runtime_assert(ptr->is_open(), "Thread isn't open!");
}

thread_entry::thread_entry(const THREADENTRY32& entry)
{
	static_assert(sizeof(decltype(entry_)) == sizeof(THREADENTRY32));
	reinterpret_cast<THREADENTRY32&>(entry_) = entry;
}

bool thread_entry::open(DWORD access)
{
	this->close( );
	handle_ = OpenThread(access, FALSE, reinterpret_cast<THREADENTRY32&>(entry_).th32ThreadID);
	return this->is_open( );
}

bool thread_entry::close( )
{
	if (!this->is_open( ))
		return false;

	handle_.reset( );
	return true;
}

HANDLE thread_entry::release_handle( )
{
	_Open_assert(this);
	return handle_.release( );
}

HANDLE thread_entry::get_handle( ) const
{
	return handle_.get( );
}

bool thread_entry::is_open( ) const
{
	return handle_ != nullptr;
}

bool thread_entry::is_paused( ) const
{
	_Open_assert(this);
	return WaitForSingleObject(handle_.get( ), 0) == WAIT_ABANDONED;
}

void threads_enumerator::operator()(DWORD process_id)
{
	if (process_id == 0)
		process_id = GetCurrentProcessId( );

	const handle snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, process_id);
	if (snapshot == nullptr)
	{
		runtime_assert("Unable to create snapshot!");
		return;
	}

	auto entry = THREADENTRY32( );

	const auto set_entry_size = [&]
	{
		entry.dwSize = sizeof(THREADENTRY32);
	};
	const auto thread32_first = [&]
	{
		set_entry_size( );
		return Thread32First(snapshot.get( ), std::addressof(entry));
	};
	const auto thread32_next = [&]
	{
		set_entry_size( );
		return Thread32Next(snapshot.get( ), std::addressof(entry));
	};

	constexpr auto min_size = FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(decltype(THREADENTRY32::th32OwnerProcessID));
	const auto my_pid       = process_id;
	const auto my_thread    = GetCurrentThreadId( );

	for (auto active = thread32_first( ); active == TRUE; active = thread32_next( ))
	{
		if (entry.dwSize < min_size)
			continue;
		if (entry.th32OwnerProcessID != my_pid)
			continue;
		if (entry.th32ThreadID == my_thread)
			continue;

		this->on_valid_thread(entry);
	}
}

frozen_threads_storage::frozen_threads_storage(frozen_threads_storage&& other) noexcept
{
	storage_ = std::move(other.storage_);
}

frozen_threads_storage& frozen_threads_storage::operator=(frozen_threads_storage&& other) noexcept
{
	std::swap(storage_, other.storage_);
	return *this;
}

void unfreeze_thread::operator( )(HANDLE h) const
{
	ResumeThread(h);
	CloseHandle(h);
}

frozen_thread::frozen_thread(HANDLE handle)
	: frozen_thread_restorer(handle)
{
	SuspendThread(handle);
}

struct frozen_threads_storage::storage_type : std::vector<frozen_thread>
{
};

frozen_threads_storage::frozen_threads_storage(bool fill)
{
	if (fill)
		this->fill( );
}

frozen_threads_storage::~frozen_threads_storage( ) = default;

void frozen_threads_storage::fill( )
{
	if (storage_ == nullptr)
	{
		storage_ = std::make_unique<storage_type>( );
		std::invoke(*static_cast<threads_enumerator*>(this));
	}
}

void frozen_threads_storage::clear( )
{
	storage_ = {};
}

void frozen_threads_storage::on_valid_thread(const THREADENTRY32& entry)
{
	thread_entry te = entry;

	constexpr auto flags = (THREAD_SUSPEND_RESUME | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT);

	if (!te.open(flags) || te.is_paused( ))
		return;

	storage_->push_back(te.release_handle( ));
}
