// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkManagerClient.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "WifiTypes.hpp"
#include "WifiError.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "lib/dbus/AppendIter.hxx"
#include "lib/dbus/CallMethodSync.hxx"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Message.hxx"
#include "lib/dbus/ReadIter.hxx"
#include "util/StringAPI.hxx"

#include <chrono>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <dbus/dbus.h>

static constexpr const char *kNm = "org.freedesktop.NetworkManager";
static constexpr const char *kNmPath = "/org/freedesktop/NetworkManager";
static constexpr const char *kNmSettingsPath = "/org/freedesktop/NetworkManager/Settings";
static constexpr const char *kNmIfaceSettings = "org.freedesktop.NetworkManager.Settings";
static constexpr const char *kNmIfaceSettingsConnection =
  "org.freedesktop.NetworkManager.Settings.Connection";
static constexpr const char *kNmIfaceDevice = "org.freedesktop.NetworkManager.Device";
static constexpr const char *kNmIfaceDeviceWireless =
  "org.freedesktop.NetworkManager.Device.Wireless";
static constexpr unsigned kNmDeviceTypeWifi = 2;
static constexpr std::uint32_t kApFlagPrivacy = 0x1U;
static constexpr std::uint32_t kNmDeviceStateDisconnected = 30U;

static std::string
NewConnectionUuid()
{
  static constexpr char h[] = "0123456789abcdef";
  unsigned char a[16]{};
  std::random_device rd;
  std::uniform_int_distribution<int> b(0, 255);
  for (auto &x : a) {
    x = static_cast<unsigned char>(b(rd));
  }
  a[6U] = static_cast<unsigned char>((a[6U] & 0x0FU) | 0x40U);
  a[8U] = static_cast<unsigned char>((a[8U] & 0x3FU) | 0x80U);
  std::string s;
  s.reserve(36U);
  for (int i = 0; i < 16; ++i) {
    s.push_back(h[(a[static_cast<unsigned>(i)] >> 4U) & 0x0FU]);
    s.push_back(h[a[static_cast<unsigned>(i)] & 0x0FU]);
    if (i == 3 || i == 5 || i == 7 || i == 9) {
      s.push_back('-');
    }
  }
  return s;
}

static void
AppendStringVariantMapString(DBusMessageIter *map, const char *key, const char *str)
{
  DBusMessageIter e, v;
  const char *k = key;
  dbus_message_iter_open_container(map, DBUS_TYPE_DICT_ENTRY, nullptr, &e);
  dbus_message_iter_append_basic(&e, DBUS_TYPE_STRING, &k);
  dbus_message_iter_open_container(&e, DBUS_TYPE_VARIANT, "s", &v);
  const char *s = str;
  dbus_message_iter_append_basic(&v, DBUS_TYPE_STRING, &s);
  dbus_message_iter_close_container(&e, &v);
  dbus_message_iter_close_container(map, &e);
}

static void
AppendStringVariantMapBytes(DBusMessageIter *map, const char *key, const void *data,
                            int len)
{
  DBusMessageIter e, v, a;
  const char *k = key;
  dbus_message_iter_open_container(map, DBUS_TYPE_DICT_ENTRY, nullptr, &e);
  dbus_message_iter_append_basic(&e, DBUS_TYPE_STRING, &k);
  dbus_message_iter_open_container(&e, DBUS_TYPE_VARIANT, "ay", &v);
  const void *b = data;
  dbus_message_iter_open_container(&v, DBUS_TYPE_ARRAY, "y", &a);
  if (len > 0)
    dbus_message_iter_append_fixed_array(&a, DBUS_TYPE_BYTE, &b, len);
  dbus_message_iter_close_container(&v, &a);
  dbus_message_iter_close_container(&e, &v);
  dbus_message_iter_close_container(map, &e);
}

template<typename F>
static void
AppendSettingsSection(DBusMessageIter *arr, const char *section_name, F &&fn)
{
  DBusMessageIter entry, value;
  const char *sn = section_name;
  dbus_message_iter_open_container(arr, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &sn);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_ARRAY, "{sv}", &value);
  fn(&value);
  dbus_message_iter_close_container(&entry, &value);
  dbus_message_iter_close_container(arr, &entry);
}

