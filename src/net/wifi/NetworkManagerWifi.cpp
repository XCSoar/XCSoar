// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkManagerWifi.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "WifiError.hpp"
#include "lib/dbus/AppendIter.hxx"
#include "lib/dbus/CallMethodSync.hxx"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Message.hxx"
#include "lib/dbus/ReadIter.hxx"
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"

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
static constexpr std::uint32_t kNmDeviceStateFailed = 120U;

/**
 * #NM_DEVICE_STATE: CONFIG(50) … through ACTIVATED(100) can all show the
 * correct #ActiveAccessPoint while IP is not ready. Requiring only 100
 * caused timeouts when switching APs or on slow DHCP.
 * We exclude NEED_AUTH(60) (wrong key / extra secrets) and low states.
 */
static bool
NmStateShowsTargetAssociation(std::uint32_t st) noexcept
{
  return st == 50U || st == 70U || st == 80U || st == 90U || st == 100U;
}

static bool
TargetApIsNowActive(ODBus::Connection &c, const char *wifi_device,
                      const NmWifi::AccessPoint &target)
{
  try {
    std::string ap;
    LinuxNetWifi::DbusGetProperty(
      c, wifi_device, kNmIfaceDeviceWireless, "ActiveAccessPoint", &ap,
      nullptr, nullptr);
    if (LinuxNetWifi::DbusObjectPathIsEmpty(ap)) {
      return false;
    }
    if (ap == target.ap_path) {
      return true;
    }
    std::string s;
    LinuxNetWifi::DbusGetByteStringProperty(
      c, ap.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "Ssid", s);
    return !target.ssid_text.empty() && s == target.ssid_text;
  } catch (...) {
    return false;
  }
}

static void
WaitForNmWifiResult(ODBus::Connection &c, const char *wifi_device,
                    const NmWifi::AccessPoint &target)
{
  using namespace std::chrono_literals;
  const auto interval = 200ms;
  constexpr int kMaxIters = 200;
  for (int left = kMaxIters; left > 0; --left) {
    std::uint32_t st = 0;
    LinuxNetWifi::DbusGetProperty(
      c, wifi_device, kNmIfaceDevice, "State", nullptr, &st, nullptr);
    if (st == kNmDeviceStateFailed) {
      throw std::runtime_error(WifiError::NM_FAIL);
    }
    if (TargetApIsNowActive(c, wifi_device, target) &&
        NmStateShowsTargetAssociation(st)) {
      return;
    }
    std::this_thread::sleep_for(interval);
  }
  throw std::runtime_error(WifiError::NM_TIMEOUT);
}

