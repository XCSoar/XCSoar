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
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Asset.hpp"
#include "OS/Sleep.h"

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <termios.h>

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
  :Port(_handler), rx_timeout(0), baud_rate(_baud_rate),
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

  SetBaudrate(baud_rate);

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
  tcflush(fd, TCIOFLUSH);
}

void
TTYPort::Run()
{
  char buffer[1024];

  // XXX use poll()
  while (!WaitForStopped(50)) {
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

size_t
TTYPort::Write(const void *data, size_t length)
{
  if (fd < 0)
    return 0;

  ssize_t nbytes = write(fd, data, length);
  return nbytes < 0 ? 0 : nbytes;
}

bool
TTYPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // Make sure the port is still open
  if (fd < 0)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

bool
TTYPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::IsInside());

  // Make sure the port was opened correctly
  if (fd < 0)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
TTYPort::SetRxTimeout(unsigned Timeout)
{
  rx_timeout = Timeout;
  return true;
}

/**
 * Convert a numeric baud rate to a termios.h constant (B*).  Returns
 * B0 on error.
 */
static speed_t
baud_rate_to_speed_t(unsigned baud_rate)
{
  switch (baud_rate) {
  case 1200:
    return B1200;

  case 2400:
    return B2400;

  case 4800:
    return B4800;

  case 9600:
    return B9600;

  case 19200:
    return B19200;

  case 38400:
    return B38400;

  case 57600:
    return B57600;

  case 115200:
    return B115200;

  default:
    return B0;
  }
}

unsigned long
TTYPort::SetBaudrate(unsigned long BaudRate)
{
  if (fd < 0)
    return 0;

  speed_t speed = baud_rate_to_speed_t(BaudRate);
  if (speed == B0)
    /* not supported */
    return 0;

  unsigned old = baud_rate;

  struct termios attr;
  if (tcgetattr(fd, &attr) < 0)
    return 0;

  attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  attr.c_oflag &= ~OPOST;
  attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | IXON | IXOFF);
  attr.c_cflag |= (CS8 | CLOCAL);
  attr.c_cc[VMIN] = 0;
  attr.c_cc[VTIME] = 1;
  cfsetospeed(&attr, speed);
  cfsetispeed(&attr, speed);
  if (tcsetattr(fd, TCSANOW, &attr) < 0) {
    close(fd);
    return 0;
  }

  baud_rate = BaudRate;

  return old;
}

int
TTYPort::Read(void *Buffer, size_t Size)
{
  if (fd < 0)
    return -1;

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, rx_timeout);
  if (ret != 1)
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
