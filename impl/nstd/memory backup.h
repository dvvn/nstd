#pragma once

#include <optional>

namespace nstd
{
	template <std::copyable T>
	class memory_backup
	{
		struct value_stored
		{
			T* owner;
			T value;
		};

	public:
		using value_type = T;

		memory_backup(const memory_backup& other)            = delete;
		memory_backup& operator=(const memory_backup& other) = delete;

		memory_backup(memory_backup&& other) noexcept
		{
			*this = std::move(other);
		}

		memory_backup& operator=(memory_backup&& other) noexcept
		{
			std::swap(backup_, other.backup_);
			return *this;
		}

		memory_backup( ) = default;

		memory_backup(T& from)
		{
			backup_.emplace(std::addressof(from), from);
		}

		template <typename T1>
			requires(std::constructible_from<T, T1>)
		memory_backup(T& from, T1&& owerride)
			: memory_backup(from)
		{
			from = T(std::forward<T1>(owerride));
		}

	private:
		void restore_impl( )
		{
			auto& b  = *backup_;
			*b.owner = std::move(b.value);
		}

	public:
		~memory_backup( )
		{
			if (has_value( ))
				restore_impl( );
		}

		void restore( )
		{
			if (has_value( ))
			{
				restore_impl( );
				reset( );
			}
		}

		void reset( ) { backup_.reset( ); }
		bool has_value( ) const { return backup_.has_value( ); }

	private:
		std::optional<value_stored> backup_;
	};
}
