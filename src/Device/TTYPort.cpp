/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Device/TTYPort.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Message.hpp"
#include "Asset.hpp"
#include "OS/Sleep.h"

#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>
#include <tchar.h>
#include <stdio.h>

static void
TTYPort_StatusMessage(unsigned type, const TCHAR *caption,
                      const TCHAR *fmt, ...)
{
  TCHAR tmp[127];
  va_list ap;

  va_start(ap, fmt);
  _vsntprintf(tmp, 127, fmt, ap);
  va_end(ap);

  if (caption)
    MessageBoxX(tmp, caption, type);
  else
    Message::AddMessage(tmp);
}

TTYPort::TTYPort(const TCHAR *path, unsigned _baud_rate, Handler &_handler)
  :Port(_handler), baud_rate(_baud_rate),
   fd(-1),
   buffer(NMEA_BUF_SIZE)
{
  assert(path != NULL);

  _tcscpy(sPortName, path);
}

TTYPort::~TTYPort()
{
  Close();
}

bool
TTYPort::Open()
{
  fd = open(sPortName, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    TTYPort_StatusMessage(MB_OK | MB_ICONINFORMATION, NULL,
                          _("Unable to open port %s"), sPortName);
    return false;
  }

  if (!StartRxThread()){
    close(fd);
    fd = -1;

    return false;
  }

  return true;
}

void
TTYPort::Flush(void)
{
  // XXX
}

void
TTYPort::run()
{
  char buffer[1024];

  // XXX use poll()
  while (!wait_stopped(50)) {
    ssize_t nbytes = read(fd, buffer, sizeof(buffer));
    for (ssize_t i = 0; i < nbytes; ++i)
      ProcessChar(buffer[i]);
  }

  Flush();
}

bool
TTYPort::Close()
{
  if (fd < 0)
    return true;

  StopRxThread();

  close(fd);
  fd = -1;
  return true;
}

void
TTYPort::Write(const void *data, unsigned length)
{
  if (fd < 0)
    return;

  write(fd, data, length);
}

bool
TTYPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::inside());

  // Make sure the port is still open
  if (fd < 0)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::defined())
    return true;

  stop();

  Thread::join();

  return true;
}

bool
TTYPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::inside());

  // Make sure the port was opened correctly
  if (fd < 0)
    return false;

  // Start the receive thread
  StoppableThread::start();
  return true;
}

bool
TTYPort::SetRxTimeout(int Timeout)
{
  return true; // XXX
}

unsigned long
TTYPort::SetBaudrate(unsigned long BaudRate)
{
  return BaudRate; // XXX
}

int
TTYPort::Read(void *Buffer, size_t Size)
{
  if (fd < 0)
    return -1;

  return read(fd, Buffer, Size);
}

void
TTYPort::ProcessChar(char c)
{
  FifoBuffer<char>::Range range = buffer.write();
  if (range.second == 0) {
    // overflow, so reset buffer
    buffer.clear();
    return;
  }

  if (c == '\n') {
    range.first[0] = _T('\0');
    buffer.append(1);

    range = buffer.read();
    handler.LineReceived(range.first);
    buffer.clear();
  } else if (c != '\r') {
    range.first[0] = c;
    buffer.append(1);
  }
}
