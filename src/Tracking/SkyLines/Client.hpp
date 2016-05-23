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

#ifndef XCSOAR_TRACKING_SKYLINES_CLIENT_HPP
#define XCSOAR_TRACKING_SKYLINES_CLIENT_HPP

#include "Handler.hpp"
#include "Net/AllocatedSocketAddress.hxx"
#include "Net/SocketDescriptor.hpp"
#include "IO/Async/SocketEventHandler.hpp"

#include <stdint.h>

struct NMEAInfo;
class IOThread;

namespace SkyLinesTracking {
  struct TrafficResponsePacket;
  struct UserNameResponsePacket;
  struct WaveResponsePacket;
  struct ThermalResponsePacket;

  class Client
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    : private SocketEventHandler
#endif
  {
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    IOThread *io_thread = nullptr;
    Handler *handler = nullptr;
#endif

    uint64_t key = 0;

    AllocatedSocketAddress address;
    SocketDescriptor socket = SocketDescriptor::Undefined();

  public:
    ~Client() { Close(); }

    constexpr
    static unsigned GetDefaultPort() {
      return 5597;
    }

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    void SetIOThread(IOThread *io_thread);
    void SetHandler(Handler *handler);
#endif

    bool IsDefined() const {
      return socket.IsDefined();
    }

    uint64_t GetKey() const {
      return key;
    }

    void SetKey(uint64_t _key) {
      key = _key;
    }

    bool Open(SocketAddress _address);
    void Close();

    template<typename P>
    bool SendPacket(const P &packet) {
      return socket.Write(&packet, sizeof(packet), address) == sizeof(packet);
    }

    bool SendFix(const NMEAInfo &basic);
    bool SendPing(uint16_t id);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    bool SendTrafficRequest(bool followees, bool club, bool near);
    bool SendUserNameRequest(uint32_t user_id);

  private:
    void OnTrafficReceived(const TrafficResponsePacket &packet, size_t length);
    void OnUserNameReceived(const UserNameResponsePacket &packet,
                            size_t length);
    void OnWaveReceived(const WaveResponsePacket &packet, size_t length);
    void OnThermalReceived(const ThermalResponsePacket &packet, size_t length);
    void OnDatagramReceived(void *data, size_t length);

    /* virtual methods from SocketEventHandler */
    bool OnSocketEvent(SocketDescriptor s, unsigned mask) override;
#endif
  };
}

#endif
