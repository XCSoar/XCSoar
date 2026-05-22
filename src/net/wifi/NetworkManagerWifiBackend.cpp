// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkManagerWifiBackend.hpp"

#include "LinuxNetWifiDbus.hpp"
#include "NetworkManagerClient.hpp"
#include "WifiError.hpp"
#include "lib/dbus/Connection.hxx"

#include <algorithm>
#include <exception>
#include <stdexcept>

namespace {

static constexpr const char *NM_NAME = "org.freedesktop.NetworkManager";
static constexpr const char *NM_DEVICE_IFACE =
  "org.freedesktop.NetworkManager.Device";
static constexpr const char *NM_ACCESS_POINT_IFACE =
  "org.freedesktop.NetworkManager.AccessPoint";

static ODBus::Connection
GetSystemConnection()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw WifiError::Exception{WifiError::Code::NoDbusConnection};

    return c;
  } catch (const WifiError::Exception &) {
    throw;
  } catch (...) {
    std::throw_with_nested(WifiError::Exception{WifiError::Code::NoDbusConnection});
  }
}

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

  return (!ap.saved_path.empty() && ap.saved_path == profile_id) ||
    MakeProfileId(ap) == profile_id;
}

[[gnu::pure]]
static bool
IsSavedConnectionPath(const char *profile_id) noexcept
{
  return profile_id != nullptr && *profile_id == '/';
}

[[gnu::pure]]
static bool
MatchesRequest(const NmClient::AccessPoint &ap, const WifiConnectRequest &request)
{
  if (!request.profile_id.empty() && MatchesProfileId(ap, request.profile_id))
    return true;

  return !request.ssid.empty() && ap.ssid_text == request.ssid.c_str();
}

static const NmClient::SavedConnection *
FindSavedConnectionForSsid(const std::vector<NmClient::SavedConnection> &saved_connections,
                           const std::string &ssid) noexcept
{
  if (ssid.empty())
    return nullptr;

  for (const auto &saved : saved_connections) {
    if (!saved.ssid_text.empty() && saved.ssid_text == ssid)
      return &saved;
  }

  return nullptr;
}

static void
PopulateSavedProfilePaths(std::vector<NmClient::AccessPoint> &access_points,
                          const std::vector<NmClient::SavedConnection> &saved_connections) noexcept
{
  for (auto &ap : access_points) {
    if (ap.ssid_text.empty())
      continue;

    if (const auto *saved = FindSavedConnectionForSsid(saved_connections, ap.ssid_text);
        saved != nullptr)
      ap.saved_path = saved->path;
  }
}

static void
AssignNetworkEntryFromAccessPoint(WifiNetworkEntry &entry,
                                  const NmClient::AccessPoint &ap,
                                  const NmClient::SavedConnection *saved,
                                  bool active) noexcept
{
  entry.Clear();
  if (saved != nullptr && !saved->path.empty()) {
    entry.profile_id = saved->path.c_str();
    entry.has_stored_credentials = true;
    entry.can_forget = true;
  } else {
    entry.profile_id = MakeProfileId(ap);
  }

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

static bool
HasVisibleCounterpart(const WifiNetworkEntry *entries, std::size_t count,
                      const NmClient::SavedConnection &saved) noexcept
{
  for (std::size_t i = 0; i < count; ++i)
    if (entries[i].is_visible && entries[i].profile_id == saved.path.c_str())
      return true;

  return false;
}

static void
AssignSavedNetworkEntry(WifiNetworkEntry &entry,
                        const NmClient::SavedConnection &saved) noexcept
{
  entry.Clear();
  entry.profile_id = saved.path.c_str();
  entry.ssid = !saved.ssid_text.empty()
    ? saved.ssid_text.c_str()
    : saved.connection_id.c_str();
  entry.has_stored_credentials = true;
  if (!saved.connection_id.empty() && saved.connection_id != saved.ssid_text)
    entry.bssid = saved.connection_id.c_str();
  entry.kind = WifiNetworkKind::SavedProfile;
  entry.can_connect = true;
  entry.can_forget = true;
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

  if (candidate.has_stored_credentials != existing.has_stored_credentials)
    return candidate.has_stored_credentials;

  if (candidate.signal_level != existing.signal_level)
    return candidate.signal_level > existing.signal_level;

  if (existing.security == WifiSecurity::Open &&
      candidate.security != WifiSecurity::Open)
    return true;

  return false;
}

} // namespace