static void
ListObjectPaths(ODBus::Connection &c, const char *dest, const char *obj_path,
                const char *iface, const char *method, std::vector<std::string> &out)
{
  out.clear();
  using namespace ODBus;
  auto m = Message::NewMethodCall(dest, obj_path, iface, method);
  Message reply = CallMethodSync(c, m);
  ReadMessageIter t{*reply.Get()};
  if (t.GetArgType() != DBUS_TYPE_ARRAY) {
    return;
  }
  for (ReadMessageIter e = t.Recurse();
       e.GetArgType() == DBUS_TYPE_OBJECT_PATH; e.Next()) {
    const char *p = e.GetString();
    if (p != nullptr)
      out.emplace_back(p);
  }
}

static void
ParseWifiSettings(ODBus::ReadMessageIter &t, std::string &ssid_out,
                  bool *wifi, std::string *connection_id_out = nullptr)
{
  ssid_out.clear();
  *wifi = false;
  if (connection_id_out != nullptr) {
    connection_id_out->clear();
  }
  if (t.GetArgType() != DBUS_TYPE_ARRAY) {
    return;
  }
  for (ODBus::ReadMessageIter e = t.Recurse();
       e.GetArgType() == DBUS_TYPE_DICT_ENTRY; e.Next()) {
    ODBus::ReadMessageIter de = e.Recurse();
    if (de.GetArgType() != DBUS_TYPE_STRING) {
      continue;
    }
    const char *section = de.GetString();
    de.Next();
    if (de.GetArgType() != DBUS_TYPE_ARRAY) {
      continue;
    }
    for (ODBus::ReadMessageIter pe = de.Recurse();
         pe.GetArgType() == DBUS_TYPE_DICT_ENTRY; pe.Next()) {
      ODBus::ReadMessageIter pde = pe.Recurse();
      if (pde.GetArgType() != DBUS_TYPE_STRING) {
        continue;
      }
      const char *k = pde.GetString();
      pde.Next();
      if (pde.GetArgType() != DBUS_TYPE_VARIANT) {
        continue;
      }
      ODBus::ReadMessageIter v = pde.Recurse();
      if (StringIsEqual(section, "connection") && StringIsEqual(k, "type") &&
          v.GetArgType() == DBUS_TYPE_STRING) {
        const char *tv = v.GetString();
        if (tv != nullptr && StringIsEqual(tv, "802-11-wireless")) {
          *wifi = true;
        }
      }
      if (connection_id_out != nullptr && StringIsEqual(section, "connection") &&
          StringIsEqual(k, "id") && v.GetArgType() == DBUS_TYPE_STRING) {
        const char *idv = v.GetString();
        if (idv != nullptr) {
          *connection_id_out = idv;
        }
      }
      if (StringIsEqual(section, "802-11-wireless") && StringIsEqual(k, "ssid")) {
        if (v.GetArgType() == DBUS_TYPE_ARRAY &&
            v.GetArrayElementType() == DBUS_TYPE_BYTE) {
          for (ODBus::ReadMessageIter b = v.Recurse();
               b.GetArgType() == DBUS_TYPE_BYTE; b.Next()) {
            unsigned char ch = 0;
            b.GetBasic(&ch);
            ssid_out.push_back(static_cast<char>(ch));
          }
        } else if (v.GetArgType() == DBUS_TYPE_STRING) {
          const char *s = v.GetString();
          if (s != nullptr) {
            ssid_out = s;
          }
        }
      }
    }
  }
}

std::vector<NmClient::SavedConnection>
NmClient::ListSavedConnections(ODBus::Connection &c)
{
  std::vector<SavedConnection> out;
  std::vector<std::string> paths;
  ListObjectPaths(
    c, kNm, kNmSettingsPath, kNmIfaceSettings, "ListConnections", paths);

  for (const auto &path : paths) {
    try {
      using namespace ODBus;
      auto m = Message::NewMethodCall(
        kNm, path.c_str(), kNmIfaceSettingsConnection, "GetSettings");
      Message r = CallMethodSync(c, m);
      ReadMessageIter t{*r.Get()};
      SavedConnection saved;
      bool wifi = false;
      ParseWifiSettings(t, saved.ssid_text, &wifi,
                        &saved.connection_id);
      if (!wifi)
        continue;

      saved.path = path;
      out.push_back(std::move(saved));
    } catch (...) {
      LogFmt("Skipping NetworkManager saved connection '{}'", path);
      LogError(std::current_exception());
    }
  }

  return out;
}

