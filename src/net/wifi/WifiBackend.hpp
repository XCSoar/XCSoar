// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiData.hpp"
#include "util/Exception.hxx"

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <memory>
#include <stdexcept>

class WifiBackend {
public:
  virtual ~WifiBackend() = default;

  virtual void EnsureConnected() = 0;
  virtual void Scan() = 0;
  virtual std::size_t ScanResults(WifiVisibleNetwork *, unsigned)
  {
    ThrowException(std::runtime_error{
        "ScanResults is not supported by this backend"});
  }

  virtual std::size_t ListNetworks(WifiConfiguredNetworkInfo *, std::size_t)
  {
    ThrowException(std::runtime_error{
        "ListNetworks is not supported by this backend"});
  }

  /* Connect using SSID + passphrase; backend handles PSK/PMK derivation. */
  virtual void Connect(const char *ssid, const char *passphrase,
                       WifiSecurity security) = 0;

  virtual void RemoveNetwork(unsigned)
  {
    ThrowException(std::runtime_error{
        "RemoveNetwork is not supported by this backend"});
  }

  virtual void SaveConfig() {}

  virtual void Disconnect()
  {
    ThrowException(std::runtime_error{
        "Disconnect is not supported by this backend"});
  }
  virtual WifiBackendStatus GetBackendStatus() = 0;

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
      entry.security = visible[i].security;
      entry.signal_unit = status.signal_unit;
      entry.kind = WifiNetworkKind::VisibleAccessPoint;
      entry.is_visible = true;
      entry.can_connect = ToWifiAuthMode(visible[i].security) != WifiAuthMode::Unsupported;
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
      entry->kind = WifiNetworkKind::SavedProfile;
      entry->has_stored_credentials = true;
      entry->can_connect = true;
      entry->can_forget = true;
    }

    for (std::size_t i = 0; i < count; ++i)
      if (!status.bssid.empty() && dest[i].bssid == status.bssid) {
        dest[i].kind = WifiNetworkKind::ConnectedNetwork;
        dest[i].can_connect = false;
        dest[i].can_disconnect = true;
        dest[i].interface_name = status.interface_name;
      }

    return count;
  }

  /**
   * Compatibility API for future backends: connect using a backend-neutral
   * request. Existing Kobo-style backends may keep implementing the legacy
   * Connect() overload.
   */
  virtual void Connect(const WifiConnectRequest &request)
  {
    Connect(request.ssid.c_str(), request.secret.c_str(), request.security);
  }

  /**
   * Compatibility API for future backends: forget a saved profile identified
   * by an opaque string. Existing Kobo-style backends may keep using integer
   * network identifiers until they are migrated.
   */
  virtual void ForgetNetwork(const char *profile_id)
  {
    RemoveNetwork(ParseProfileId(profile_id));
    SaveConfig();
  }

protected:
  [[gnu::pure]]
  static unsigned ParseProfileId(const char *profile_id)
  {
    if (profile_id == nullptr || *profile_id == 0)
      throw std::runtime_error{"Missing profile identifier"};

    char *end = nullptr;
    const auto id = std::strtoul(profile_id, &end, 10);
    if (end == profile_id || *end != 0)
      throw std::runtime_error{"Unsupported profile identifier"};

    return (unsigned)id;
  }

private:
  [[gnu::pure]]
  static StaticString<32> FormatProfileId(unsigned id)
  {
    StaticString<32> value;
    value.UnsafeFormat("%u", id);
    return value;
  }
};

using UniqueWifiBackend = std::unique_ptr<WifiBackend>;