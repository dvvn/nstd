#pragma once

#include <wrl/client.h>

#include <concepts>

namespace nstd::winapi
{
	template <typename T>
	class comptr : public Microsoft::WRL::ComPtr<T>
	{
		using _Base = Microsoft::WRL::ComPtr<T>;

	public:
		using _Base::_Base;
		using _Base::operator=;

		//it calls unwanted addref method
		comptr(T* ptr) = delete;
		comptr& operator=(T* ptr) = delete;

		template <typename T1>
			requires(std::derived_from<T, T1>)
		operator T1* () const noexcept
		{
			return _Base::Get( );
		}

		template <typename T1>
			requires(std::derived_from<T, T1>)
		operator T1** () noexcept
		{
			return _Base::ReleaseAndGetAddressOf( );
		}

		template <typename T1>
			requires(std::derived_from<T, T1>)
		operator T1* const* () const noexcept
		{
			return _Base::GetAddressOf( );
		}

		explicit operator bool( ) const
		{
			return !!_Base::Get( );
		}

		bool operator!( ) const
		{
			return !_Base::Get( );
		}
	};
}