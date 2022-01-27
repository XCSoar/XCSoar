/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "util/StaticString.hxx"
#include "net/SocketDescriptor.hxx"

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

/**
 * All methods that are not `noexcept` throw on error.
 */
class WPASupplicant {
  SocketDescriptor fd;

public:
  WPASupplicant() noexcept:fd(SocketDescriptor::Undefined()) {}

  ~WPASupplicant() noexcept {
    Close();
  }

  [[gnu::pure]]
  bool IsConnected() const noexcept {
    // TODO: what if the socket is broken?
    return fd.IsDefined();
  }

  /**
   * Throws on error.
   */
  void Connect(const char *path);

  void EnsureConnected(const char *path) {
    if (!IsConnected())
      Connect(path);
  }

  void Close() noexcept;

  void SendCommand(const char *cmd);

  void ExpectResponse(const char *expected);

  void ExpectOK() {
    ExpectResponse("OK\n");
  }

  void SaveConfig() {
    SendCommand("SAVE_CONFIG");
    ExpectOK();
  }

  bool Status(WifiStatus &status);

  void Scan() {
    SendCommand("SCAN");
    ExpectOK();
  }

  /**
   * @return the number of networks
   */
  std::size_t ScanResults(WifiVisibleNetwork *dest, unsigned max);

  /**
   * @return the network id
   */
  unsigned AddNetwork();

  void SetNetworkString(unsigned id, const char *name, const char *value);

  void SetNetworkID(unsigned id, const char *name, const char *value);

  void SetNetworkSSID(unsigned id, const char *ssid) {
    SetNetworkString(id, "ssid", ssid);
  }

  void SetNetworkPSK(unsigned id, const char *psk) {
    SetNetworkID(id, "psk", psk);
  }

  void SelectNetwork(unsigned id);
  void EnableNetwork(unsigned id);
  void DisableNetwork(unsigned id);
  void RemoveNetwork(unsigned id);

  /**
   * Throws on error.
   *
   * @return the number of networks
   */
  std::size_t ListNetworks(WifiConfiguredNetworkInfo *dest, std::size_t max);

private:
  void ReadDiscard() noexcept;

  std::size_t ReadTimeout(void *buffer, size_t length, int timeout_ms=2000);
};

#endif
