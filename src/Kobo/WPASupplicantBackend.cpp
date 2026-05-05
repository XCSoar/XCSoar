// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WPASupplicantBackend.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "net/wifi/WifiError.hpp"
#include "system/Error.hxx"
#include "util/HexFormat.hxx"
#include "util/StringAPI.hxx"

#include <array>
#include <exception>
#include <cstring>
#include <stdexcept>

/* workaround because OpenSSL has a typedef called "UI", which clashes
   with our "UI" namespace */
#define UI OPENSSL_UI
#include <openssl/evp.h> // for PKCS5_PBKDF2_HMAC_SHA1()
#undef UI

namespace {

static void
ValidateConnectParameters(const char *ssid, const char *psk,
                         WifiSecurity security)
{
  if (ssid == nullptr || *ssid == '\0')
    throw std::runtime_error{"Missing WiFi network name"};

  if ((security == WifiSecurity::WPA || security == WifiSecurity::WEP) &&
      (psk == nullptr || *psk == '\0'))
    throw std::runtime_error{"Missing WiFi passphrase"};
}

static void
ConfigureNetwork(WPASupplicant &wpa, unsigned id, const char *ssid,
                 const char *psk, WifiSecurity security)
{
  ValidateConnectParameters(ssid, psk, security);

  char *endPsk_ptr;

  wpa.SetNetworkSSID(id, ssid);

  if (security == WifiSecurity::WPA) {
    std::array<std::byte, 32> pmk;
    if (PKCS5_PBKDF2_HMAC_SHA1(psk, (int)std::strlen(psk),
                               (const unsigned char *)ssid,
                               (int)std::strlen(ssid),
                               4096,
                               (int)pmk.size(),
                               (unsigned char *)pmk.data()) != 1)
      throw std::runtime_error{"Failed to derive WPA PSK"};

    std::array<char, sizeof(pmk) * 2 + 1> hex;
    *HexFormat(hex.data(), pmk) = 0;

    wpa.SetNetworkPSK(id, hex.data());
  } else if (security == WifiSecurity::WEP) {
    wpa.SetNetworkID(id, "key_mgmt", "NONE");

    (void)strtoll(psk, &endPsk_ptr, 16);
    const std::size_t psk_length = std::strlen(psk);
    const bool has_hex_prefix = psk_length >= 2 &&
      psk[0] == '0' && psk[1] == 'x';

    if ((*endPsk_ptr == '\0') && !has_hex_prefix)
      wpa.SetNetworkID(id, "wep_key0", psk);
    else
      wpa.SetNetworkString(id, "wep_key0", psk);

    wpa.SetNetworkID(id, "auth_alg", "OPEN\tSHARED");
  } else if (security == WifiSecurity::Open) {
    wpa.SetNetworkID(id, "key_mgmt", "NONE");
  } else {
    throw std::runtime_error{"Unsupported Wifi security type"};
  }
}

} // namespace

WPASupplicantBackend::WPASupplicantBackend(const char *interface_name)
{
  if (interface_name == nullptr || *interface_name == '\0')
    throw std::invalid_argument{"Invalid interface_name"};

  interface_name_ = interface_name;
}

void
WPASupplicantBackend::EnsureConnected()
{
  try {
    wpa_.EnsureConnected(FmtBuffer<64>("/var/run/wpa_supplicant/{}", interface_name_).c_str());
  } catch (const std::system_error &e) {
    if (IsErrno(e, ENOENT) || IsErrno(e, ENODEV) || IsErrno(e, EIO))
      std::throw_with_nested(WifiError::Exception{WifiError::Code::NoInterface});

    throw;
  }
}

void
WPASupplicantBackend::Scan()
{
  EnsureConnected();
  wpa_.Scan();
}

std::size_t
WPASupplicantBackend::ScanResults(WifiVisibleNetwork *dest, unsigned max)
{
  return wpa_.ScanResults(dest, max);
}

std::size_t
WPASupplicantBackend::ListNetworks(WifiConfiguredNetworkInfo *dest, std::size_t max)
{
  return wpa_.ListNetworks(dest, max);
}

void
WPASupplicantBackend::Connect(const char *ssid, const char *psk, WifiSecurity security)
{
  ValidateConnectParameters(ssid, psk, security);

  const unsigned id = wpa_.AddNetwork();
  try {
    ConfigureNetwork(wpa_, id, ssid, psk, security);
    wpa_.EnableNetwork(id);
    wpa_.SaveConfig();
  } catch (...) {
    try {
      wpa_.RemoveNetwork(id);
    } catch (...) {
    }

    throw;
  }
}

void
WPASupplicantBackend::RemoveNetwork(unsigned id)
{
  wpa_.RemoveNetwork(id);
}

void
WPASupplicantBackend::SaveConfig()
{
  wpa_.SaveConfig();
}

void
WPASupplicantBackend::Disconnect()
{
  EnsureConnected();

  unsigned active_network_id;
  if (wpa_.GetCurrentNetworkId(active_network_id))
    wpa_.DisableNetwork(active_network_id);
}

WifiBackendStatus
WPASupplicantBackend::GetBackendStatus()
{
  EnsureConnected();

  WifiBackendStatus status;
  status.interface_name = interface_name_.c_str();
  status.signal_unit = StringIsEqual(interface_name_.c_str(), "eth0")
    ? WifiSignalUnit::Relative
    : WifiSignalUnit::Dbm;

  (void)wpa_.Status(status);
  status.state = !status.bssid.empty()
    ? WifiConnectionState::Connected
    : WifiConnectionState::Disconnected;
  return status;
}

void
WPASupplicantBackend::Connect(const WifiConnectRequest &request)
{
  EnsureConnected();

  if (!request.profile_id.empty() && request.ssid.empty()) {
    wpa_.SelectNetwork(WifiBackend::ParseProfileId(request.profile_id));
    return;
  }

  Connect(request.ssid.empty() ? nullptr : request.ssid.c_str(),
          request.secret.empty() ? nullptr : request.secret.c_str(),
          request.security);
}

void
WPASupplicantBackend::ForgetNetwork(const char *profile_id)
{
  EnsureConnected();
  wpa_.RemoveNetwork(WifiBackend::ParseProfileId(profile_id));
  wpa_.SaveConfig();
}
