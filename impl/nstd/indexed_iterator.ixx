module;

#include <memory>
#include <concepts>

export module nstd.indexed_iterator;

struct smart_pointer_accepter
{
	template<typename ...T>
	smart_pointer_accepter(const std::unique_ptr<T...>&)
	{
	}

	template<typename ...T>
	smart_pointer_accepter(const std::shared_ptr<T...>&)
	{
	}

	template<typename ...T>
	smart_pointer_accepter(const std::weak_ptr<T...>&)
	{
	}
};

template<typename T>
concept smart_pointer = std::constructible_from<smart_pointer_accepter, T>;

template<typename T>
auto& _Get_ref(T& ref) noexcept
{
	if constexpr(smart_pointer<T>)
		return *ref;
	else
		return ref;
}

template<typename T>
auto _Get_ptr(T& ref) noexcept
{
	if constexpr(smart_pointer<T>)
		return ref.get( );
	else
		return std::addressof(ref);
}

export namespace nstd
{
	template<typename T>
	class indexed_iterator
	{
		using _ConstT = std::add_const_t<T>;
		using _OtherT = std::conditional_t<std::is_const_v<T>, std::remove_const_t<T>, _ConstT>;
		using _Const_data = decltype(std::declval<_ConstT>( ).data( ));

	public:
		friend class indexed_iterator<_OtherT>;

		indexed_iterator( ) = default;

		indexed_iterator(T& s, std::nullptr_t) = delete;

		indexed_iterator(T& s, const size_t off)
			:source_(std::addressof(s)), index_(off)
		{
		}

		indexed_iterator(T& s, _Const_data const ptr)
			:indexed_iterator(s, std::distance<_Const_data>(s.data( ), ptr))
		{
		}

		indexed_iterator(const indexed_iterator<_OtherT>& other)
			: source_(other.source_), index_(other.index_)
		{
		}

		auto _Unwrapped( ) const noexcept
		{
			return _Get_ptr(source_->data( )[index_]);
		}

		auto operator->( ) const noexcept
		{
			return _Unwrapped( );
		}

		auto& operator*( ) const noexcept
		{
			return _Get_ref(source_->data( )[index_]);
		}

		bool operator!( ) const noexcept
		{
			return source_ == nullptr;
		}

		explicit operator bool( ) const noexcept
		{
			return source_ != nullptr;
		}

		bool operator==(const indexed_iterator other) const noexcept
		{
			return source_ == other.source_ && index_ == other.index_;
		}

		// Prefix increment
		indexed_iterator& operator++( ) noexcept
		{
			++index_;
			return *this;
		}

		// Postfix increment
		indexed_iterator operator++(int) noexcept
		{
			return {source_, index_++};
		}

	private:
		T* source_ = nullptr;
		size_t index_ = static_cast<size_t>(-1);
	};

	template<class T>
	class to_indexed_iterators final : T
	{
		using T::begin;
		using T::end;

		//currently unsupported
		using T::cbegin;
		using T::cend;
		using T::rbegin;
		using T::rend;

	public:
		using iterator = indexed_iterator<T>;
		using const_iterator = indexed_iterator<const T>;

		using T::T;

		iterator begin( ) noexcept
		{
			return {*this,0u};
		}

		const_iterator begin( ) const noexcept
		{
			return {*this,0u};
		}

		iterator end( ) noexcept
		{
			return {*this,this->size( )};
		}

		const_iterator end( ) const noexcept
		{
			return {*this,this->size( )};
		}
	};
}