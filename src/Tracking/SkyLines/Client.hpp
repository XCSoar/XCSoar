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

#include <boost/asio/ip/udp.hpp>

#include <stdint.h>

struct NMEAInfo;

namespace SkyLinesTracking {
  struct TrafficResponsePacket;
  struct UserNameResponsePacket;
  struct WaveResponsePacket;
  struct ThermalResponsePacket;

  class Client {
    Handler *const handler;

    uint64_t key = 0;

    boost::asio::ip::udp::endpoint endpoint;
    boost::asio::ip::udp::socket socket;

    uint8_t buffer[4096];
    boost::asio::ip::udp::endpoint sender_endpoint;

  public:
    explicit Client(boost::asio::io_service &io_service,
                    Handler *_handler=nullptr)
      :handler(_handler), socket(io_service) {}
    ~Client() { Close(); }

    constexpr
    static unsigned GetDefaultPort() {
      return 5597;
    }

    constexpr
    static const char *GetDefaultPortString() {
      return "5597";
    }

    boost::asio::io_service &get_io_service() {
      return socket.get_io_service();
    }

    /**
     * Is SkyLines tracking enabled in configuration?
     */
    bool IsEnabled() const {
      return key != 0;
    }

    bool IsDefined() const {
      return IsConnected();
    }

    bool IsConnected() const {
      return socket.is_open();
    }

    uint64_t GetKey() const {
      return key;
    }

    void SetKey(uint64_t _key) {
      key = _key;
    }

    bool Open(boost::asio::ip::udp::endpoint _endpoint);
    void Close();

    template<typename P>
    void SendPacket(const P &packet) {
      socket.send_to(boost::asio::buffer(&packet, sizeof(packet)),
                     endpoint, 0);
    }

    void SendFix(const NMEAInfo &basic);
    void SendPing(uint16_t id);

    void SendTrafficRequest(bool followees, bool club, bool near_);
    void SendUserNameRequest(uint32_t user_id);

  private:
    void OnTrafficReceived(const TrafficResponsePacket &packet, size_t length);
    void OnUserNameReceived(const UserNameResponsePacket &packet,
                            size_t length);
    void OnWaveReceived(const WaveResponsePacket &packet, size_t length);
    void OnThermalReceived(const ThermalResponsePacket &packet, size_t length);
    void OnDatagramReceived(void *data, size_t length);

    void OnReceive(const boost::system::error_code &ec, size_t size);
    void AsyncReceive();
  };
}

#endif
