/*
 * Copyright 2003-2021 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_TIMER_EVENT_HXX
#define MPD_TIMER_EVENT_HXX

#include "Chrono.hxx"
#include "util/BindMethod.hxx"

#include <boost/intrusive/set_hook.hpp>

class EventLoop;

/**
 * This class invokes a callback function after a certain amount of
 * time.  Use Schedule() to start the timer or Cancel() to cancel it.
 *
 * This class is not thread-safe, all methods must be called from the
 * thread that runs the #EventLoop, except where explicitly documented
 * as thread-safe.
 */
class TimerEvent final
	: public boost::intrusive::set_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>
{
	friend class EventLoop;

	EventLoop &loop;

	using Callback = BoundMethod<void() noexcept>;
	const Callback callback;

	/**
	 * When is this timer due?  This is only valid if IsPending()
	 * returns true.
	 */
	Event::Clock::time_point due;

public:
	TimerEvent(EventLoop &_loop, Callback _callback) noexcept
		:loop(_loop), callback(_callback) {}

	auto &GetEventLoop() const noexcept {
		return loop;
	}

	bool IsPending() const noexcept {
		return is_linked();
	}

	void Schedule(Event::Duration d) noexcept;

	/**
	 * Like Schedule(), but is a no-op if there is a due time
	 * earlier than the given one.
	 */
	void ScheduleEarlier(Event::Duration d) noexcept;

	void Cancel() noexcept {
		unlink();
	}

private:
	void Run() noexcept {
		callback();
	}
};

#endif /* MAIN_NOTIFY_H */
