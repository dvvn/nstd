#include <nstd/timer.h>

namespace nstd
{
	timer::timer(bool start)
	{
		if (start)
			this->set_start( );
	}

	bool timer::started( ) const
	{
		return start_.has_value( );
	}

	bool timer::updated( ) const
	{
		return end_.has_value( );
	}

	void timer::set_start( )
	{
		start_.emplace(clock_type::now( ));
		end_.reset( );
	}

	void timer::set_end( )
	{
		runtime_assert(started( ), "Timer not started");
		end_.emplace(clock_type::now( ));
	}

	timer::time_point::duration timer::elapsed( ) const
	{
		runtime_assert(updated( ), "Timer not updated");
		return *end_ - *start_;
	}
}
