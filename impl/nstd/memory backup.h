#pragma once
#include <optional>

namespace nstd
{
	template <std::copyable T>
	class memory_backup
	{
	public:
		struct value_type
		{
			std::reference_wrapper<T> owner;
			T value;
		};

		memory_backup(const memory_backup& other)            = delete;
		memory_backup& operator=(const memory_backup& other) = delete;

		memory_backup(memory_backup&& other) noexcept
		{
			*this = std::move(other);
		}

		memory_backup& operator=(memory_backup&& other) noexcept
		{
			std::swap(this->data_, other.data_);
			return *this;
		}

		memory_backup() = default;

		memory_backup(T& from)
		{
			data_.emplace(std::ref(from), from);
		}

		template <typename T1>
			requires(std::constructible_from<T, T1>)
		memory_backup(T& from, T1&& owerride)
			: memory_backup(from)
		{
			from = T(std::forward<T1>(owerride));
		}

	private:
		template <bool FromDestructor>
		void restore_impl()
		{
			if (data_.has_value( ))
			{
				auto& d        = *data_;
				d.owner.get( ) = std::move(d.value);
				if constexpr (!FromDestructor)
					data_.reset( );
			}
		}

	public:
		~memory_backup()
		{
			this->restore_impl<true>( );
		}

		void restore()
		{
			this->restore_impl<false>( );
		}

		void reset()
		{
			data_.reset( );
		}

		/**
		 * \brief added to prevent create varianle-holder. create backup and call this function from fucntion paramter
		 */
		template <typename T1>
		_NODISCARD T1 val(T1&& val)
		{
			(void)this;
			return val;
		}

		bool has_value() const
		{
			return data_.has_value( );
		}

	private:
		std::optional<value_type> data_;
	};
}
