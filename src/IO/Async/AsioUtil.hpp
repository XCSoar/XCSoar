/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_ASIO_UTIL_HPP
#define XCSOAR_ASIO_UTIL_HPP

#include "Thread/Mutex.hpp"
#include "Thread/Cond.hxx"

#include <boost/asio.hpp>

/**
 * Invoke the specified function in a boost::asio context and wait for
 * its completion.
 */
template<typename F>
void
DispatchWait(boost::asio::io_service &io_service, F &&f)
{
  if (io_service.stopped()) {
    /* the io_service is not running, probably because we're shutting
       down; running the function inside this thread is the best we
       can do to avoid a deadlock; that is not completely safe, but
       safe for our use cases */
    f();
    return;
  }

  Mutex mutex;
  Cond cond;
  bool finished = false;

  io_service.dispatch([&](){
      f();

      ScopeLock lock(mutex);
      finished = true;
      cond.signal();
    });

  ScopeLock lock(mutex);
  while (!finished)
    cond.wait(mutex);
}

/**
 * Cancel the specified object wait for its completion.
 */
template<typename T>
void
CancelWait(boost::asio::io_service &io_service, T &t)
{
  DispatchWait(io_service, [&](){
      t.cancel();
    });
}

/**
 * Cancel the specified object wait for its completion.
 */
template<typename T>
void
CancelWait(T &t)
{
  CancelWait(t.get_io_service(), t);
}

#endif
