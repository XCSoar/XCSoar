// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkManagerWifiBackend.hpp"

#include "LinuxNetWifiDbus.hpp"
#include "NetworkManagerClient.hpp"
#include "WifiError.hpp"
#include "lib/dbus/Connection.hxx"

#include <algorithm>
#include <stdexcept>

namespace {

static constexpr const char *kNmName = "org.freedesktop.NetworkManager";
static constexpr const char *kNmDeviceIface =
  "org.freedesktop.NetworkManager.Device";
static constexpr const char *kNmAccessPointIface =
  "org.freedesktop.NetworkManager.AccessPoint";

[[gnu::pure]]
static StaticString<256>
MakeProfileId(const NmClient::AccessPoint &ap)
{
  StaticString<256> id;
  if (!ap.hw_address.empty())
    id.UnsafeFormat("nm-bssid:%s", ap.hw_address.c_str());
  else if (!ap.ap_path.empty())
    id.UnsafeFormat("nm-ap:%s", ap.ap_path.c_str());
  else
    id.UnsafeFormat("nm-ssid:%s", ap.ssid_text.c_str());

  return id;
}

[[gnu::pure]]
static bool
MatchesProfileId(const NmClient::AccessPoint &ap, const char *profile_id) noexcept
{
  if (profile_id == nullptr || *profile_id == 0)
    return false;

  return MakeProfileId(ap) == profile_id;
}

[[gnu::pure]]
static bool
MatchesRequest(const NmClient::AccessPoint &ap, const WifiConnectRequest &request)
{
  if (!request.profile_id.empty() && MatchesProfileId(ap, request.profile_id))
    return true;

  return !request.ssid.empty() && ap.ssid_text == request.ssid.c_str();
}

static void
AssignNetworkEntryFromAccessPoint(WifiNetworkEntry &entry,
                                  const NmClient::AccessPoint &ap,
                                  bool active) noexcept
{
  entry.Clear();
  entry.profile_id = MakeProfileId(ap);
  entry.bssid = ap.hw_address.c_str();
  entry.ssid = ap.ssid_text.c_str();
  entry.signal_level = ap.strength;
  entry.security = ap.needs_key ? WifiSecurity::WPA : WifiSecurity::Open;
  entry.signal_unit = WifiSignalUnit::Relative;
  entry.is_visible = true;

  if (active) {
    entry.kind = WifiNetworkKind::ConnectedNetwork;
    entry.can_disconnect = true;
  } else {
    entry.kind = WifiNetworkKind::VisibleAccessPoint;
    entry.can_connect = true;
  }
}

[[gnu::pure]]
static bool
ShouldReplaceDuplicateSsidEntry(const WifiNetworkEntry &existing,
                                const WifiNetworkEntry &candidate) noexcept
{
  if (candidate.kind == WifiNetworkKind::ConnectedNetwork)
    return existing.kind != WifiNetworkKind::ConnectedNetwork;

  if (existing.kind == WifiNetworkKind::ConnectedNetwork)
    return false;

  if (candidate.signal_level != existing.signal_level)
    return candidate.signal_level > existing.signal_level;

  if (existing.security == WifiSecurity::Open &&
      candidate.security != WifiSecurity::Open)
    return true;

  return false;
}

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
NetworkManagerWifiBackend::RefreshDevice(ODBus::Connection &c)
{
  if (!LinuxNetWifi::NameHasOwner(c, kNmName))
    throw std::runtime_error{"NetworkManager is not available"};

  wifi_device = NmClient::FindWifiDevice(c);
  if (wifi_device.empty())
    throw std::runtime_error{"No WiFi device found"};

  std::string iface;
  LinuxNetWifi::DbusGetProperty(c, wifi_device.c_str(), kNmDeviceIface,
                                "Interface", &iface, nullptr, nullptr);
  interface_name = iface.c_str();
}

void
NetworkManagerWifiBackend::EnsureConnected()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    RefreshDevice(c);
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
NetworkManagerWifiBackend::Scan()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    RefreshDevice(c);
    NmClient::RequestScan(c, wifi_device.c_str());
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
NetworkManagerWifiBackend::Connect(const char *ssid, const char *passphrase,
                                   WifiSecurity security)
{
  WifiConnectRequest request;
  request.ssid = ssid;
  request.secret = passphrase;
  request.security = security;
  Connect(request);
}

void
NetworkManagerWifiBackend::Connect(const WifiConnectRequest &request)
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    RefreshDevice(c);

