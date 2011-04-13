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

#include "Device/TCPPort.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Message.hpp"

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

static void
TCPPort_StatusMessage(unsigned type, const TCHAR *caption,
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

TCPPort::TCPPort(unsigned _port, Handler &_handler)
  :Port(_handler), port(_port), rx_timeout(0),
   listener_fd(-1), connection_fd(-1),
   buffer(NMEA_BUF_SIZE)
{
}

TCPPort::~TCPPort()
{
  Close();
}

bool
TCPPort::Open()
{
  assert(listener_fd < 0);
  assert(connection_fd < 0);

  listener_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listener_fd < 0) {
    TCPPort_StatusMessage(MB_OK | MB_ICONINFORMATION, NULL,
                          _("Failed to create TCP socket"));
    return false;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons((uint16_t)port);

  const int reuse = 1;
#ifdef HAVE_POSIX
  const void *optval = &reuse;
#else
  const char *optval = (const char *)&reuse;
#endif
  setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, optval, sizeof(reuse));

  if (bind(listener_fd, (const struct sockaddr *)&address,
           sizeof(address)) < 0) {
    TCPPort_StatusMessage(MB_OK | MB_ICONINFORMATION, NULL,
                          _("Failed to bind to TCP port %u"), port);
    close(listener_fd);
    listener_fd = -1;
    return false;
  }

  if (listen(listener_fd, 1) < 0) {
    TCPPort_StatusMessage(MB_OK | MB_ICONINFORMATION, NULL,
                          _("Failed to listen on TCP port %u"), port);
    close(listener_fd);
    listener_fd = -1;
    return false;
  }

  if (!StartRxThread()){
    close(listener_fd);
    listener_fd = -1;
    return false;
  }

  return true;
}

void
TCPPort::Flush(void)
{
}

void
TCPPort::run()
{
  char buffer[1024];

  while (!is_stopped()) {
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
        close(listener_fd);
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
          close(connection_fd);
          connection_fd = -1;
          continue;
        }

        for (ssize_t i = 0; i < nbytes; ++i)
          ProcessChar(buffer[i]);
      } else if (ret < 0) {
        close(connection_fd);
        connection_fd = -1;
      }
    }
  }
}

bool
TCPPort::Close()
{
  if (listener_fd < 0) {
    assert(connection_fd < 0);
    return true;
  }

  StopRxThread();

  if (connection_fd >= 0) {
    close(connection_fd);
    connection_fd = -1;
  }

  close(listener_fd);
  listener_fd = -1;
  return true;
}

void
TCPPort::Write(const void *data, unsigned length)
{
  if (connection_fd < 0)
    return;

  send(connection_fd, (const char *)data, length, 0);
}

bool
TCPPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::inside());

  // Make sure the port is still open
  if (listener_fd < 0)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::defined())
    return true;

  stop();

  Thread::join();

  return true;
}

bool
TCPPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::inside());

  // Make sure the port was opened correctly
  if (listener_fd < 0)
    return false;

  // Start the receive thread
  StoppableThread::start();
  return true;
}

bool
TCPPort::SetRxTimeout(int Timeout)
{
  rx_timeout = Timeout;
  return true;
}

unsigned long
TCPPort::SetBaudrate(unsigned long BaudRate)
{
  return 0;
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
  timeout.tv_usec = 250000;

  int ret = select(ufd + 1, &rfds, NULL, NULL, &timeout);
  if (ret != 1)
    return -1;

  return read(ufd, buffer, length);
}

void
TCPPort::ProcessChar(char c)
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
