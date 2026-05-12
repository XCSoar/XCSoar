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

[[gnu::pure]]
static bool
CanForgetService(const CmClient::ServiceEntry &service) noexcept
{
  return service.favorite && !service.immutable;
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
  try {
    EnsureConnected();

    auto c = ODBus::Connection::GetSystem();

    CmClient::EnableWifiTechnology(c);
    CmClient::ScanWifiTechnology(c);
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
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

    CmClient::EnableWifiTechnology(c);

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

    const char *ssid = nullptr;
    if (!request.ssid.empty())
      ssid = request.ssid.c_str();
    else if (!found->ssid_text.empty())
      ssid = found->ssid_text.c_str();

    const char *secret = request.secret.empty()
      ? nullptr
      : request.secret.c_str();

    CmClient::Connect(c, found->path.c_str(), ssid, secret);
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
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
      status.interface_name = service.interface_name.c_str();
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
      entry.is_visible = service.has_strength ||
        CmClient::IsActiveServiceState(service.state);
      entry.has_stored_credentials = service.favorite;

      if (CmClient::IsActiveServiceState(service.state)) {
        entry.kind = WifiNetworkKind::ConnectedNetwork;
        entry.interface_name = service.interface_name.c_str();
        entry.can_disconnect = true;
        entry.can_forget = CanForgetService(service);
      } else if (service.favorite) {
        entry.kind = WifiNetworkKind::SavedProfile;
        entry.can_connect = true;
        entry.can_forget = CanForgetService(service);
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
ConnmanWifiBackend::ForgetNetwork(const char *profile_id)
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    CmClient::Remove(c, profile_id);
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}