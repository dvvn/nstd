module;

#include "comptr_includes.h"

export module nstd.winapi.comptr;

export namespace nstd::winapi
{
	template <typename T, typename Base = Microsoft::WRL::ComPtr<T>>
	struct comptr : Base
	{
		using Base::Base;
		using Base::operator=;

		//it calls unwanted addref method
		comptr(T* ptr) = delete;
		comptr& operator=(T* ptr) = delete;
		
		template <typename T1>
			requires(std::derived_from<T, T1>)
		operator T1* () const noexcept
		{
			return Base::Get( );
		}

		template <typename T1>
			requires(std::derived_from<T, T1>)
		operator T1** () noexcept
		{
			return Base::ReleaseAndGetAddressOf( );
		}

		template <typename T1>
			requires(std::derived_from<T, T1>)
		operator T1* const* () const noexcept
		{
			return Base::GetAddressOf( );
		}

		explicit operator bool( ) const
		{
			return !!Base::Get( );
		}

		bool operator!( ) const
		{
			return !Base::Get( );
		}
	};
}