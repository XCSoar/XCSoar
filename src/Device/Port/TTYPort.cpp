/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "TTYPort.hpp"
#include "Asset.hpp"
#include "OS/LogError.hpp"
#include "OS/Sleep.h"
#include "IO/Async/GlobalIOThread.hpp"

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <termios.h>

#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

TTYPort::~TTYPort()
{
  BufferedPort::BeginClose();

  if (tty.IsDefined())
    io_thread->LockRemove(tty.Get());

  BufferedPort::EndClose();
}

PortState
TTYPort::GetState() const
{
  return valid.load(std::memory_order_relaxed)
    ? PortState::READY
    : PortState::FAILED;
}

bool
TTYPort::Drain()
{
  return tty.Drain();
}

bool
TTYPort::Open(const TCHAR *path, unsigned _baud_rate)
{
  if (!tty.OpenNonBlocking(path)) {
    LogErrno(_T("Failed to open port '%s'"), path);
    return false;
  }

  baud_rate = _baud_rate;
  if (!SetBaudrate(baud_rate))
    return false;

  valid.store(true, std::memory_order_relaxed);
  io_thread->LockAdd(tty.Get(), Poll::READ, *this);
  return true;
}

const char *
TTYPort::OpenPseudo()
{
  if (!tty.OpenNonBlocking("/dev/ptmx") || !tty.Unlock())
    return NULL;

  valid.store(true, std::memory_order_relaxed);
  io_thread->LockAdd(tty.Get(), Poll::READ, *this);
  return tty.GetSlaveName();
}

void
TTYPort::Flush()
{
  if (!valid.load(std::memory_order_relaxed))
    return;

  tty.FlushInput();
  BufferedPort::Flush();
}

Port::WaitResult
TTYPort::WaitWrite(unsigned timeout_ms)
{
  assert(tty.IsDefined());

  if (!valid.load(std::memory_order_relaxed))
    return WaitResult::FAILED;

  int ret = tty.WaitWritable(timeout_ms);
  if (ret > 0)
    return WaitResult::READY;
  else if (ret == 0)
    return WaitResult::TIMEOUT;
  else
    return WaitResult::FAILED;
}

size_t
TTYPort::Write(const void *data, size_t length)
{
  assert(tty.IsDefined());

  if (!valid.load(std::memory_order_relaxed))
    return 0;

  ssize_t nbytes = tty.Write(data, length);
  if (nbytes < 0 && errno == EAGAIN) {
    /* the output fifo is full; wait until we can write (or until the
       timeout expires) */
    if (WaitWrite(5000) != Port::WaitResult::READY)
      return 0;

    nbytes = tty.Write(data, length);
  }

  return nbytes < 0 ? 0 : nbytes;
}

static unsigned
speed_t_to_baud_rate(speed_t speed)
{
  switch (speed) {
  case B1200:
    return 1200;

  case B2400:
    return 2400;

  case B4800:
    return 4800;

  case B9600:
    return 9600;

  case B19200:
    return 19200;

  case B38400:
    return 38400;

  case B57600:
    return 57600;

  case B115200:
    return 115200;

  default:
    return 0;
  }
}

unsigned
TTYPort::GetBaudrate() const
{
  assert(tty.IsDefined());

  struct termios attr;
  if (!tty.GetAttr(attr))
    return 0;

  return speed_t_to_baud_rate(cfgetispeed(&attr));
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

bool
TTYPort::SetBaudrate(unsigned BaudRate)
{
  assert(tty.IsDefined());

  speed_t speed = baud_rate_to_speed_t(BaudRate);
  if (speed == B0)
    /* not supported */
    return false;

  struct termios attr;
  if (!tty.GetAttr(attr))
    return false;

  attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  attr.c_oflag &= ~OPOST;
  attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS);
  attr.c_cflag |= (CS8 | CLOCAL);
  attr.c_cc[VMIN] = 0;
  attr.c_cc[VTIME] = 1;
  cfsetospeed(&attr, speed);
  cfsetispeed(&attr, speed);
  if (!tty.SetAttr(TCSANOW, attr))
    return false;

  baud_rate = BaudRate;
  return true;
}

bool
TTYPort::OnFileEvent(int fd, unsigned mask)
{
  char buffer[1024];

  ssize_t nbytes = tty.Read(buffer, sizeof(buffer));
  if (nbytes == 0 || (nbytes < 0 && errno != EAGAIN && errno != EINTR)) {
    valid.store(false, std::memory_order_relaxed);
    return false;
  }

  if (nbytes > 0)
    BufferedPort::DataReceived(buffer, nbytes);

  return true;
}
