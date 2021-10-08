#pragma once

#include <optional>

namespace nstd
{
	template <typename T>
	class confirmable_value
	{
		static size_t _Hash(const T& value)
		{
			return std::invoke(std::hash<T>( ), value);
		}

		void _Set(T&& value)
		{
			const auto hash = _Hash(value);
			if (hash_ == hash)
				return;

			std::swap(value_, value);
			hash_ = hash;
		}

		void _Construct(T&& value)
		{
			value_          = std::move(value);
			hash_           = _Hash(value_);
			hash_confiremd_ = ~hash_;
		}

	public:
		using value_type = T;

		confirmable_value(const T& value)
			requires(std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>)
		{
			auto copy = value;
			_Construct(std::move(copy));
		}

		confirmable_value(T&& value)
		{
			_Construct(std::move(value));
		}

		confirmable_value& operator=(T&& val)
		{
			_Set(std::move(val));
			return *this;
		}

		class setter_proxy
		{
		public:
			setter_proxy(confirmable_value* const owner)
				: owner_(owner)
			{
			}

			~setter_proxy()
			{
				if (!value_.has_value( ))
					return;

				if constexpr (std::is_trivially_destructible_v<T>)
				{
					owner_->_Set(std::move(*value_));
				}
				else
				{
					T out = std::move(*value_);
					static_assert(std::default_initializable<T>, __FUNCTION__": T isn't default initializable");
					value_.emplace(T( ));

					owner_->_Set(std::move(out));
				}
			}

			setter_proxy(const setter_proxy& other)            = delete;
			setter_proxy& operator=(const setter_proxy& other) = delete;

			setter_proxy(setter_proxy&& other) noexcept
				: value_(std::move(other.value_)),
				  owner_(other.owner_)
			{
				other.value_.reset( );
			}

			setter_proxy& operator=(setter_proxy&& other) noexcept
			{
				std::swap(value_, other.value_);
				std::swap(owner_, other.owner_);
				return *this;
			}

			operator const T&() const
			{
				return owner_->value_;
			}

			operator T&()
			{
				if (!value_.has_value( ))
					value_.emplace(owner_->value_);

				return *value_;
			}

		private:
			std::optional<T> value_;
			confirmable_value* owner_;
		};

		class setter_proxy_ptr
		{
		public:
			setter_proxy_ptr(setter_proxy&& proxy)
				: proxy_(std::move(proxy))
			{
			}

			setter_proxy& operator*()
			{
				return proxy_;
			}

			operator T*()
			{
				T& val = proxy_;
				return std::addressof(val);
			}

		private:
			setter_proxy proxy_;
		};

		setter_proxy_ptr operator&()
		{
			return setter_proxy(this);
		}

		operator const T&() const
		{
			return value_;
		}

		setter_proxy operator*()
		{
			return this;
		}

		void confirm()
		{
			hash_confiremd_ = hash_;
		}

		bool confirmed() const
		{
			return hash_ == hash_confiremd_;
		}

	private:
		T value_;
		size_t hash_, hash_confiremd_;
	};
}
