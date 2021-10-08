#pragma once
#include "handle.h"
//#include "nstd/enum as struct.h"

//ReSharper disable  CppInconsistentNaming
struct tagTHREADENTRY32;
using THREADENTRY32 = tagTHREADENTRY32;
using DWORD = unsigned long;
using LONG = long;
//ReSharper restore CppInconsistentNaming
//#include <TlHelp32.h>

namespace nstd::os
{
	/*struct thread_access final
	{
		enum value_type : DWORD
		{
			terminate = THREAD_TERMINATE,
			suspend_resume = THREAD_SUSPEND_RESUME,
			get_context = THREAD_GET_CONTEXT,
			set_context = THREAD_SET_CONTEXT,
			query_information = THREAD_QUERY_INFORMATION,
			set_information = THREAD_SET_INFORMATION,
			set_thread_token = THREAD_SET_THREAD_TOKEN,
			impersonate = THREAD_IMPERSONATE,
			direct_impersonation = THREAD_DIRECT_IMPERSONATION,
			set_limited_information = THREAD_SET_LIMITED_INFORMATION,
			query_limited_information = THREAD_QUERY_LIMITED_INFORMATION,
			resume = THREAD_RESUME,
			all_access = THREAD_ALL_ACCESS,
		};

		NSTD_ENUM_STRUCT_BITFLAG(thread_access);
	};*/

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
		virtual void on_valid_thread(const THREADENTRY32& entry) =0;
	};

	class unfreeze_thread
	{
	public:
		using pointer = HANDLE;
		void operator()(HANDLE h) const;
	};

	using frozen_thread_restorer = std::unique_ptr<HANDLE, unfreeze_thread>;

	class frozen_thread: public frozen_thread_restorer
	{
	public:
		frozen_thread(HANDLE handle);
	};

	class frozen_threads_storage final: threads_enumerator
	{
	public:
		frozen_threads_storage(frozen_threads_storage&& other) noexcept;
		frozen_threads_storage& operator =(frozen_threads_storage&& other) noexcept;

		frozen_threads_storage(const frozen_threads_storage& other)             = delete;
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
