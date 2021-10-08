#pragma once

namespace std::chrono
{
	struct steady_clock;
}

namespace nstd
{
	class smooth_object_base;

	template <class T, class Clock = std::chrono::steady_clock>
	class smooth_value_base;

	template <class T, class Clock = std::chrono::steady_clock>
	class smooth_value_linear;
}