static std::string
MakeConnectionId(const NmClient::AccessPoint &ap)
{
  std::string id;
  if (!ap.ssid_text.empty()) {
    id.reserve(ap.ssid_text.size());

    for (unsigned char ch : ap.ssid_text) {
      if (id.size() >= 200)
        break;

      if (ch >= 0x20 && ch <= 0x7e)
        id.push_back(static_cast<char>(ch));
      else
        id.push_back('_');
    }

    if (id.find_first_not_of(' ') != std::string::npos)
      return id;
  }

  return "WiFi network";
}

bool
NmClient::HasSavedConnectionForSsid(ODBus::Connection &c,
                                    const std::string &ssid) noexcept
{
  try {
    return !FindSavedConnectionPathForSsid(c, ssid).empty();
  } catch (...) {
    return false;
  }
}

std::string
NmClient::FindSavedConnectionPathForSsid(ODBus::Connection &c,
                                          const std::string &ssid)
{
  if (ssid.empty())
    return {};

  for (const auto &saved : ListSavedConnections(c))
    if (saved.ssid_text == ssid)
      return saved.path;

  return {};
}

static const char *
RequireConnectionPath(const char *connection_path, const char *method)
{
  if (connection_path == nullptr || *connection_path == 0)
    throw std::runtime_error{std::string{"Missing NetworkManager connection path for "} + method};

  return connection_path;
}

void
NmClient::Remove(ODBus::Connection &c, const char *connection_path)
{
  connection_path = RequireConnectionPath(connection_path, "Remove");

  using namespace ODBus;
  auto m = Message::NewMethodCall(kNm, connection_path,
             kNmIfaceSettingsConnection, "Delete");
  (void)CallMethodSync(c, m);
}

static const char *
RequireWifiDevicePath(const char *wifi_device, const char *method)
{
  if (wifi_device == nullptr || *wifi_device == '\0')
    throw std::runtime_error{std::string{"Missing NetworkManager Wi-Fi device path for "} + method};

  return wifi_device;
}

void
NmClient::ConnectSaved(ODBus::Connection &c, const char *wifi_device,
                       const char *connection_path, const char *ap_path)
{
  connection_path = RequireConnectionPath(connection_path, "ActivateConnection");
  wifi_device = RequireWifiDevicePath(wifi_device, "ActivateConnection");
  if (ap_path == nullptr || *ap_path == '\0')
    ap_path = "/";

  using namespace ODBus;
  auto m = Message::NewMethodCall(kNm, kNmPath, kNm, "ActivateConnection");
  DBusMessage *msg = m.Get();
  DBusMessageIter it;
  const char *p1 = connection_path;
  const char *p2 = wifi_device;
  const char *p3 = ap_path;
  dbus_message_iter_init_append(msg, &it);
  if (!dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &p1) ||
      !dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &p2) ||
      !dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &p3)) {
    throw std::runtime_error("ActivateConnection: build message");
  }
  (void)CallMethodSync(c, m);
}

static void
AddConnectionAndActivate(ODBus::Connection &c, const char *wifi_device,
                         const NmClient::AccessPoint &ap, const char *wpa2_psk)
{
  const std::string id = MakeConnectionId(ap);

  const std::string uu{NewConnectionUuid()};

  using namespace ODBus;
  auto m = Message::NewMethodCall(
    kNm, kNmPath, kNm, "AddAndActivateConnection");
  DBusMessage *msg = m.Get();
  DBusMessageIter it, a_ss;
  dbus_message_iter_init_append(msg, &it);
  if (!dbus_message_iter_open_container(&it, DBUS_TYPE_ARRAY, "{sa{sv}}", &a_ss)) {
    throw std::runtime_error("AddAndActivateConnection: open a{sa{sv}}");
  }
  {
    AppendSettingsSection(
      &a_ss, "connection",
      [&](DBusMessageIter *m2) {
        AppendStringVariantMapString(m2, "id", id.c_str());
        AppendStringVariantMapString(m2, "uuid", uu.c_str());
        AppendStringVariantMapString(
          m2, "type", "802-11-wireless");
      }
    );
    AppendSettingsSection(
      &a_ss, "802-11-wireless", [&](DBusMessageIter *m2) {
        AppendStringVariantMapBytes(
          m2, "ssid", ap.ssid_text.data(),
          (int)ap.ssid_text.size());
        AppendStringVariantMapString(
          m2, "mode", "infrastructure");
        if (wpa2_psk != nullptr && wpa2_psk[0] != '\0') {
          AppendStringVariantMapString(
            m2, "security", "802-11-wireless-security");
        }
      }
    );
    AppendSettingsSection(
        &a_ss, "ipv4", [&](DBusMessageIter *m2) {
            AppendStringVariantMapString(m2, "method", "auto");
        }
      );
    AppendSettingsSection(
        &a_ss, "ipv6", [&](DBusMessageIter *m2) {
            AppendStringVariantMapString(m2, "method", "auto");
        }
      );
    if (wpa2_psk != nullptr && wpa2_psk[0] != '\0') {
      AppendSettingsSection(
        &a_ss, "802-11-wireless-security", [&](DBusMessageIter *m2) {
          AppendStringVariantMapString(
            m2, "key-mgmt", "wpa-psk");
          AppendStringVariantMapString(m2, "psk", wpa2_psk);
        }
      );
    }
  }
  if (!dbus_message_iter_close_container(&it, &a_ss)) {
    throw std::runtime_error("AddAndActivateConnection: close a{sa{sv}}");
  }
  {
    const char *s_dev = wifi_device;
    const char *s_ap = ap.ap_path.c_str();
    if (!dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &s_dev) ||
        !dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &s_ap)) {
      throw std::runtime_error("AddAndActivateConnection: device paths");
    }
  }
  Message reply = CallMethodSync(c, m);
  (void)reply;
}

