/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_TCP_CLIENT_PORT_HPP
#define XCSOAR_DEVICE_TCP_CLIENT_PORT_HPP

#include "SocketPort.hpp"

/**
 * A #Port implementation that connects to a TCP port.
 */
class TCPClientPort final
  : public SocketPort
{
#ifdef HAVE_POSIX
  /**
   * The unconnected socket.  This will be moved to SocketPort::socket
   * as soon as the connection has been established.
   */
  SocketDescriptor connecting;
#endif

public:
  TCPClientPort(DataHandler &handler)
    :SocketPort(handler) {}

#ifdef HAVE_POSIX
  virtual ~TCPClientPort();
#endif

  bool Connect(const char *host, unsigned port);

#ifdef HAVE_POSIX
  /* virtual methods from class Port */
  virtual PortState GetState() const override;

protected:
  /* virtual methods from class FileEventHandler */
  virtual bool OnFileEvent(int fd, unsigned mask) override;
#endif
};

#endif
