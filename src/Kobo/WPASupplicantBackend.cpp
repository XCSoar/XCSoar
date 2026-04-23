// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WPASupplicantBackend.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "util/HexFormat.hxx"
#include "util/StringAPI.hxx"

#include <array>
#include <cstring>
#include <stdexcept>

/* workaround because OpenSSL has a typedef called "UI", which clashes
   with our "UI" namespace */
#define UI OPENSSL_UI
#include <openssl/evp.h> // for PKCS5_PBKDF2_HMAC_SHA1()
#undef UI

WPASupplicantBackend::WPASupplicantBackend(const char *interface_name)
{
  if (interface_name == nullptr || *interface_name == '\0')
    throw std::invalid_argument{"Invalid interface_name"};

  interface_name_ = interface_name;
}

void
WPASupplicantBackend::EnsureConnected()
{
  wpa_.EnsureConnected(FmtBuffer<64>("/var/run/wpa_supplicant/{}", interface_name_).c_str());
}

void
WPASupplicantBackend::Scan()
{
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
  const unsigned id = wpa_.AddNetwork();
  char *endPsk_ptr;

  wpa_.SetNetworkSSID(id, ssid);

  if (security == WPA_SECURITY) {
    std::array<std::byte, 32> pmk;
    PKCS5_PBKDF2_HMAC_SHA1(psk, (int)std::strlen(psk),
                           (const unsigned char *)ssid, (int)std::strlen(ssid),
                           4096,
                           (int)pmk.size(), (unsigned char *)pmk.data());

    std::array<char, sizeof(pmk) * 2 + 1> hex;
    *HexFormat(hex.data(), pmk) = 0;

    wpa_.SetNetworkPSK(id, hex.data());
  } else if (security == WEP_SECURITY) {
    wpa_.SetNetworkID(id, "key_mgmt", "NONE");

    (void) strtoll(psk, &endPsk_ptr, 16);

    if ((*endPsk_ptr == '\0') &&                                   // confirm strtoll processed all of psk
        (std::strlen(psk) >= 2) && (psk[0] != '0') && (psk[1] != 'x'))  // and the first two characters were no "0x"
      wpa_.SetNetworkID(id, "wep_key0", psk);
    else
      wpa_.SetNetworkString(id, "wep_key0", psk);

    wpa_.SetNetworkID(id, "auth_alg", "OPEN\tSHARED");
  } else if (security == OPEN_SECURITY){
    wpa_.SetNetworkID(id, "key_mgmt", "NONE");
  } else
    throw std::runtime_error{"Unsupported Wifi security type"};

  wpa_.EnableNetwork(id);
  wpa_.SaveConfig();
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

bool
WPASupplicantBackend::Status(WifiStatus &status)
{
  return wpa_.Status(status);
}

bool
WPASupplicantBackend::IsSignalLevelInDbm() const
{
  return !StringIsEqual(interface_name_.c_str(), "eth0");
}
