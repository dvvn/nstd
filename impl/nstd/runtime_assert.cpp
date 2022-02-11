#include "runtime_assert_impl.h"

#include <algorithm>
#include <stdexcept>
#include <variant>
#include <vector>
#include <mutex>
#undef NDEBUG
#include <cassert>

using namespace nstd;

template<typename C>
static auto _End(const C* ptr)
{
	return ptr + std::char_traits<C>::length(ptr);
}

template<typename T, typename C>
static auto _To_string(const C* str)
{
	return std::basic_string<T>(str, _End(str));
}

template<typename S, typename C>
static void _Append(S& str, const C* ptr)
{
	str.append(ptr, _End(ptr));
}

template<typename S, typename C>
static void _Append(S& str, C chr)
{
	str += static_cast<S::value_type>(chr);
}

template<typename C>
class msg_packed
{
public:

	using string_type = std::basic_string<C>;
	using pointer_type = const C*;

	msg_packed( ) = default;

	msg_packed(string_type&& str) :data_(std::move(str)) { }
	msg_packed(pointer_type ptr) :data_(ptr) { }

	template<typename T>
	msg_packed(const T* ptr) : data_(_To_string<C>(ptr))
	{
		static_assert(sizeof(T) < sizeof(C));
	}

	operator pointer_type ( )const
	{
		return std::visit([]<typename T>(const T & obj)
		{
			if constexpr (std::is_class_v<T>)
				return obj.c_str( );
			else
				return obj;
		}, data_);
	}

private:
	std::variant<string_type, pointer_type>data_;
};

template<typename C>
msg_packed(const C*)->msg_packed<C>;

template<typename C>
static msg_packed<C> _Assert_msg(const char* expression, const char* message)
{
	if (!expression)
		return message;
	if (!message)
		return expression;

	std::basic_string<C> msg;
	_Append(msg, message);
	_Append(msg, " (");
	_Append(msg, expression);
	_Append(msg, ')');
	return msg;
}

static void _Assert(const char* expression, const char* message, const std::source_location& location) noexcept
{
#if defined(_MSC_VER)
	_wassert(_Assert_msg<wchar_t>(expression, message), msg_packed<wchar_t>(location.file_name( )), location.line( ));
#elif defined(__GNUC__)
	__assert_fail(_Assert_msg<char>(expression, message), location.file_name( ), location.line( ), location.function_name( ));
#else
	TODO
#endif
}

struct default_assert_handler final : rt_assert_handler
{
	void handle(const char* expression, const char* message, const std::source_location& location) noexcept override
	{
		_Assert(expression, message, location);
	}

	void handle(const char* message, const std::source_location& location) noexcept override
	{
		_Assert(nullptr, message, location);
	}
};

template<class T>
concept class_only = std::is_class_v<T>;

struct rt_assert_handler_root::entry
{
	rt_assert_handler* get( ) const
	{
		return std::visit([]<typename T>(const T & obj)
		{
			if constexpr (std::is_class_v<T>)
				return obj.get( );
			else
				return obj;
		}, data_);
	}

	rt_assert_handler* operator->( ) const
	{
		return get( );
	}

	entry(handler_unique&& h) :data_(std::move(h)) { }
	entry(const handler_shared& h) :data_(h) { }
	entry(handler_ptr h) :data_(h) { }

private:
	std::variant<handler_unique, handler_shared, handler_ptr> data_;
};

struct rt_assert_handler_root::impl
{
	std::mutex mtx;
	std::vector<entry> storage;
};

rt_assert_handler_root::rt_assert_handler_root( )
{
	impl_ = std::make_unique<impl>( );
	//add nolock
	impl_->storage.emplace_back(std::make_unique<default_assert_handler>( ));
}

rt_assert_handler_root::~rt_assert_handler_root( ) = default;

template <class Mtx, class Rng, typename T>
static void _Add_impl(Mtx & mtx, Rng & storage, T && handler)
{
	const auto lock = std::scoped_lock(mtx);
#ifdef _DEBUG
	if (!storage.empty( ))
	{
		const size_t id = handler->id( );
		for (const auto& el : storage)
		{
			if (el->id( ) == id)
				throw std::logic_error("Handler with given id already exists!");
		}
	}
#endif
	storage.emplace_back(std::forward<T>(handler));
}

void rt_assert_handler_root::add(handler_unique && handler)
{
	_Add_impl(impl_->mtx, impl_->storage, std::move(handler));
}

void rt_assert_handler_root::add(const handler_shared & handler)
{
	_Add_impl(impl_->mtx, impl_->storage, handler);
}

void rt_assert_handler_root::add(handler_ptr handler)
{
	_Add_impl(impl_->mtx, impl_->storage, handler);
}

void rt_assert_handler_root::remove(size_t id)
{
	const auto lock = std::scoped_lock(impl_->mtx);
	auto& s = impl_->storage;
	const auto end = s.end( );
	for (auto itr = s.begin( ); itr != end; ++itr)
	{
		if (id == (*itr)->id( ))
		{
			s.erase(itr);
			break;
		}
	}
}

template<class Mtx, class Rng, typename ...Args>
static void _Handle(Mtx & mtx, Rng & storage, Args...args)
{
	const auto lock = std::scoped_lock(mtx);
	for (const auto& el : storage)
		el->handle(args...);
}

void rt_assert_handler_root::handle(bool expression_result, const char* expression, const char* message, const std::source_location & location) const noexcept
{
	if (expression_result)
		return;
	_Handle(impl_->mtx, impl_->storage, expression, message, location);
}

void rt_assert_handler_root::handle(const char* message,
									[[maybe_unused]] const char* unused1, [[maybe_unused]] const char* unused2
									, const std::source_location & location) const noexcept
{
	_Handle(impl_->mtx, impl_->storage, message, location);
}
