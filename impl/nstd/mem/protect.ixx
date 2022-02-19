module;

#include <windows.h>
#include <optional>

export module nstd.mem:protect;

export namespace nstd::mem
{
	class protect
	{
	public:
		struct value_type
		{
			LPVOID addr;
			SIZE_T size;
			DWORD flags;
		};

		protect(const protect&) = delete;
		protect& operator=(const protect&) = delete;

		protect(protect&& other) noexcept;
		protect& operator=(protect&& other) noexcept;

		protect( ) = default;

		protect(LPVOID addr, SIZE_T size, DWORD new_flags);

		~protect( );

		bool restore( );
		bool has_value( ) const;

	private:
		std::optional<value_type> info_;
	};
}
