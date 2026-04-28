// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiBackend.hpp"

class ConnmanWifiBackend final : public WifiBackend {
public:
  void EnsureConnected() override;
  void Scan() override;
  void Connect(const char *ssid, const char *passphrase,
               WifiSecurity security) override;
  void Connect(const WifiConnectRequest &request) override;
  void Disconnect() override;
  WifiBackendStatus GetBackendStatus() override;
  std::size_t GetNetworks(WifiNetworkEntry *dest, std::size_t max) override;
  void ForgetNetwork(const char *profile_id) override;
};