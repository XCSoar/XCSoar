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

#include "TCPPort.hpp"

#include <unistd.h>
#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <stdint.h>

#ifdef HAVE_POSIX
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef HAVE_POSIX

/**
 * On WIN32, the WinSock function closesocket() must be used to close
 * socket descriptors, which are very different from file descriptors.
 * This wrapper allows using closesocket() on all other operating
 * systems, simply a wrapper for close().
 */
static inline void
closesocket(int fd)
{
  close(fd);
}

#endif

TCPPort::~TCPPort()
{
  if (listener_fd < 0)
    return;

  StopRxThread();

  if (connection_fd >= 0)
    closesocket(connection_fd);

  closesocket(listener_fd);
}

bool
TCPPort::Open(unsigned port)
{
  listener_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listener_fd < 0) {
    return false;
  }

  // Set socket options
  const int reuse = 1;
#ifdef HAVE_POSIX
  const void *optval = &reuse;
#else
  const char *optval = (const char *)&reuse;
#endif
  setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, optval, sizeof(reuse));

  // Bind socket to specified port number
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons((uint16_t)port);

  if (bind(listener_fd, (const struct sockaddr *)&address,
           sizeof(address)) < 0) {
    closesocket(listener_fd);
    listener_fd = -1;
    return false;
  }

  if (listen(listener_fd, 1) < 0) {
    closesocket(listener_fd);
    listener_fd = -1;
    return false;
  }

  connection_fd = -1;
  return true;
}

bool
TCPPort::IsValid() const
{
  return listener_fd >= 0;
}

bool
TCPPort::Drain()
{
  /* writes are synchronous */
  return true;
}

void
TCPPort::Flush()
{
}

void
TCPPort::Run()
{
  char buffer[1024];

  while (!CheckStopped()) {
    assert(listener_fd >= 0);

    if (connection_fd < 0) {
      /* accept new connection */

      fd_set rfds;
      FD_ZERO(&rfds);

      FD_SET(listener_fd, &rfds);

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 250000;

      int ret = select(listener_fd + 1, &rfds, NULL, NULL, &timeout);
      if (ret > 0)
        connection_fd = accept(listener_fd, NULL, NULL);
      else if (ret < 0) {
        closesocket(listener_fd);
        listener_fd = -1;
        break;
      }
    } else {
      /* read from existing client connection */

      fd_set rfds;
      FD_ZERO(&rfds);

      FD_SET(connection_fd, &rfds);

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 250000;

      int ret = select(connection_fd + 1, &rfds, NULL, NULL, &timeout);
      if (ret > 0) {
        ssize_t nbytes = recv(connection_fd, buffer, sizeof(buffer), 0);
        if (nbytes <= 0) {
          closesocket(connection_fd);
          connection_fd = -1;
          continue;
        }

        handler.DataReceived(buffer, nbytes);
      } else if (ret < 0) {
        closesocket(connection_fd);
        connection_fd = -1;
      }
    }
  }
}

size_t
TCPPort::Write(const void *data, size_t length)
{
  if (connection_fd < 0)
    return 0;

  ssize_t nbytes = send(connection_fd, (const char *)data, length, 0);
  return nbytes < 0 ? 0 : nbytes;
}

bool
TCPPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // Make sure the port is still open
  if (listener_fd < 0)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

bool
TCPPort::StartRxThread()
{
  if (Thread::IsDefined())
    /* already running */
    return true;

  // Make sure the port was opened correctly
  if (listener_fd < 0)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

unsigned
TCPPort::GetBaudrate() const
{
  return 0;
}

bool
TCPPort::SetBaudrate(unsigned baud_rate)
{
  return true;
}

int
TCPPort::Read(void *buffer, size_t length)
{
  if (connection_fd < 0)
    return -1;

  fd_set rfds;
  FD_ZERO(&rfds);
  unsigned ufd = connection_fd;
  FD_SET(ufd, &rfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  int ret = select(ufd + 1, &rfds, NULL, NULL, &timeout);
  if (ret != 1)
    return -1;

  return read(connection_fd, buffer, length);
}

Port::WaitResult
TCPPort::WaitRead(unsigned timeout_ms)
{
  if (connection_fd < 0)
    return WaitResult::FAILED;

  fd_set rfds;
  FD_ZERO(&rfds);
  unsigned ufd = connection_fd;
  FD_SET(ufd, &rfds);

  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;

  int ret = select(ufd + 1, &rfds, NULL, NULL, &timeout);
  if (ret > 0)
    return WaitResult::READY;
  else if (ret == 0)
    return WaitResult::TIMEOUT;
  else
    return WaitResult::FAILED;
}
