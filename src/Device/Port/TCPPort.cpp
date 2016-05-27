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

#include "TCPPort.hpp"
#include "Net/Option.hpp"
#include "IO/Async/AsioUtil.hpp"

TCPPort::TCPPort(boost::asio::io_service &io_service,
                 unsigned port,
                 PortListener *_listener, DataHandler &_handler)
  :BufferedPort(_listener, _handler),
   acceptor(io_service,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
   connection(io_service)
{
  acceptor.listen(1);

  AsyncAccept();
}

TCPPort::~TCPPort()
{
  BufferedPort::BeginClose();

  if (connection.is_open())
    CancelWait(connection);

  if (acceptor.is_open())
    CancelWait(acceptor);

  BufferedPort::EndClose();
}

PortState
TCPPort::GetState() const
{
  if (connection.is_open())
    return PortState::READY;
  else if (acceptor.is_open())
    return PortState::LIMBO;
  else
    return PortState::FAILED;
}

size_t
TCPPort::Write(const void *data, size_t length)
{
  if (!connection.is_open())
    return 0;

  boost::system::error_code ec;
  size_t nbytes = connection.send(boost::asio::buffer(data, length), 0, ec);
  if (ec)
    nbytes = 0;

  return nbytes;
}

void
TCPPort::OnAccept(const boost::system::error_code &ec)
{
  if (ec == boost::asio::error::operation_aborted)
    /* this object has already been deleted; bail out quickly without
       touching anything */
    return;

  if (ec) {
    acceptor.close();
    StateChanged();
    Error(ec.message().c_str());
    return;
  }

  StateChanged();

  connection.set_option(SendTimeoutS(1));

  AsyncRead();
}

void
TCPPort::OnRead(const boost::system::error_code &ec, size_t nbytes)
{
  if (ec == boost::asio::error::operation_aborted)
    /* this object has already been deleted; bail out quickly without
       touching anything */
    return;

  if (ec) {
    connection.close();
    AsyncAccept();
    StateChanged();
    Error(ec.message().c_str());
    return;
  }

  DataReceived(input, nbytes);

  AsyncRead();
}
