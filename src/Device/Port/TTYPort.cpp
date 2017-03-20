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

#include "TTYPort.hpp"
#include "Asset.hpp"
#include "OS/FileDescriptor.hxx"
#include "OS/Error.hxx"
#include "IO/Async/AsioUtil.hpp"
#include "Util/StringFormat.hpp"

#include <system_error>
#include <boost/system/system_error.hpp>

#include <sys/stat.h>
#include <termios.h>

#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <windef.h> // for MAX_PATH

TTYPort::TTYPort(boost::asio::io_service &io_service,
                 PortListener *_listener, DataHandler &_handler)
  :BufferedPort(_listener, _handler),
   serial_port(io_service)
{
}

TTYPort::~TTYPort()
{
  BufferedPort::BeginClose();

  if (serial_port.is_open())
    CancelWait(serial_port);

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
#ifdef __BIONIC__
  /* bionic doesn't have tcdrain() */
  return true;
#else
  return tcdrain(serial_port.native_handle()) == 0;
#endif
}

gcc_pure
static bool
IsCharDev(const char *path)
{
  struct stat st;
  return stat(path, &st) == 0 && S_ISCHR(st.st_mode);
}

bool
TTYPort::Open(const TCHAR *path, unsigned baud_rate)
{
  if (IsAndroid() && IsCharDev(path)) {
    /* attempt to give the XCSoar process permissions to access the
       USB serial adapter; this is mostly relevant to the Nook */
    TCHAR command[MAX_PATH];
    StringFormat(command, MAX_PATH, "su -c 'chmod 666 %s'", path);
    system(command);
  }

  boost::system::error_code ec;
  serial_port.open(path, ec);
  if (ec) {
    char error_msg[MAX_PATH + 16];
    StringFormat(error_msg, sizeof(error_msg), "Failed to open %s", path);
    throw boost::system::system_error(ec);
  }

  if (!SetBaudrate(baud_rate))
    return false;

  serial_port.set_option(boost::asio::serial_port_base::parity(
                             boost::asio::serial_port_base::parity::none),
                         ec);
  if (ec)
    return false;

  serial_port.set_option(boost::asio::serial_port_base::character_size(
                             boost::asio::serial_port_base::character_size(8)),
                         ec);
  if (ec)
    return false;

  serial_port.set_option(boost::asio::serial_port_base::stop_bits(
                             boost::asio::serial_port_base::stop_bits::one),
                         ec);
  if (ec)
    return false;

  serial_port.set_option(boost::asio::serial_port_base::flow_control(
                             boost::asio::serial_port_base::flow_control::none),
                         ec);
  if (ec)
    return false;

  class
  {
  public:
    boost::system::error_code store(
        termios& attr, boost::system::error_code& ec) const {
      /* The IGNBRK flag is explicitly cleared by boost::asio::serial_port, and
         it offers no built-in option to change this.
         This flag is needed for some setups, to avoid receiving unwanted '\0'
         charachters, which cannot be detected by weak checksum algorithms. */
      attr.c_iflag |= IGNBRK;

      /* boost::asio::serial_port leaves the VMIN and VTIME parameters
         unintialised, which can lead to undesired behaviour. */
      attr.c_cc[VMIN] = 1;
      attr.c_cc[VTIME] = 0;

      ec = boost::system::error_code();
      return ec;
    }
  } custom_options;
  serial_port.set_option(custom_options, ec);
  if (ec)
    return false;

  valid.store(true, std::memory_order_relaxed);

  AsyncRead();

  StateChanged();
  return true;
}

const char *
TTYPort::OpenPseudo()
{
  const char *path = "/dev/ptmx";

  FileDescriptor fd;
  if (!fd.OpenNonBlocking(path))
    throw FormatErrno("Failed to open %s", path);

  serial_port.assign(fd.Get());

  if (unlockpt(serial_port.native_handle()) < 0)
    throw FormatErrno("unlockpt('%s') failed", path);

  valid.store(true, std::memory_order_relaxed);

  AsyncRead();

  StateChanged();
  return ptsname(serial_port.native_handle());
}

void
TTYPort::Flush()
{
  if (!valid.load(std::memory_order_relaxed))
    return;

  tcflush(serial_port.native_handle(), TCIFLUSH);
  BufferedPort::Flush();
}

Port::WaitResult
TTYPort::WaitWrite(unsigned timeout_ms)
{
  assert(serial_port.is_open());

  if (!valid.load(std::memory_order_relaxed))
    return WaitResult::FAILED;

  const FileDescriptor fd(serial_port.native_handle());
  int ret = fd.WaitWritable(timeout_ms);
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
  assert(serial_port.is_open());

  if (!valid.load(std::memory_order_relaxed))
    return 0;

  boost::system::error_code ec;
  auto nbytes = serial_port.write_some(boost::asio::buffer(data, length), ec);
  if (ec == boost::asio::error::try_again) {
    /* the output fifo is full; wait until we can write (or until the
       timeout expires) */
    if (WaitWrite(5000) != Port::WaitResult::READY)
      return 0;

    nbytes = serial_port.write_some(boost::asio::buffer(data, length), ec);
  }

  return nbytes;
}

unsigned
TTYPort::GetBaudrate() const
{
  assert(serial_port.is_open());

  boost::asio::serial_port_base::baud_rate baud_rate;
  boost::system::error_code ec;
  const_cast<boost::asio::serial_port &>(serial_port).get_option(baud_rate, ec);
  return ec ? 0 : baud_rate.value();
}

bool
TTYPort::SetBaudrate(unsigned baud_rate)
{
  assert(serial_port.is_open());

  boost::system::error_code ec;
  serial_port.set_option(boost::asio::serial_port_base::baud_rate(baud_rate),
                         ec);
  return !ec;
}

void
TTYPort::OnReadReady(const boost::system::error_code &ec)
{
  if (ec == boost::asio::error::operation_aborted)
    /* this object has already been deleted; bail out quickly without
       touching anything */
    return;

  if (ec) {
    valid.store(false, std::memory_order_relaxed);
    StateChanged();
    Error(ec.message().c_str());
    return;
  }

  char buffer[1024];

  boost::system::error_code ec2;
  auto nbytes = serial_port.read_some(boost::asio::buffer(buffer,
                                                          sizeof(buffer)),
                                      ec2);
  if (nbytes == 0 || (ec2 && ec2 != boost::asio::error::try_again &&
                      ec2 != boost::asio::error::interrupted)) {
    valid.store(false, std::memory_order_relaxed);
    StateChanged();
    return;
  }

  if (nbytes > 0)
    BufferedPort::DataReceived(buffer, nbytes);

  AsyncRead();
}
