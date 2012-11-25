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

#ifndef XCSOAR_TRACKING_SKYLINES_CLIENT_HPP
#define XCSOAR_TRACKING_SKYLINES_CLIENT_HPP

#include "OS/SocketAddress.hpp"
#include "OS/SocketDescriptor.hpp"
#include "IO/Async/FileEventHandler.hpp"

#include <stdint.h>

struct NMEAInfo;
class IOThread;

#ifdef HAVE_POSIX
#define HAVE_SKYLINES_TRACKING_HANDLER
#endif

namespace SkyLinesTracking {
  class Handler {
  public:
    virtual void OnAck(uint16_t id) {}
  };

  class Client
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    : private FileEventHandler
#endif
  {
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    IOThread *io_thread;
    Handler *handler;
#endif

    uint64_t key;

    SocketAddress address;
    SocketDescriptor socket;

  public:
    Client()
      :
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
      io_thread(nullptr), handler(nullptr),
#endif
      key(0) {}
    ~Client() { Close(); }

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    void SetIOThread(IOThread *io_thread);
    void SetHandler(Handler *handler);
#endif

    bool IsDefined() const {
      return socket.IsDefined();
    }

    void SetKey(uint64_t _key) {
      key = _key;
    }

    bool Open(const char *host);
    void Close();

    bool SendFix(const NMEAInfo &basic);
    bool SendPing(uint16_t id);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  private:
    void OnDatagramReceived(void *data, size_t length);

    /* virtual methods from FileEventHandler */
    virtual bool OnFileEvent(int fd, unsigned mask) gcc_override;
#endif
  };
}

#endif