void
NetworkManagerWifiBackend::RefreshDevice(ODBus::Connection &c)
{
  if (!LinuxNetWifi::NameHasOwner(c, NM_NAME))
    throw WifiError::Exception{WifiError::Code::NetworkManagerUnavailable};

  wifi_device = NmClient::FindWifiDevice(c);
  if (wifi_device.empty())
    throw WifiError::Exception{WifiError::Code::NoInterface};

  std::string iface;
  LinuxNetWifi::DbusGetProperty(c, wifi_device.c_str(), NM_DEVICE_IFACE,
                                "Interface", &iface, nullptr, nullptr);
  interface_name = iface.c_str();
}

void
NetworkManagerWifiBackend::EnsureConnected()
{
  auto c = GetSystemConnection();
  RefreshDevice(c);
}

void
NetworkManagerWifiBackend::Scan()
{
  auto c = GetSystemConnection();
  RefreshDevice(c);
  NmClient::RequestScan(c, wifi_device.c_str());
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
  auto c = GetSystemConnection();
  RefreshDevice(c);

  auto access_points = NmClient::ListAccessPoints(c, wifi_device.c_str());
  const auto saved_connections = NmClient::ListSavedConnections(c);
  PopulateSavedProfilePaths(access_points, saved_connections);

  if (!request.profile_id.empty() && request.ssid.empty() &&
      IsSavedConnectionPath(request.profile_id.c_str())) {
    const auto saved = std::find_if(access_points.begin(), access_points.end(),
                                    [&request](const auto &ap) {
                                      return ap.saved_path == request.profile_id.c_str();
                                    });
    if (saved != access_points.end()) {
      NmClient::Connect(c, wifi_device.c_str(), *saved, nullptr);
    } else {
      NmClient::ConnectSaved(c, wifi_device.c_str(),
                             request.profile_id.c_str(), nullptr);
    }

    return;
  }

  const auto found = std::find_if(access_points.begin(), access_points.end(),
                                  [&request](const auto &ap) {
                                    return MatchesRequest(ap, request);
                                  });
  if (found == access_points.end())
    throw WifiError::Exception{WifiError::Code::Gone};

  const char *secret = request.secret.empty() ? nullptr : request.secret.c_str();
  NmClient::Connect(c, wifi_device.c_str(), *found, secret);
}

void
NetworkManagerWifiBackend::Disconnect()
{
  auto c = GetSystemConnection();
  RefreshDevice(c);
  NmClient::Disconnect(c, wifi_device.c_str());
}

WifiBackendStatus
NetworkManagerWifiBackend::GetBackendStatus()
{
  auto c = GetSystemConnection();
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
    NM_ACCESS_POINT_IFACE, "Ssid", ssid);
  std::string hw_address;
  LinuxNetWifi::DbusGetProperty(c, active_ap.c_str(), NM_ACCESS_POINT_IFACE,
                                "HwAddress", &hw_address, nullptr, nullptr);

  status.state = WifiConnectionState::Connected;
  status.ssid = ssid.c_str();
  status.bssid = hw_address.c_str();
  return status;
}

std::size_t
NetworkManagerWifiBackend::GetNetworks(WifiNetworkEntry *dest, std::size_t max)
{
  if (dest == nullptr || max == 0)
    return 0;

  auto c = GetSystemConnection();
  RefreshDevice(c);

  const std::string active_ap =
    NmClient::GetActiveAccessPointPath(c, wifi_device.c_str());
  auto access_points = NmClient::ListAccessPoints(c, wifi_device.c_str());
  const auto saved_connections = NmClient::ListSavedConnections(c);
  PopulateSavedProfilePaths(access_points, saved_connections);

  std::size_t count = 0;
  for (const auto &ap : access_points) {
    const bool active = !active_ap.empty() &&
      NmClient::IsSameBssidAsActive(c, active_ap.c_str(), ap);
    if (ap.ssid_text.empty() && !active)
      continue;

    WifiNetworkEntry candidate;
    AssignNetworkEntryFromAccessPoint(candidate, ap,
                                  FindSavedConnectionForSsid(saved_connections,
                                                             ap.ssid_text),
                                  active);
    if (active)
      candidate.interface_name = interface_name;

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

  for (const auto &saved : saved_connections) {
    if (count >= max)
      break;

    if (HasVisibleCounterpart(dest, count, saved))
      continue;

    AssignSavedNetworkEntry(dest[count++], saved);
  }

  return count;
}

void
NetworkManagerWifiBackend::ForgetNetwork(const char *profile_id)
{
  auto c = GetSystemConnection();
  if (!LinuxNetWifi::NameHasOwner(c, NM_NAME))
    throw WifiError::Exception{WifiError::Code::NetworkManagerUnavailable};

  NmClient::Remove(c, profile_id);
}