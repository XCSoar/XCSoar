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

#ifndef XCSOAR_SIGNAL_LISTENER_HPP
#define XCSOAR_SIGNAL_LISTENER_HPP

#include "OS/UniqueFileDescriptor.hxx"

#include <boost/asio/posix/stream_descriptor.hpp>

class SignalListener {
  UniqueFileDescriptor fd;
  boost::asio::posix::stream_descriptor asio;

public:
  explicit SignalListener(boost::asio::io_service &io_service)
    :asio(io_service) {}

  boost::asio::io_service &get_io_service() {
    return asio.get_io_service();
  }

private:
  bool InternalCreate(const sigset_t &mask);

  template<typename... Args>
  bool InternalCreate(sigset_t &mask, unsigned signo,
                      Args&&... args) {
    sigaddset(&mask, signo);
    return InternalCreate(mask, args...);
  }

public:
  template<typename... Args>
  bool Create(unsigned signo, Args&&... args) {
    sigset_t mask;
    sigemptyset(&mask);
    return InternalCreate(mask, signo, args...);
  }

  void Destroy();

protected:
  virtual void OnSignal(int signo) = 0;

private:
  void AsyncRead() {
    asio.async_read_some(boost::asio::null_buffers(),
                         std::bind(&SignalListener::OnReadReady, this,
                                   std::placeholders::_1));
  }

  void OnReadReady(const boost::system::error_code &ec);
};

#endif