void
NmClient::Connect(ODBus::Connection &c, const char *wifi_device, const AccessPoint &ap,
                  const char *wpa2_psk_or_null)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "Connect");

  if (!ap.saved_path.empty()) {
    ConnectSaved(c, wifi_device, ap.saved_path.c_str(), ap.ap_path.c_str());
    return;
  }

  /* Reuse an existing profile for this SSID. Multiple Trendnet APs (BSSIDs)
     must not each trigger AddAndActivateConnection (NM then creates
     "Trendnet", "Trendnet 1", …). */
  const std::string saved_path = FindSavedConnectionPathForSsid(c, ap.ssid_text);
  if (!saved_path.empty()) {
    ConnectSaved(c, wifi_device, saved_path.c_str(), ap.ap_path.c_str());
    return;
  }

  const char *pw = ((wpa2_psk_or_null != nullptr) && wpa2_psk_or_null[0] != '\0')
    ? wpa2_psk_or_null
    : nullptr;
  if (pw != nullptr) {
    AddConnectionAndActivate(c, wifi_device, ap, pw);
  } else if (ap.needs_key) {
    throw WifiError::Exception{WifiError::Code::NeedKey};
  } else {
    AddConnectionAndActivate(c, wifi_device, ap, nullptr);
  }
}

std::string
NmClient::FindWifiDevice(ODBus::Connection &c)
{
  std::vector<std::string> devs;
  ListObjectPaths(c, kNm, kNmPath, kNm, "GetDevices", devs);
  for (const auto &d : devs) {
    std::uint32_t t = 0;
    LinuxNetWifi::DbusGetProperty(
      c, d.c_str(), kNmIfaceDevice, "DeviceType", nullptr, &t,
      nullptr);
    if (t == kNmDeviceTypeWifi)
      return d;
  }
  return {};
}

void
NmClient::SetWirelessEnabled(ODBus::Connection &c, bool on)
{
  LinuxNetWifi::DbusSetPropertyBoolean(
    c, kNmPath, kNm, "WirelessEnabled", on
  );
}

void
NmClient::RequestScan(ODBus::Connection &c, const char *wifi_device)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "RequestScan");

  using namespace ODBus;
  auto m = Message::NewMethodCall(
    kNm, wifi_device, kNmIfaceDeviceWireless,
    "RequestScan");
  {
    AppendMessageIter t{*m.Get()};
    AppendMessageIter o(t, DBUS_TYPE_ARRAY, "{sv}");
    o.CloseContainer(t);
  }
  (void)CallMethodSync(c, m);
}

