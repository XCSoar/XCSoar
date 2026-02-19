// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

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
#if defined(__linux__) && !defined(ANDROID)
#include <sys/ioctl.h>
/* For TIOCSSERIAL method (works on USB-to-serial drivers like FTDI).
   Android uses USB-serial via its Java USB API, not TIOCSSERIAL. */
#ifndef TIOCGSERIAL
#define TIOCGSERIAL 0x541E
#endif
#ifndef TIOCSSERIAL
#define TIOCSSERIAL 0x541F
#endif
#ifndef ASYNC_SPD_CUST
#define ASYNC_SPD_CUST 0x0030
#endif
#ifndef ASYNC_SPD_MASK
#define ASYNC_SPD_MASK 0x1030
#endif
#ifndef __LINUX_SERIAL_H
struct serial_struct {
  int type;
  int line;
  unsigned int port;
  int irq;
  int flags;
  int xmit_fifo_size;
  int custom_divisor;
  int baud_base;
  unsigned short close_delay;
  char io_type;
  char reserved_char[1];
  int hub6;
  unsigned short closing_wait;
  unsigned short closing_wait2;
  unsigned char *iomem_base;
  unsigned short iomem_reg_shift;
  unsigned int port_high;
  unsigned long iomap_base;
};
#endif
#endif

#include <cassert>
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

#ifdef B460800
  case B460800:
    return 460800;
#endif

#ifdef B500000
  case B500000:
    return 500000;
#endif

#ifdef B576000
  case B576000:
    return 576000;
#endif

#ifdef B921600
  case B921600:
    return 921600;
#endif

#ifdef B1000000
  case B1000000:
    return 1000000;
#endif

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

#ifdef B460800
  case 460800:
    return B460800;
#endif

#ifdef B500000
  case 500000:
    return B500000;
#endif

#ifdef B576000
  case 576000:
    return B576000;
#endif

#ifdef B921600
  case 921600:
    return B921600;
#endif

#ifdef B1000000
  case 1000000:
    return B1000000;
#endif

  default:
    return B0;
  }
}

static void
SetTermiosRaw(TTYDescriptor tty, speed_t speed)
{
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

static void
SetBaudrate(TTYDescriptor tty, unsigned baud_rate)
{
  assert(tty.IsDefined());

  speed_t speed = baud_rate_to_speed_t(baud_rate);

  if (speed == B0 && baud_rate > 230400) {
#if defined(__linux__) && !defined(ANDROID)
    struct serial_struct serinfo;
    if (ioctl(tty.Get(), TIOCGSERIAL, &serinfo) < 0)
      throw MakeErrno("TIOCGSERIAL failed");

    SetTermiosRaw(tty, B38400);

    if (serinfo.baud_base <= 0)
      throw std::runtime_error("Serial port does not report baud_base");

    serinfo.flags = (serinfo.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
    serinfo.custom_divisor = (serinfo.baud_base + (baud_rate / 2)) / baud_rate;
    if (serinfo.custom_divisor < 1)
      serinfo.custom_divisor = 1;
    if (ioctl(tty.Get(), TIOCSSERIAL, &serinfo) < 0)
      throw MakeErrno("TIOCSSERIAL failed");
    return;
#else
    throw std::runtime_error("Custom baud rates above 230400 not supported on this platform");
#endif
  }

  if (speed == B0)
    throw std::runtime_error("Unsupported baud rate");

#if defined(__linux__) && !defined(ANDROID)
  /* Clear ASYNC_SPD_CUST flag when switching to a standard baud rate,
     so GetBaudrate() won't return stale custom values */
  struct serial_struct serinfo;
  if (ioctl(tty.Get(), TIOCGSERIAL, &serinfo) == 0 &&
      (serinfo.flags & ASYNC_SPD_MASK) == ASYNC_SPD_CUST) {
    serinfo.flags &= ~ASYNC_SPD_MASK;
    ioctl(tty.Get(), TIOCSSERIAL, &serinfo);
  }
#endif

  SetTermiosRaw(tty, speed);
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
TTYPort::Open(const char *path, unsigned baud_rate)
{
  auto fd = OpenTTY(path, baud_rate);

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

#if defined(__linux__) && !defined(ANDROID)
  /* Only trust ASYNC_SPD_CUST when termios speed is B38400 (the
     TIOCSSERIAL marker); otherwise the port has been switched back
     to a standard baud rate and the custom divisor is stale */
  if (cfgetispeed(&attr) == B38400) {
    struct serial_struct serinfo;
    if (ioctl(tty.Get(), TIOCGSERIAL, &serinfo) == 0 &&
        (serinfo.flags & ASYNC_SPD_MASK) == ASYNC_SPD_CUST &&
        serinfo.baud_base > 0 && serinfo.custom_divisor > 0) {
      unsigned custom_baud = serinfo.baud_base / serinfo.custom_divisor;
      if (custom_baud > 230400)
        return custom_baud;
    }
  }
#endif

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
