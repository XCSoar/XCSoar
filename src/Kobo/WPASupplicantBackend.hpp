// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/wifi/WifiBackend.hpp"
#include "WPASupplicant.hpp"
#include <string>

class WPASupplicantBackend final : public WifiBackend {
public:
  explicit WPASupplicantBackend(const char *interface_name = "wlan0");
  ~WPASupplicantBackend() override = default;

  void EnsureConnected() override;
  void Scan() override;
  std::size_t ScanResults(WifiVisibleNetwork *dest, unsigned max) override;
  std::size_t ListNetworks(WifiConfiguredNetworkInfo *dest, std::size_t max) override;
  void Connect(const char *ssid, const char *passphrase, WifiSecurity security) override;
  void RemoveNetwork(unsigned id) override;
  void SaveConfig() override;
  void Disconnect() override;
  WifiBackendStatus GetBackendStatus() override;
  void Connect(const WifiConnectRequest &request) override;
  void ForgetNetwork(const char *profile_id) override;

private:
  WPASupplicant wpa_;
  std::string interface_name_;
};
