#pragma once
#include "runtime_assert_fwd.h"

#include <chrono>

namespace nstd
{
	class timer
	{
	public:
		using clock_type = std::chrono::steady_clock;
		using time_point = clock_type::time_point;

		timer(bool start = false);

		bool started() const;
		bool updated() const;

		void set_start();
		void set_end();

		time_point::duration elapsed() const;

	private:
		std::optional<time_point> start_, end_;
	};

	class benchmark_timer final : protected timer
	{
	public:
		benchmark_timer() = default;

		template <class Fn, typename ...Args>
		benchmark_timer(Fn&& fn, Args&&...args)
			requires(std::invocable<decltype(fn), decltype(args)>)
		{
			this->work(std::forward<Fn>(fn), std::forward<Args>(args)...);
		}

		template <class Fn, typename ...Args>
		void work(Fn&& fn, Args&&...args)
			requires(std::invocable<decltype(fn), decltype(args)>)
		{
			this->set_start( );
			std::invoke((fn), std::forward<Args>(args)...);
			this->set_end( );
		}

		template <class Fn, typename ...Args>
		void work(size_t count, Fn&& fn, Args&&...args)
			requires(std::invocable<decltype(fn), decltype(args)>)
		{
			runtime_assert(count == 0, "bad count found");
			if (count == 1)
				return this->work((fn), std::forward<Args>(args)...);;
			this->set_start( );
			while (count--)
				std::invoke((fn), (args)...);
			this->set_end( );
		}

		using timer::elapsed;
		using timer::updated;
	};

#if 0
	template <typename ...Args>
	auto benchmark_invoke(Args&&...args) -> std::chrono::nanoseconds
	{
		benchmark_timer timer;
		timer.work(std::forward<Args>(args)...);
		return timer.elapsed( );
	}
#endif
}
