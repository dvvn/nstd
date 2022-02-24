module;

#include <functional>
#include <optional>

export module nstd.lazy_invoke;

template<typename Fn>
concept func_maybe_null = requires(const Fn fn)
{
	!fn;
};

export namespace nstd
{
	template<typename Fn>
	class lazy_invoke
	{
	public:
		using value_type = std::conditional_t<func_maybe_null<Fn>, Fn, std::optional<Fn>>;
		using func_type = Fn;

		constexpr ~lazy_invoke( )
		{
			if (!fn_)
				return;
			if constexpr (!func_maybe_null<Fn>)
				std::invoke(*fn_);
			else
				std::invoke(fn_);
		}

		template<typename Fn1>
		constexpr lazy_invoke(Fn1&& fn) :fn_(std::forward<Fn1>(fn))
		{
		}

		constexpr lazy_invoke(lazy_invoke&& other)noexcept
		{
			*this = std::move(other);
		}

		constexpr lazy_invoke& operator=(lazy_invoke&& other)noexcept
		{
			using std::swap;
			swap(fn_, other.fn_);
		}

	private:
		value_type fn_;
	};

	template<typename Fn>
	lazy_invoke(const Fn&)->lazy_invoke<Fn>;
}