    const auto access_points = NmClient::ListAccessPoints(c, wifi_device.c_str());
    const auto found = std::find_if(access_points.begin(), access_points.end(),
                                    [&request](const auto &ap) {
                                      return MatchesRequest(ap, request);
                                    });
    if (found == access_points.end())
      throw std::runtime_error{WifiError::GONE};

    const std::string active_ap =
      NmClient::GetActiveAccessPointPath(c, wifi_device.c_str());
    if (!LinuxNetWifi::DbusObjectPathIsEmpty(active_ap) &&
        !NmClient::IsSameBssidAsActive(c, active_ap.c_str(), *found)) {
      NmClient::Disconnect(c, wifi_device.c_str());
      NmClient::WaitUntilWifiDisconnected(c, wifi_device.c_str());
    }

    const char *secret = request.secret.empty() ? nullptr : request.secret.c_str();
    NmClient::ConnectToAp(c, wifi_device.c_str(), *found, secret);
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
NetworkManagerWifiBackend::RemoveNetwork(unsigned)
{
  ThrowUnsupported("Removing saved NetworkManager profiles is not supported");
}

void
NetworkManagerWifiBackend::SaveConfig()
{
}

bool
NetworkManagerWifiBackend::Status(WifiStatus &status)
{
  const auto s = GetBackendStatus();
  status.bssid = s.bssid;
  status.ssid = s.ssid;
  return s.state == WifiConnectionState::Connected;
}

void
NetworkManagerWifiBackend::Disconnect()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    RefreshDevice(c);
    NmClient::Disconnect(c, wifi_device.c_str());
    NmClient::WaitUntilWifiDisconnected(c, wifi_device.c_str());
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

const char *
NetworkManagerWifiBackend::GetInterfaceName() const
{
  return interface_name.c_str();
}

WifiSignalUnit
NetworkManagerWifiBackend::GetSignalUnit() const
{
  return WifiSignalUnit::Relative;
}

WifiBackendStatus
NetworkManagerWifiBackend::GetBackendStatus()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    RefreshDevice(c);

    WifiBackendStatus status;
    status.interface_name = interface_name;
    status.signal_unit = WifiSignalUnit::Relative;

    const std::string active_ap =
      NmClient::GetActiveAccessPointPath(c, wifi_device.c_str());
    if (active_ap.empty()) {
      status.state = WifiConnectionState::Disconnected;
      return status;
    }

    std::string ssid;
    LinuxNetWifi::DbusGetByteStringProperty(c, active_ap.c_str(),
      kNmAccessPointIface, "Ssid", ssid);
    std::string hw_address;
    LinuxNetWifi::DbusGetProperty(c, active_ap.c_str(), kNmAccessPointIface,
                                  "HwAddress", &hw_address, nullptr, nullptr);

    status.state = WifiConnectionState::Connected;
    status.ssid = ssid.c_str();
    status.bssid = hw_address.c_str();
    return status;
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

std::size_t
NetworkManagerWifiBackend::GetNetworks(WifiNetworkEntry *dest, std::size_t max)
{
  if (dest == nullptr || max == 0)
    return 0;

  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw std::runtime_error{"No D-Bus connection"};

    RefreshDevice(c);

    const std::string active_ap =
      NmClient::GetActiveAccessPointPath(c, wifi_device.c_str());
    const auto access_points = NmClient::ListAccessPoints(c, wifi_device.c_str());

    std::size_t count = 0;
    for (const auto &ap : access_points) {
      const bool active = !active_ap.empty() &&
        NmClient::IsSameBssidAsActive(c, active_ap.c_str(), ap);
      if (ap.ssid_text.empty() && !active)
        continue;

      WifiNetworkEntry candidate;
      AssignNetworkEntryFromAccessPoint(candidate, ap, active);

      WifiNetworkEntry *existing = nullptr;
      if (!candidate.ssid.empty())
        for (std::size_t i = 0; i < count; ++i)
          if (dest[i].ssid == candidate.ssid) {
            existing = &dest[i];
            break;
          }

      if (existing != nullptr) {
        if (ShouldReplaceDuplicateSsidEntry(*existing, candidate))
          *existing = candidate;

        continue;
      }

      if (count >= max)
        break;

      dest[count++] = candidate;
    }

    return count;
  } catch (const std::exception &e) {
    throw TranslateWifiException(e);
  }
}

void
NetworkManagerWifiBackend::ForgetNetwork(const char *)
{
  ThrowUnsupported("Forgetting saved NetworkManager profiles is not supported");
}