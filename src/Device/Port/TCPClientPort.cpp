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

#include "TCPClientPort.hpp"
#include "IO/Async/AsioUtil.hpp"
#include "Net/Option.hpp"
#include "Util/StaticString.hxx"

TCPClientPort::TCPClientPort(boost::asio::io_service &io_service,
                             PortListener *_listener, DataHandler &_handler)
  :BufferedPort(_listener, _handler),
   resolver(io_service),
   socket(io_service)
{
}

TCPClientPort::~TCPClientPort()
{
  BufferedPort::BeginClose();

  if (socket.is_open())
    CancelWait(socket);
  else
    CancelWait(resolver);

  BufferedPort::EndClose();
}

bool
TCPClientPort::Connect(const char *host, unsigned port)
{
  NarrowString<32> service;
  service.UnsafeFormat("%u", port);

  resolver.async_resolve({host, service.c_str()},
                         std::bind(&TCPClientPort::OnResolved, this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));

  return true;
}

size_t
TCPClientPort::Write(const void *data, size_t length)
{
  if (!socket.is_open())
    return 0;

  boost::system::error_code ec;
  size_t nbytes = socket.send(boost::asio::buffer(data, length), 0, ec);
  if (ec)
    nbytes = 0;

  return nbytes;
}

void
TCPClientPort::OnResolved(const boost::system::error_code &ec,
                          boost::asio::ip::tcp::resolver::iterator i)
{
  if (ec == boost::asio::error::operation_aborted)
    /* this object has already been deleted; bail out quickly without
       touching anything */
    return;

  if (ec) {
    state = PortState::FAILED;
    StateChanged();
    Error(ec.message().c_str());
    return;
  }

  socket.async_connect(*i,
                       std::bind(&TCPClientPort::OnConnect, this,
                                 std::placeholders::_1));
}

void
TCPClientPort::OnConnect(const boost::system::error_code &ec)
{
  if (ec == boost::asio::error::operation_aborted)
    /* this object has already been deleted; bail out quickly without
       touching anything */
    return;

  if (ec) {
    socket.close();
    state = PortState::FAILED;
    StateChanged();
    Error(ec.message().c_str());
    return;
  }

  SendTimeoutS send_timeout(1);
  socket.set_option(send_timeout);

  state = PortState::READY;
  StateChanged();

  socket.async_receive(boost::asio::buffer(input, sizeof(input)),
                       std::bind(&TCPClientPort::OnRead, this,
                                 std::placeholders::_1,
                                 std::placeholders::_2));
}

void
TCPClientPort::OnRead(const boost::system::error_code &ec, size_t nbytes)
{
  if (ec == boost::asio::error::operation_aborted)
    /* this object has already been deleted; bail out quickly without
       touching anything */
    return;

  if (ec) {
    socket.close();
    state = PortState::FAILED;
    StateChanged();
    Error(ec.message().c_str());
    return;
  }

  DataReceived(input, nbytes);

  socket.async_receive(boost::asio::buffer(input, sizeof(input)),
                       std::bind(&TCPClientPort::OnRead, this,
                                 std::placeholders::_1,
                                 std::placeholders::_2));
}
