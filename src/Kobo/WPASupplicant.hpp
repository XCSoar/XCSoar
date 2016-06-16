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

#ifndef XCSOAR_KOBO_WIFI_HPP
#define XCSOAR_KOBO_WIFI_HPP

#include "Util/StaticString.hxx"
#include "Net/SocketDescriptor.hpp"

enum WifiSecurity {
  WPA_SECURITY,
  WEP_SECURITY,
  OPEN_SECURITY,
};

struct WifiStatus {
  StaticString<32> bssid;
  StaticString<256> ssid;

  void Clear() {
    bssid.clear();
    ssid.clear();
  }
};

struct WifiVisibleNetwork {
  StaticString<32> bssid;
  StaticString<256> ssid;
  unsigned signal_level;
  enum WifiSecurity security;
};

struct WifiConfiguredNetworkInfo {
  int id;
  StaticString<256> ssid;
  StaticString<32> bssid;
};

class WPASupplicant {
  SocketDescriptor fd;

public:
  WPASupplicant():fd(SocketDescriptor::Undefined()) {}

  ~WPASupplicant() {
    Close();
  }

  gcc_pure
  bool IsConnected() const {
    // TODO: what if the socket is broken?
    return fd.IsDefined();
  }

  bool Connect(const char *path);
  void Close();

  bool SendCommand(const char *cmd);
  bool ExpectResponse(const char *expected);

  bool ExpectOK() {
    return ExpectResponse("OK\n");
  }

  bool SaveConfig() {
    return SendCommand("SAVE_CONFIG") && ExpectOK();
  }

  bool Status(WifiStatus &status);

  bool Scan();

  /**
   * @return the number of networks or -1 on error
   */
  int ScanResults(WifiVisibleNetwork *dest, unsigned max);

  /**
   * @return the network id or -1 on error
   */
  int AddNetwork();

  bool SetNetworkString(unsigned id, const char *name, const char *value);

  bool SetNetworkID(unsigned id, const char *name, const char *value);

  bool SetNetworkSSID(unsigned id, const char *ssid) {
    return SetNetworkString(id, "ssid", ssid);
  }

  bool SetNetworkPSK(unsigned id, const char *psk) {
    return SetNetworkString(id, "psk", psk);
  }

  bool SelectNetwork(unsigned id);
  bool EnableNetwork(unsigned id);
  bool DisableNetwork(unsigned id);
  bool RemoveNetwork(unsigned id);

  /**
   * @return the number of networks or -1 on error
   */
  int ListNetworks(WifiConfiguredNetworkInfo *dest, unsigned max);

private:
  ssize_t ReadTimeout(void *buffer, size_t length, int timeout_ms=1000);
};

#endif
