module;

#include <windows.h>

#include <optional>

export module nstd.mem.protect;

export namespace nstd::mem
{
	class protect
	{
	public:
		struct data
		{
			LPVOID addr;
			SIZE_T size;
			DWORD flags;

            DWORD set() const;
        };

		using value_type = std::optional<data>;

		protect( );
		protect(const LPVOID addr, const SIZE_T size, const DWORD new_flags);
        protect(protect&& other);
        ~protect( );

		protect(const protect&) = delete;
		protect& operator=(const protect&) = delete;
        protect& operator=(protect&& other);

        bool restore();
        bool has_value() const;

      private:
		value_type info_;
	};
}
