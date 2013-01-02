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

#ifndef XCSOAR_DEVICE_TTY_PORT_HPP
#define XCSOAR_DEVICE_TTY_PORT_HPP

#include "Thread/StoppableThread.hpp"
#include "Thread/Flag.hpp"
#include "BufferedPort.hpp"
#include "OS/TTYDescriptor.hpp"
#include "IO/Async/FileEventHandler.hpp"

/**
 * A serial port class for POSIX (/dev/ttyS*, /dev/ttyUSB*).
 */
class TTYPort : public BufferedPort, protected FileEventHandler
{
  unsigned baud_rate;

  TTYDescriptor tty;

  Flag valid;

public:
  /**
   * Creates a new TTYPort object, but does not open it yet.
   *
   * @param _handler the callback object for input received on the
   * port
   */
  TTYPort(DataHandler &_handler):BufferedPort(_handler) {}

  virtual ~TTYPort();

  /**
   * Opens the serial port
   * @return True on success, False on failure
   */
  bool Open(const TCHAR *path, unsigned baud_rate);

  /**
   * Opens this object with a new pseudo-terminal.  This is only used
   * for debugging.
   *
   * @return the path of the slave pseudo-terminal, NULL on error
   */
  const char *OpenPseudo();

  WaitResult WaitWrite(unsigned timeout_ms);

  /* virtual methods from class Port */
  virtual bool IsValid() const;
  virtual bool Drain();
  virtual void Flush();
  virtual bool SetBaudrate(unsigned baud_rate);
  virtual unsigned GetBaudrate() const;
  virtual size_t Write(const void *data, size_t length);

protected:
  /* virtual methods from class FileEventHandler */
  virtual bool OnFileEvent(int fd, unsigned mask);
};

#endif
