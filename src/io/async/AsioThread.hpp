/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_ASIO_THREAD_HPP
#define XCSOAR_ASIO_THREAD_HPP

#include "thread/Thread.hpp"
#include "event/Loop.hxx"

/**
 * A thread which runs a boost::asio::io_context.
 */
class AsioThread final : protected Thread {
  EventLoop event_loop{ThreadId::Null()};

public:
  AsioThread():Thread("asio") {}

  /**
   * Start the thread.  This method should be called after creating
   * this object.
   *
   * Throws on error.
   */
  void Start();

  /**
   * Stop the thread.  This method must be called before the
   * destructor.
   */
  void Stop();

  auto &GetEventLoop() noexcept {
    return event_loop;
  }

  operator EventLoop &() noexcept {
    return event_loop;
  }

protected:
  /* virtual methods from Thread */
  void Run() noexcept override;
};

#endif