std::vector<NmClient::AccessPoint>
NmClient::ListAccessPoints(ODBus::Connection &c, const char *wifi_device)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "ListAccessPoints");

  std::vector<AccessPoint> aps;
  std::vector<std::string> paths;
  ListObjectPaths(
    c, kNm, wifi_device, kNmIfaceDeviceWireless,
    "GetAllAccessPoints", paths);
  for (const std::string &p : paths) {
    AccessPoint ap;
    LinuxNetWifi::DbusGetByteStringProperty(
      c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "Ssid", ap.ssid_text);
    LinuxNetWifi::DbusGetProperty(
      c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "HwAddress",
      &ap.hw_address, nullptr, nullptr);
    ap.ap_path = p;
    std::uint32_t f = 0, w = 0, r = 0, strength = 0;
    LinuxNetWifi::DbusGetProperty(
      c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "Flags", nullptr, &f,
      nullptr);
    LinuxNetWifi::DbusGetProperty(
      c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "WpaFlags", nullptr, &w,
      nullptr);
    LinuxNetWifi::DbusGetProperty(
      c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "RsnFlags", nullptr, &r,
      nullptr);
    LinuxNetWifi::DbusGetProperty(
      c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "Strength", nullptr, &strength,
      nullptr);
    (void)w;
    (void)r;
    if (strength < 300U)
      ap.strength = (int)strength;
    if ((f & kApFlagPrivacy) != 0U || w != 0U || r != 0U)
      ap.needs_key = true;
    aps.push_back(std::move(ap));
  }
  return aps;
}

std::string
NmClient::GetActiveAccessPointPath(ODBus::Connection &c, const char *wifi_device)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "GetActiveAccessPointPath");

  std::string p;
  LinuxNetWifi::DbusGetProperty(
    c, wifi_device, kNmIfaceDeviceWireless, "ActiveAccessPoint",
    &p, nullptr, nullptr);
  if (LinuxNetWifi::DbusObjectPathIsEmpty(p)) {
    return {};
  }
  return p;
}

static std::string
ActiveSsid(ODBus::Connection &c, const char *wifi_device)
{
  const std::string p = NmClient::GetActiveAccessPointPath(c, wifi_device);
  if (p.empty()) {
    return {};
  }
  std::string s;
  LinuxNetWifi::DbusGetByteStringProperty(
    c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "Ssid", s);
  return s;
}

std::string
NmClient::FormatStatus(ODBus::Connection &c, const char *wifi_device)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "FormatStatus");

  bool wifion = true;
  LinuxNetWifi::DbusGetProperty(c, kNmPath, kNm, "WirelessEnabled", nullptr, 
                                nullptr, &wifion);
  if (!wifion)
    return _("Not connected");

  const std::string ssid = ActiveSsid(c, wifi_device);
  if (ssid.empty())
    return _("Not connected");

  return ssid;
}

void
NmClient::Disconnect(ODBus::Connection &c, const char *wifi_device)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "Disconnect");

  std::string ac;
  LinuxNetWifi::DbusGetProperty(
    c, wifi_device, kNmIfaceDevice, "ActiveConnection", &ac,
    nullptr, nullptr);
  if (LinuxNetWifi::DbusObjectPathIsEmpty(ac)) {
    return;
  }
  using namespace ODBus;
  auto m = Message::NewMethodCall(
    kNm, kNmPath, kNm, "DeactivateConnection");
  {
    DBusMessage *msg = m.Get();
    DBusMessageIter it;
    const char *p = ac.c_str();
    dbus_message_iter_init_append(msg, &it);
    if (!dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &p)) {
      throw std::runtime_error("DeactivateConnection: build");
    }
  }
  (void)CallMethodSync(c, m);
}

bool
NmClient::IsSameBssidAsActive(ODBus::Connection &c,
                              const char *active_access_point_path,
                              const AccessPoint &target) noexcept
{
  try {
    if (active_access_point_path == nullptr) {
      return false;
    }
    std::string a{active_access_point_path};
    if (LinuxNetWifi::DbusObjectPathIsEmpty(a)) {
      return false;
    }
    if (a == target.ap_path) {
      return true;
    }
    if (target.hw_address.empty()) {
      return false;
    }
    std::string h;
    LinuxNetWifi::DbusGetProperty(
      c, a.c_str(),
      "org.freedesktop.NetworkManager.AccessPoint", "HwAddress", &h,
      nullptr, nullptr);
    return !h.empty() && h == target.hw_address;
  } catch (...) {
    return false;
  }
}

void
NmClient::WaitUntilWifiDisconnected(ODBus::Connection &c, const char *wifi_device)
{
  wifi_device = RequireWifiDevicePath(wifi_device, "WaitUntilWifiDisconnected");

  using namespace std::chrono_literals;
  for (int i = 0; i < 150; i++) {
    std::uint32_t st = 0;
    LinuxNetWifi::DbusGetProperty(
      c, wifi_device, kNmIfaceDevice, "State", nullptr, &st, nullptr);
    if (st == kNmDeviceStateDisconnected) {
      return;
    }
    std::this_thread::sleep_for(100ms);
  }
}
