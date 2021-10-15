#pragma once

#include "smooth_value_fwd.h"

#include <chrono>

// ReSharper disable once CppUnusedIncludeDirective
//#include "universal_operators.h"

namespace nstd
{
	class smooth_object_base
	{
	protected:
		virtual ~smooth_object_base() = default;

	public:
		enum class state :uint8_t
		{
			IDLE
			, RESTARTED_DELAYED
			, RESTARTED
			, STARTED
			, RUNNING
			, FINISHED
		};

		virtual bool update() = 0;
	};

	template <class T, class Clock>
	class smooth_value_base : public smooth_object_base
	{
		struct target
		{
			virtual ~target() = default;
			virtual void write_value(T& obj) = 0;
			virtual T& get_value() = 0;
			virtual bool own_value() const = 0;
		};

	public:
		class target_external final : public target
		{
		public:
			target_external() = default;

			void write_value(T& obj) override { target_val_ = std::addressof(obj); }
			T& get_value() override { return *target_val_; }
			bool own_value() const override { return false; }

		private:
			T* target_val_ = nullptr;
		};

		class target_internal final : public target
		{
		public:
			target_internal() = default;

			void write_value(T& obj) override { target_val_ = (obj); }
			T& get_value() override { return target_val_; }
			bool own_value() const override { return true; }

		private:
			T target_val_;
		};

		//--------

		using duration = typename Clock::duration;
		using time_point = typename Clock::time_point;

		using value_type = T;
		using unachieved_value = std::optional<T>;

		using target_holder = std::unique_ptr<target>;

		~smooth_value_base() override
		{
			if (!target_val_ || !this->active())
				return;
			if (!target_val_->own_value())
				target_val_->write_value(end_val_);
		}

		smooth_value_base()
		{
			static_assert(std::copyable<T>);
			static_assert(std::chrono::is_clock_v<Clock>);
			start_val_ = temp_val_ = end_val_ = T();
		}

		smooth_value_base(const smooth_value_base& other) = delete;
		smooth_value_base(smooth_value_base&& other) noexcept = default;
		smooth_value_base& operator=(const smooth_value_base& other) = delete;
		smooth_value_base& operator=(smooth_value_base&& other) noexcept = default;

		//----

		void set_target(target_holder&& holder)
		{
			target_val_ = std::move(holder);
		}

		template <std::derived_from<target> Q>
		void set_target()
		{
			set_target(std::make_unique<Q>());
		}

		target* get_target() const
		{
			return target_val_.get();
		}

		//----

		void set_start(const T& start)
		{
			start_val_ = start;
		}

		void set_end(const T& end)
		{
			end_val_ = end;
		}

		const T& get_start_val() const
		{
			return start_val_;
		}

		const T& get_end_val() const
		{
			return end_val_;
		}

		void decrease_time() {
			const auto elapsed_time = last_time_ - start_time_;
			const auto time_remaining = duration_ - elapsed_time;

			//some time has passed, now we only need to wait for this segment
			start_time_ = last_time_ - time_remaining;
		}

		void restart_or_decrease_time(bool delayed_restart) {
			if (!this->active())
				restart(delayed_restart);
			else
				decrease_time();
		}

		void set_new_range(const T& new_end, bool update_force = false)
		{
			constexpr auto equal = [](const T& a, const T& b)
			{
				if constexpr (std::equality_comparable<T>)
					return a == b;
				else if constexpr (std::is_trivially_destructible_v<T>)
					return std::memcmp(std::addressof(a), std::addressof(b), sizeof(T)) == 0;
			};

			if (!update_force && equal(new_end, end_val_))
				return;

			if (!this->active() || !equal(start_val_, new_end))
			{
				start_val_ = std::move(end_val_);
				end_val_ = new_end;
				temp_val_old_ = temp_val_;
			}
			else
			{
				decrease_time();
				inverse();
			}

			this->start(true, true);
		}

		void set_duration(duration duration)
		{
			duration_ = duration;
		}

		//----

		void start(bool wait_for_finish, bool delayed)
		{
			if (active() && wait_for_finish)
				return;
			/*if (target_val_ == nullptr)
				set_target<target_internal>( );*/
			restart(delayed);
		}

		state get_state() const
		{
			return state_;
		}

		bool active() const
		{
			switch (state_)
			{
			case state::IDLE:
			case state::FINISHED:
				return false;
			default:
				return true;
			}
		}

		void restart(bool delayed)
		{
			if (delayed)
			{
				state_ = state::RESTARTED_DELAYED;
				return;
			}

			temp_val_ = start_val_;
			state_ = state::RESTARTED;
			temp_val_old_.reset();
			start_time_ = Clock::now();
		}

		void inverse()
		{
			std::swap(start_val_, end_val_);
		}

		//---

		bool update() final
		{
			switch (state_)
			{
			case state::RESTARTED_DELAYED:
				this->restart(false);
				break;
			case state::FINISHED:
				state_ = state::IDLE;
			case state::IDLE:
				return false;
			}

			duration frame_time, elapsed_time;

			if (state_ == state::RESTARTED)
			{
				state_ = state::STARTED;
				frame_time = elapsed_time = duration::zero();
				last_time_ = start_time_;
				if (!target_val_->own_value())
				{
					//force if value externally changed
					target_val_->get_value() = temp_val_old_.value_or(start_val_);
				}
				return false;
			}
			else
			{
				if (state_ == state::STARTED)
					state_ = state::RUNNING;

				auto now = Clock::now();
				frame_time = now - last_time_;
				elapsed_time = now - start_time_;
				last_time_ = now;
			}

			if (elapsed_time >= duration_)
			{
				state_ = state::FINISHED;
				target_val_->get_value() = end_val_;
			}
			else
			{
				this->update_impl(temp_val_old_, start_val_, temp_val_, end_val_, frame_time, elapsed_time, duration_);
				target_val_->get_value() = temp_val_;
			}

			return true;
		}

	protected:
		virtual void update_impl(const unachieved_value& current_unachieved, const T& from, T& current, const T& to,
			duration frame_time, duration elapsed_time, duration duration) = 0;

	private:
		time_point start_time_, last_time_;
		duration duration_;
		target_holder target_val_;
		value_type start_val_, temp_val_, end_val_;
		unachieved_value temp_val_old_;
		state state_ = state::IDLE;
	};

	template <typename T, class Clock>
	class smooth_value_linear : public smooth_value_base<T, Clock>
	{
	protected:
		using base = smooth_value_base<T, Clock>;
		using duration = typename base::duration;
		using unachieved_value = typename base::unachieved_value;

		void update_impl(const unachieved_value& current_old, const T& from, T& current, const T& to,
			duration frame_time, duration elapsed_time, duration duration) override
		{
			const auto diff = static_cast<float>(elapsed_time.count()) / static_cast<float>(duration.count());
			current = std::lerp(current_old.value_or(from), to, diff);
		}
	};
}
