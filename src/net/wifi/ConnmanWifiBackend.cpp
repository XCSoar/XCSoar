// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConnmanWifiBackend.hpp"

#include "ConnmanClient.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "WifiError.hpp"
#include "lib/dbus/Connection.hxx"

#include <algorithm>
#include <stdexcept>

namespace {

static constexpr const char *CONNMAN_NAME = "net.connman";

static std::runtime_error
TranslateWifiException(const std::exception &e)
{
  return std::runtime_error{FormatWifiErrorForUser(e.what())};
}

[[noreturn]]
static void
ThrowUnsupported(const char *message)
{
  throw std::runtime_error{message};
}

} // namespace

void
ConnmanWifiBackend::EnsureConnected()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    if (!LinuxNetWifi::NameHasOwner(c, CONNMAN_NAME))
      throw std::runtime_error{"ConnMan is not available"};
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
ConnmanWifiBackend::Scan()
{
  EnsureConnected();
}

std::size_t
ConnmanWifiBackend::ScanResults(WifiVisibleNetwork *, unsigned)
{
  return 0;
}

std::size_t
ConnmanWifiBackend::ListNetworks(WifiConfiguredNetworkInfo *, std::size_t)
{
  return 0;
}

void
ConnmanWifiBackend::Connect(const char *ssid, const char *passphrase,
                            WifiSecurity security)
{
  WifiConnectRequest request;
  request.ssid = ssid;
  request.secret = passphrase;
  request.security = security;
  Connect(request);
}

void
ConnmanWifiBackend::Connect(const WifiConnectRequest &request)
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    const auto services = CmClient::ListServices(c);
    const auto found = std::find_if(services.begin(), services.end(),
                                    [&request](const auto &service) {
                                      if (!request.profile_id.empty() &&
                                          service.path == request.profile_id.c_str())
                                        return true;

                                      return !request.ssid.empty() &&
                                        service.ssid_text == request.ssid.c_str();
                                    });
    if (found == services.end())
      throw std::runtime_error{WifiError::GONE};

    if (!request.secret.empty())
      CmClient::SetPassphrase(c, found->path.c_str(), request.secret.c_str());

    CmClient::Connect(c, found->path.c_str());
    CmClient::WaitForServiceConnected(c, found->path.c_str());
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
ConnmanWifiBackend::RemoveNetwork(unsigned)
{
  ThrowUnsupported("Removing saved ConnMan services is not supported");
}

void
ConnmanWifiBackend::SaveConfig()
{
}

bool
ConnmanWifiBackend::Status(WifiStatus &status)
{
  const auto s = GetBackendStatus();
  status.bssid = s.bssid;
  status.ssid = s.ssid;
  return s.state == WifiConnectionState::Connected;
}

void
ConnmanWifiBackend::Disconnect()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    for (const auto &service : CmClient::ListServices(c))
      if (CmClient::IsActiveServiceState(service.state))
        CmClient::Disconnect(c, service.path.c_str());
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

const char *
ConnmanWifiBackend::GetInterfaceName() const
{
  return "";
}

WifiSignalUnit
ConnmanWifiBackend::GetSignalUnit() const
{
  return WifiSignalUnit::Relative;
}

WifiBackendStatus
ConnmanWifiBackend::GetBackendStatus()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    WifiBackendStatus status;
    status.signal_unit = WifiSignalUnit::Relative;

    for (const auto &service : CmClient::ListServices(c)) {
      if (!CmClient::IsActiveServiceState(service.state))
        continue;

      status.state = WifiConnectionState::Connected;
      status.ssid = service.ssid_text.c_str();
      return status;
    }

    status.state = WifiConnectionState::Disconnected;
    return status;
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

std::size_t
ConnmanWifiBackend::GetNetworks(WifiNetworkEntry *dest, std::size_t max)
{
  if (dest == nullptr || max == 0)
    return 0;

  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    const auto services = CmClient::ListServices(c);
    std::size_t count = 0;
    for (const auto &service : services) {
      if (count >= max)
        break;

      auto &entry = dest[count++];
      entry.Clear();
      entry.profile_id = service.path.c_str();
      entry.ssid = service.ssid_text.c_str();
      entry.signal_level = service.strength;
      entry.security = service.needs_key ? WifiSecurity::WPA : WifiSecurity::Open;
      entry.signal_unit = WifiSignalUnit::Relative;
      entry.is_visible = true;

      if (CmClient::IsActiveServiceState(service.state)) {
        entry.kind = WifiNetworkKind::ConnectedNetwork;
        entry.can_disconnect = true;
      } else {
        entry.kind = WifiNetworkKind::VisibleAccessPoint;
        entry.can_connect = true;
      }
    }

    return count;
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
ConnmanWifiBackend::ForgetNetwork(const char *)
{
  ThrowUnsupported("Forgetting saved ConnMan services is not supported");
}