/** New UUID (v4) for #connection; required for valid NetworkManager profiles. */
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
NMDbusObjectPaths(ODBus::Connection &c, const char *dest, const char *obj_path,
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

/**
 * Parse #GetSettings result: set @a wifi if type is 802-11-wireless, and
 * ESSID from [802-11-wireless] ssid (ay or s).
 * @param connection_id_out if non-null, filled from [connection] #id
 */
static void
ParseGetSettingsForWifi(ODBus::ReadMessageIter &t, std::string &ssid_out,
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

static std::string
FindSavedWifiConnectionPath(ODBus::Connection &c, const std::string &ssid_wanted)
{
	std::vector<std::string> paths;
	NMDbusObjectPaths(
		c, kNm, kNmSettingsPath, kNmIfaceSettings, "ListConnections", paths);
	for (const std::string &p : paths) {
		try {
			using namespace ODBus;
			auto m = Message::NewMethodCall(
				kNm, p.c_str(), kNmIfaceSettingsConnection, "GetSettings");
			Message r = CallMethodSync(c, m);
			ReadMessageIter t{*r.Get()};
			std::string ssid;
			bool wifi = false;
			ParseGetSettingsForWifi(t, ssid, &wifi);
			if (wifi && ssid == ssid_wanted) {
				return p;
			}
		} catch (...) {
		}
	}
	return {};
}

/**
 * Remove saved profiles created by XCSoar (connection \a id with prefix
 * \c xcsoar- / \c xcsoar-ap for a hidden network) for the same SSID before
 * #AddAndActivate, so NetworkManager does not add another profile for each
 * connect.
 */
static void
DeleteXcsoarWifiProfilesForSsid(ODBus::Connection &c, const NmWifi::AccessPoint &ap)
{
	const bool empty_ssid = ap.ssid_text.empty();
	std::vector<std::string> paths;
	NMDbusObjectPaths(
		c, kNm, kNmSettingsPath, kNmIfaceSettings, "ListConnections", paths);
	for (const std::string &p : paths) {
		std::string connection_id;
		try {
			using namespace ODBus;
			auto m = Message::NewMethodCall(
				kNm, p.c_str(), kNmIfaceSettingsConnection, "GetSettings");
			Message r = CallMethodSync(c, m);
			ReadMessageIter t{*r.Get()};
			std::string ssid;
			bool wifi = false;
			ParseGetSettingsForWifi(t, ssid, &wifi, &connection_id);
			if (!wifi) {
				continue;
			}
			bool is_xcsoar = false;
			if (empty_ssid) {
				/* NmWifi::AddConnectionAndActivate uses id "xcsoar-ap". */
				is_xcsoar = (connection_id == "xcsoar-ap");
			} else {
				is_xcsoar = StringStartsWith(connection_id.c_str(), "xcsoar-") &&
					ssid == ap.ssid_text;
			}
			if (!is_xcsoar) {
				continue;
			}
		} catch (...) {
			continue;
		}
		try {
			using namespace ODBus;
			auto md = Message::NewMethodCall(
				kNm, p.c_str(), kNmIfaceSettingsConnection, "Delete");
			(void)CallMethodSync(c, md);
		} catch (...) {
		}
	}
}

bool
NmWifi::HasSavedConnectionForSsid(ODBus::Connection &c,
				 const std::string &ssid) noexcept
{
	try {
		if (ssid.empty()) {
			return false;
		}
		return !FindSavedWifiConnectionPath(c, ssid).empty();
	} catch (...) {
		return false;
	}
}

static void
ActivateSavedConnection(ODBus::Connection &c, const char *connection_path,
			const char *wifi_device, const char *ap_path)
{
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
                         const NmWifi::AccessPoint &ap, const char *wpa2_psk)
{
	DeleteXcsoarWifiProfilesForSsid(c, ap);
	std::string id("xcsoar-");
	if (!ap.ssid_text.empty()) {
		for (char ch : ap.ssid_text) {
			if (id.size() > 200)
				break;
			if (IsAlphaNumericASCII(ch) || ch == '-')
				id += ch;
			else
				id += '_';
		}
	} else
		id += "ap";

	const std::string uu{NewConnectionUuid()};

	using namespace ODBus;
	/* AddConnection lives on Settings, not on org.freedesktop.NetworkManager.
	   AddAndActivateConnection is on the manager and does both in one call.
	   A unique #connection.uuid and #802-11-wireless.security for WPA are
	   required: without security=802-11-wireless-security the PSK section
	   is ignored and activation may "succeed" on D-Bus but not associate. */
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
			});
		AppendSettingsSection(
			&a_ss, "802-11-wireless", [&](DBusMessageIter *m2) {
				AppendStringVariantMapBytes(
					m2, "ssid", ap.ssid_text.data(),
					(int)ap.ssid_text.size());
				AppendStringVariantMapString(
					m2, "mode", "infrastructure");
				if (wpa2_psk != nullptr && wpa2_psk[0] != '\0') {
					/* Link the [802-11-wireless-security] group (NM keyfile). */
					AppendStringVariantMapString(
						m2, "security", "802-11-wireless-security");
				}
			});
		AppendSettingsSection(
			&a_ss, "ipv4", [&](DBusMessageIter *m2) {
				AppendStringVariantMapString(m2, "method", "auto");
			});
		AppendSettingsSection(
			&a_ss, "ipv6", [&](DBusMessageIter *m2) {
				AppendStringVariantMapString(m2, "method", "auto");
			});
		if (wpa2_psk != nullptr && wpa2_psk[0] != '\0') {
			AppendSettingsSection(
				&a_ss, "802-11-wireless-security", [&](DBusMessageIter *m2) {
					AppendStringVariantMapString(
						m2, "key-mgmt", "wpa-psk");
					AppendStringVariantMapString(m2, "psk", wpa2_psk);
				});
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
	(void)CallMethodSync(c, m);
}

void
NmWifi::ConnectToAp(ODBus::Connection &c, const char *wifi_device, const AccessPoint &ap,
                   const char *wpa2_psk_or_null)
{
	const char *pw = ((wpa2_psk_or_null != nullptr) && wpa2_psk_or_null[0] != '\0')
		? wpa2_psk_or_null
		: nullptr;
	if (pw != nullptr) {
		AddConnectionAndActivate(c, wifi_device, ap, pw);
	} else if (ap.needs_key) {
		const std::string saved{FindSavedWifiConnectionPath(c, ap.ssid_text)};
		if (!saved.empty()) {
			ActivateSavedConnection(
				c, saved.c_str(), wifi_device, ap.ap_path.c_str());
		} else {
			throw std::runtime_error(WifiError::NEED_KEY);
		}
	} else {
		AddConnectionAndActivate(c, wifi_device, ap, nullptr);
	}
	WaitForNmWifiResult(c, wifi_device, ap);
}

std::string
NmWifi::FindWifiDevice(ODBus::Connection &c)
{
	std::vector<std::string> devs;
	NMDbusObjectPaths(c, kNm, kNmPath, kNm, "GetDevices", devs);
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
NmWifi::SetWirelessEnabled(ODBus::Connection &c, bool on)
{
	LinuxNetWifi::DbusSetPropertyBoolean(
		c, kNmPath, kNm, "WirelessEnabled", on);
}

void
NmWifi::RequestScan(ODBus::Connection &c, const char *wifi_device)
{
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
	std::this_thread::sleep_for(std::chrono::seconds(2));
}

std::vector<NmWifi::AccessPoint>
NmWifi::ListAccessPoints(ODBus::Connection &c, const char *wifi_device)
{
	std::vector<AccessPoint> aps;
	std::vector<std::string> paths;
	NMDbusObjectPaths(
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
		(void)w; /* for future WPA vs WPA2/3 distinction */
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
NmWifi::GetActiveAccessPointPath(ODBus::Connection &c, const char *wifi_device)
{
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
	const std::string p = NmWifi::GetActiveAccessPointPath(c, wifi_device);
	if (p.empty()) {
		return {};
	}
	std::string s;
	LinuxNetWifi::DbusGetByteStringProperty(
		c, p.c_str(), "org.freedesktop.NetworkManager.AccessPoint", "Ssid", s);
	return s;
}

std::string
NmWifi::FormatStatus(ODBus::Connection &c, const char *wifi_device)
{
	bool wifion = true;
	LinuxNetWifi::DbusGetProperty(c, kNmPath, kNm, "WirelessEnabled", nullptr, nullptr,
					&wifion);
	if (!wifion)
		return "WiFi off (radio)";
	const std::string ssid = ActiveSsid(c, wifi_device);
	if (ssid.empty())
		return "WiFi: not associated";
	return std::string("WiFi: ") + ssid;
}

void
NmWifi::Disconnect(ODBus::Connection &c, const char *wifi_device)
{
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
NmWifi::IsSameBssidAsActive(ODBus::Connection &c,
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
NmWifi::WaitUntilWifiDisconnected(ODBus::Connection &c, const char *wifi_device)
{
	using namespace std::chrono_literals;
	/* #Disconnect returns before the device is in #DISCONNECTED(30);
	   otherwise autoconnect can re-join the old profile before
	   #ConnectToAp for the user’s chosen AP. */
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
