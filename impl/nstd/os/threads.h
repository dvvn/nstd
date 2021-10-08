#pragma once
#include <nstd/os/handle.h>

//ReSharper disable  CppInconsistentNaming
struct tagTHREADENTRY32;
using THREADENTRY32 = tagTHREADENTRY32;
using DWORD = unsigned long;
using LONG = long;
//ReSharper restore CppInconsistentNaming
//#include <TlHelp32.h>

namespace nstd::os
{
	class thread_entry
	{
	public:
		thread_entry(const THREADENTRY32& entry);

		//THREAD_***
		bool open(DWORD access);
		bool close( );

		[[nodiscard]]
		HANDLE release_handle( );
		HANDLE get_handle( ) const;

		bool is_open( ) const;
		bool is_paused( ) const;

		/*
		DWORD   dwSize;
		DWORD   cntUsage;
		DWORD   th32ThreadID;       // this thread
		DWORD   th32OwnerProcessID; // Process this thread is associated with
		LONG    tpBasePri;
		LONG    tpDeltaPri;
		DWORD   dwFlags;
		*/
	private:
		uint8_t entry_[sizeof(DWORD) * 5 + sizeof(LONG) * 2];
		handle  handle_;
	};

	class threads_enumerator
	{
	public:
		virtual ~threads_enumerator( ) = default;
		void    operator()(DWORD process_id = 0);

	protected:
		virtual void on_valid_thread(const THREADENTRY32& entry) = 0;
	};

	class unfreeze_thread
	{
	public:
		using pointer = HANDLE;
		void operator()(HANDLE h) const;
	};

	using frozen_thread_restorer = std::unique_ptr<HANDLE, unfreeze_thread>;

	class frozen_thread : public frozen_thread_restorer
	{
	public:
		frozen_thread(HANDLE handle);
	};

	class frozen_threads_storage final : threads_enumerator
	{
	public:
		frozen_threads_storage(frozen_threads_storage&& other) noexcept;
		frozen_threads_storage& operator =(frozen_threads_storage&& other) noexcept;

		frozen_threads_storage(const frozen_threads_storage& other) = delete;
		frozen_threads_storage& operator =(const frozen_threads_storage& other) = delete;

		explicit frozen_threads_storage(bool fill);
		~frozen_threads_storage( ) override;

		void fill( );
		void clear( );

		struct storage_type;

	protected:
		void on_valid_thread(const THREADENTRY32& entry) override;

	private:
		std::unique_ptr<storage_type> storage_;
	};
}
