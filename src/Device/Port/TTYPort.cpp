// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TTYPort.hpp"
#include "Device/Error.hpp"
#include "Asset.hpp"
#include "lib/fmt/SystemError.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "system/Error.hxx"
#include "system/TTYDescriptor.hxx"
#include "system/FileUtil.hpp"
#include "event/Call.hxx"
#include "util/StringFormat.hpp"

#include <system_error>
#include <boost/system/system_error.hpp>

#include <sys/stat.h>
#include <termios.h>

#include <cassert>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <windef.h> // for MAX_PATH

static constexpr unsigned
speed_t_to_baud_rate(speed_t speed) noexcept
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

  case B230400:
    return 230400;

  default:
    return 0;
  }
}

/**
 * Convert a numeric baud rate to a termios.h constant (B*).  Returns
 * B0 on error.
 */
static constexpr speed_t
baud_rate_to_speed_t(unsigned baud_rate) noexcept
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

  case 230400:
    return B230400;

  default:
    return B0;
  }
}

static void
SetBaudrate(TTYDescriptor tty, unsigned BaudRate)
{
  assert(tty.IsDefined());

  speed_t speed = baud_rate_to_speed_t(BaudRate);
  if (speed == B0)
    throw std::runtime_error("Unsupported baud rate");

  struct termios attr;
  if (!tty.GetAttr(attr))
    throw MakeErrno("tcgetattr() failed");

  attr.c_iflag &= ~(BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  attr.c_iflag |= (IGNPAR | IGNBRK);
  attr.c_oflag &= ~OPOST;
  attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS);
  attr.c_cflag |= (CS8 | CLOCAL);
  attr.c_cc[VMIN] = 0;
  attr.c_cc[VTIME] = 1;
  cfsetospeed(&attr, speed);
  cfsetispeed(&attr, speed);
  if (!tty.SetAttr(TCSANOW, attr))
    throw MakeErrno("tcsetattr() failed");
}

static UniqueFileDescriptor
OpenTTY(const char *path, unsigned baud_rate)
{
  UniqueFileDescriptor fd;
  if (!fd.OpenNonBlocking(path))
    throw FmtErrno("Failed to open {}", path);

  const TTYDescriptor tty(fd);
  SetBaudrate(tty, baud_rate);
  return fd;
}

TTYPort::TTYPort(EventLoop &event_loop,
                 PortListener *_listener, DataHandler &_handler)
  :BufferedPort(_listener, _handler),
   socket(event_loop, BIND_THIS_METHOD(OnSocketReady))
{
}

TTYPort::~TTYPort() noexcept
{
  BlockingCall(GetEventLoop(), [this](){
    socket.Close();
  });
}

PortState
TTYPort::GetState() const noexcept
{
  return valid.load(std::memory_order_relaxed)
    ? PortState::READY
    : PortState::FAILED;
}

bool
TTYPort::Drain()
{
  const TTYDescriptor tty(socket.GetFileDescriptor());
  return tty.Drain();
}

void
TTYPort::Open(const TCHAR *path, unsigned baud_rate)
{
  auto fd = OpenTTY(path, baud_rate);
  ::SetBaudrate(TTYDescriptor(fd), baud_rate);

  socket.Open(fd.Release());

  BlockingCall(GetEventLoop(), [this](){
    socket.ScheduleRead();
  });

  valid.store(true, std::memory_order_relaxed);

  StateChanged();
}

const char *
TTYPort::OpenPseudo()
{
  const char *path = "/dev/ptmx";

  UniqueFileDescriptor fd;
  if (!fd.OpenNonBlocking(path))
    throw FmtErrno("Failed to open {}", path);

  const TTYDescriptor tty(fd);
  if (!tty.Unlock())
    throw FmtErrno("unlockpt('{}') failed", path);

  socket.Open(fd.Release());

  valid.store(true, std::memory_order_relaxed);

  BlockingCall(GetEventLoop(), [this](){
    socket.ScheduleRead();
  });

  StateChanged();
  return tty.GetSlaveName();
}

void
TTYPort::Flush()
{
  assert(socket.IsDefined());

  if (!valid.load(std::memory_order_relaxed))
    return;

  const TTYDescriptor tty(socket.GetFileDescriptor());
  tty.FlushInput();
  BufferedPort::Flush();
}

inline void
TTYPort::WaitWrite(unsigned timeout_ms)
{
  assert(socket.IsDefined());

  if (!valid.load(std::memory_order_relaxed))
    throw std::runtime_error("Port is closed");

  const TTYDescriptor fd(socket.GetFileDescriptor());
  int ret = fd.WaitWritable(timeout_ms);
  if (ret > 0)
    return;
  else if (ret == 0)
      throw DeviceTimeout{"Port write timeout"};
  else
      throw MakeErrno("Port write failed");
}

std::size_t
TTYPort::Write(std::span<const std::byte> src)
{
  assert(socket.IsDefined());

  if (!valid.load(std::memory_order_relaxed))
    throw std::runtime_error("Port is closed");

  TTYDescriptor fd(socket.GetFileDescriptor());
  auto nbytes = fd.Write(src.data(), src.size());
  if (nbytes < 0) {
    if (errno != EAGAIN)
      /* the output fifo is full; wait until we can write (or until
         the timeout expires) */
      WaitWrite(5000);

    nbytes = fd.Write(src.data(), src.size());
    if (nbytes < 0)
      throw MakeErrno("Port write failed");
  }

  return nbytes;
}

unsigned
TTYPort::GetBaudrate() const noexcept
{
  assert(socket.IsDefined());

  const TTYDescriptor tty(socket.GetFileDescriptor());
  struct termios attr;
  if (!tty.GetAttr(attr))
    return 0;

  return speed_t_to_baud_rate(cfgetispeed(&attr));
}

void
TTYPort::SetBaudrate(unsigned baud_rate)
{
  assert(socket.IsDefined());

  const TTYDescriptor tty(socket.GetFileDescriptor());
  ::SetBaudrate(tty, baud_rate);
}

void
TTYPort::OnSocketReady(unsigned) noexcept
{
  TTYDescriptor tty(socket.GetFileDescriptor());

  std::byte input[4096];
  ssize_t nbytes = tty.Read(input, sizeof(input));
  if (nbytes < 0) {
    int e = errno;
    socket.Cancel();
    valid.store(false, std::memory_order_relaxed);
    StateChanged();
    Error(strerror(e));
    return;
  }

  if (nbytes == 0) {
    socket.Close();
    valid.store(false, std::memory_order_relaxed);
    StateChanged();
    return;
  }

  DataReceived({input, std::size_t(nbytes)});
}
