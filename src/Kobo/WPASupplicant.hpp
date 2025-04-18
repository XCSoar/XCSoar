// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "net/SocketDescriptor.hxx"

#include <cstddef>
#include <span>
#include <string_view>

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
  signed signal_level;  // decibels (dBm) or other unknown unit of measurement, >=0
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

  void SendCommand(std::string_view cmd);

  void ExpectResponse(std::string_view expected);

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

  std::size_t ReadTimeout(std::span<std::byte> dest, int timeout_ms=2000);

  std::string_view ReadStringTimeout(std::span<char> buffer, int timeout_ms=2000);

  std::string_view ExpectLineTimeout(std::span<char> buffer, int timeout_ms=2000);
};
