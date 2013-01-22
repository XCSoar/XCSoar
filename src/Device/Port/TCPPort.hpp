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

#ifndef XCSOAR_DEVICE_TCP_PORT_HPP
#define XCSOAR_DEVICE_TCP_PORT_HPP

#include "SocketPort.hpp"

#ifndef HAVE_POSIX
#include "Thread/Trigger.hpp"
#endif

/**
 * A TCP listener port class.
 */
class TCPPort : public SocketPort
{
  SocketDescriptor listener;

#ifndef HAVE_POSIX
  SocketThread thread;

  Trigger closed_trigger;
#endif

public:
  /**
   * Creates a new TCPPort object, but does not open it yet.
   *
   * @param handler the callback object for input received on the
   * port
   */
  TCPPort(DataHandler &handler)
    :SocketPort(handler)
#ifndef HAVE_POSIX
    , thread(listener, *this)
#endif
  {}

  /**
   * Closes the serial port (Destructor)
   */
  virtual ~TCPPort();

  /**
   * Opens the serial port
   * @return True on success, False on failure
   */
  bool Open(unsigned port);

  /* overrided virtual methods from SocketPort */
  virtual PortState GetState() const override;

protected:
  /* virtual methods from class FileEventHandler */
  virtual bool OnFileEvent(int fd, unsigned mask) override;
};

#endif
