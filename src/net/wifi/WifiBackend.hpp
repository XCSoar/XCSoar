// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiTypes.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <memory>

class WifiBackend {
public:
  virtual ~WifiBackend() = default;

  virtual void EnsureConnected() = 0;
  virtual void Scan() = 0;
  virtual std::size_t ScanResults(WifiVisibleNetwork *dest, unsigned max) = 0;
  virtual std::size_t ListNetworks(WifiConfiguredNetworkInfo *dest,
                                   std::size_t max) = 0;

  /* Connect using SSID + passphrase; backend handles PSK/PMK derivation. */
  virtual void Connect(const char *ssid, const char *passphrase,
                       WifiSecurity security) = 0;

  virtual void RemoveNetwork(unsigned id) = 0;
  virtual void SaveConfig() = 0;
  virtual bool Status(WifiStatus &status) = 0;

  /* Name of the network interface used by the backend (e.g. "wlan0"). */
  virtual const char *GetInterfaceName() const = 0;

  /* Whether signal levels returned by this backend are dBm values. */
  virtual bool IsSignalLevelInDbm() const = 0;

  /**
   * Compatibility API for future backends: returns a merged list of visible
   * and configured networks. Existing Kobo-style backends may keep using the
   * older ScanResults()/ListNetworks() API until they are migrated.
   */
  virtual std::size_t GetNetworks(WifiNetworkEntry *dest, std::size_t max)
  {
    if (dest == nullptr || max == 0)
      return 0;

    EnsureConnected();

    const auto status = GetBackendStatus();

    auto visible = std::make_unique<WifiVisibleNetwork[]>(max);
    const std::size_t n_visible = ScanResults(visible.get(), max);

    std::size_t count = 0;
    for (std::size_t i = 0; i < n_visible && count < max; ++i) {
      auto &entry = dest[count++];
      entry.Clear();
      entry.bssid = visible[i].bssid;
      entry.ssid = visible[i].ssid;
      entry.signal_level = visible[i].signal_level;
      entry.auth = ToWifiAuthMode(visible[i].security);
      entry.signal_unit = status.signal_unit;
      entry.is_visible = true;
    }

    auto configured = std::make_unique<WifiConfiguredNetworkInfo[]>(max);
    const std::size_t n_configured = ListNetworks(configured.get(), max);

    for (std::size_t i = 0; i < n_configured; ++i) {
      const auto &network = configured[i];
      WifiNetworkEntry *entry = nullptr;

      for (std::size_t j = 0; j < count; ++j) {
        if (dest[j].profile_id == FormatProfileId(network.id)) {
          entry = &dest[j];
          break;
        }

        if (network.bssid == "any") {
          if (dest[j].is_visible && dest[j].ssid == network.ssid) {
            entry = &dest[j];
            break;
          }
        } else if (dest[j].bssid == network.bssid) {
          entry = &dest[j];
          break;
        }
      }

      if (entry == nullptr) {
        if (count >= max)
          break;

        entry = &dest[count++];
        entry->Clear();
        entry->bssid = network.bssid;
        entry->ssid = network.ssid;
        entry->signal_unit = status.signal_unit;
      }

      entry->profile_id = FormatProfileId(network.id);
      entry->is_saved = true;
    }

    for (std::size_t i = 0; i < count; ++i)
      dest[i].is_connected = !status.bssid.empty() && dest[i].bssid == status.bssid;

    return count;
  }

  /**
   * Compatibility API for future backends: returns backend-neutral status and
   * presentation metadata derived from the older Kobo-style API.
   */
  virtual WifiBackendStatus GetBackendStatus()
  {
    EnsureConnected();

    WifiStatus legacy_status;
    legacy_status.Clear();
    const bool ok = Status(legacy_status);

    WifiBackendStatus status;
    status.interface_name = GetInterfaceName();
    status.signal_unit = IsSignalLevelInDbm()
      ? WifiSignalUnit::Dbm
      : WifiSignalUnit::Relative;
    status.bssid = legacy_status.bssid;
    status.ssid = legacy_status.ssid;
    status.state = ok && !legacy_status.bssid.empty()
      ? WifiConnectionState::Connected
      : WifiConnectionState::Disconnected;
    return status;
  }

  /**
   * Compatibility API for future backends: connect using a backend-neutral
   * request. Existing Kobo-style backends may keep implementing the legacy
   * Connect() overload.
   */
  virtual void Connect(const WifiConnectRequest &request)
  {
    Connect(request.ssid.c_str(), request.secret.c_str(),
            request.auth == WifiAuthMode::Open
            ? OPEN_SECURITY
            : WPA_SECURITY);
  }

  /**
   * Compatibility API for future backends: forget a saved profile identified
   * by an opaque string. Existing Kobo-style backends may keep using integer
   * network identifiers until they are migrated.
   */
  virtual void ForgetNetwork(const char *profile_id)
  {
    if (profile_id == nullptr || *profile_id == 0)
      return;

    char *end = nullptr;
    const auto id = std::strtoul(profile_id, &end, 10);
    if (end == profile_id || *end != 0)
      throw std::runtime_error{"Unsupported profile identifier"};

    RemoveNetwork((unsigned)id);
    SaveConfig();
  }

private:
  [[gnu::pure]]
  static StaticString<32> FormatProfileId(int id)
  {
    StaticString<32> value;
    value.UnsafeFormat("%d", id);
    return value;
  }
};

using UniqueWifiBackend = std::unique_ptr<WifiBackend>;