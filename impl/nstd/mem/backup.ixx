module;

#include <optional>

export module nstd.mem.backup;

export namespace nstd::mem
{
	template <std::copyable T>
	class backup
	{
		struct value_stored
		{
			T* owner;
			T value;
		};

	public:
		using value_type = T;

		backup(const backup& other)            = delete;
		backup& operator=(const backup& other) = delete;

		backup(backup&& other) noexcept
		{
			*this = std::move(other);
		}

		backup& operator=(backup&& other) noexcept
		{
			std::swap(backup_, other.backup_);
			return *this;
		}

		backup( ) = default;

		backup(T& from)
		{
			backup_.emplace(std::addressof(from), from);
		}

		template <typename T1>
			requires(std::constructible_from<T, T1>)
		backup(T& from, T1&& owerride)
			: backup(from)
		{
			from = T(std::forward<T1>(owerride));
		}

		const T& get( ) const { return backup_->value; }

	private:
		void restore_impl( )
		{
			auto& b = *backup_;
			std::swap(*b.owner, b.value);
		}

	public:
		~backup( )
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
