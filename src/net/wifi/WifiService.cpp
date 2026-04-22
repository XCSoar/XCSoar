// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WifiService.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "WifiError.hpp"
#include "lib/dbus/Connection.hxx"
#include "util/StaticString.hxx"
#include "LogFile.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <thread>

/* NetworkManager is preferred if both are registered (per project design). */
static constexpr const char *kNmName = "org.freedesktop.NetworkManager";
static constexpr const char *kConnmanName = "net.connman";

bool
WifiService::Detect()
{
	/* Clear prior scan rows; re-probe the bus. If NetworkManager is up but
	   exposes no WiFi device, we still fall back to ConnMan (so setup can
	   work when only ConnMan is useful).  Scan() does not fall back in that
	   case—it leaves rows empty. */
	rows.clear();
	backend = Backend::None;
	nm_device_path.clear();
	auto c = ODBus::Connection::GetSystem();
	if (!c) {
		return false;
	}
	if (LinuxNetWifi::NameHasOwner(c, kNmName)) {
		nm_device_path = NmWifi::FindWifiDevice(c);
		if (!nm_device_path.empty()) {
			backend = Backend::NetworkManager;
			return true;
		}
	}
	if (LinuxNetWifi::NameHasOwner(c, kConnmanName)) {
		backend = Backend::ConnMan;
		return true;
	}
	return false;
}

static std::string
FormatLabelNm(const NmWifi::AccessPoint &a)
{
	/* First row: ESSID only; #LinuxWifiListWidget::FormatDetail shows
	   security and signal (same two-line pattern as ConnMan). */
	return a.ssid_text;
}

void
WifiService::Scan()
{
	/* See comment in Detect(): if NM is present but has no WiFi device, we do
	   not list ConnMan here; caller keeps prior backend/rows as applicable. */
	rows.clear();
	auto c = ODBus::Connection::GetSystem();
	if (!c) {
		return;
	}
	if (LinuxNetWifi::NameHasOwner(c, kNmName)) {
		nm_device_path = NmWifi::FindWifiDevice(c);
		if (nm_device_path.empty()) {
			return;
		}
		backend = Backend::NetworkManager;
		const std::string active_ap =
			NmWifi::GetActiveAccessPointPath(c, nm_device_path.c_str());
		NmWifi::RequestScan(c, nm_device_path.c_str());
		for (auto &ap : NmWifi::ListAccessPoints(c, nm_device_path.c_str())) {
			Row r;
			r.row_backend = Backend::NetworkManager;
			r.nm = std::move(ap);
			r.list_label = FormatLabelNm(r.nm);
			if (!r.nm.ap_path.empty() && r.nm.ap_path == active_ap) {
				r.is_connected = true;
			}
			rows.push_back(std::move(r));
		}
		std::sort(
			rows.begin(), rows.end(), [](const Row &a, const Row &b) {
				if (a.nm.strength != b.nm.strength) {
					return a.nm.strength > b.nm.strength;
				}
				return a.nm.ssid_text < b.nm.ssid_text;
			});
	} else if (LinuxNetWifi::NameHasOwner(c, kConnmanName)) {
		backend = Backend::ConnMan;
		for (auto s : CmWifi::ListServices(c)) {
			Row r;
			r.row_backend = Backend::ConnMan;
			r.cm = std::move(s);
			if (!r.cm.ssid_text.empty()) {
				r.list_label = r.cm.ssid_text;
			} else {
				r.list_label = r.cm.path;
			}
			r.is_connected = CmWifi::IsActiveServiceState(r.cm.state);
			rows.push_back(std::move(r));
		}
		std::sort(
			rows.begin(), rows.end(), [](const Row &a, const Row &b) {
				if (a.cm.strength != b.cm.strength) {
					return a.cm.strength > b.cm.strength;
				}
				return a.cm.ssid_text < b.cm.ssid_text;
			});
	}
}

static std::optional<WifiService::Row>
MatchRowAfterScan(
    const std::vector<WifiService::Row> &in,
    const WifiService::Row &snap) noexcept
{
	if (snap.row_backend == WifiService::Backend::NetworkManager) {
		for (const auto &r : in) {
			if (r.row_backend == WifiService::Backend::NetworkManager &&
			    r.nm.ap_path == snap.nm.ap_path) {
				return r;
			}
		}
		/* D-Bus AccessPoint paths can be recreated; BSSID (HwAddress) is stable. */
		if (!snap.nm.hw_address.empty()) {
			for (const auto &r : in) {
				if (r.row_backend == WifiService::Backend::NetworkManager &&
				    r.nm.hw_address == snap.nm.hw_address) {
					return r;
				}
			}
		}
		if (!snap.nm.ssid_text.empty()) {
			int n = 0;
			WifiService::Row out{};
			for (const auto &r : in) {
				if (r.row_backend == WifiService::Backend::NetworkManager &&
				    r.nm.ssid_text == snap.nm.ssid_text) {
					n++;
					out = r;
				}
			}
			if (n == 1) {
				return out;
			}
		}
	} else if (snap.row_backend == WifiService::Backend::ConnMan) {
		for (const auto &r : in) {
			if (r.row_backend == WifiService::Backend::ConnMan &&
			    r.cm.path == snap.cm.path) {
				return r;
			}
		}
	}
	return std::nullopt;
}

bool
WifiService::HasSavedSystemPsk(ODBus::Connection &c, const Row &r) const noexcept
{
	if (r.row_backend != Backend::NetworkManager) {
		return false;
	}
	if (r.nm.ssid_text.empty()) {
		return false;
	}
	return NmWifi::HasSavedConnectionForSsid(c, r.nm.ssid_text);
}

std::string
WifiService::GetStatusLine()
{
	auto c = ODBus::Connection::GetSystem();
	if (!c)
		return "No D-Bus (system) connection";
	if (backend == Backend::NetworkManager && !nm_device_path.empty())
		return NmWifi::FormatStatus(c, nm_device_path.c_str());
	if (backend == Backend::ConnMan)
		return CmWifi::FormatStatus(c);
	return "";
}

void
WifiService::SetRadio(bool on)
{
	if (backend != Backend::NetworkManager)
		return;
	if (nm_device_path.empty())
		return;
	auto c = ODBus::Connection::GetSystem();
	if (c) {
		NmWifi::SetWirelessEnabled(c, on);
	}
}

void
WifiService::Connect(const Row &snapshot, const char *psk)
{
	auto c = ODBus::Connection::GetSystem();
	if (!c) {
		LogFmt("net/wifi: Connect: no D-Bus connection");
		throw std::runtime_error("No D-Bus connection");
	}
	Scan();
	const std::optional<Row> m = MatchRowAfterScan(rows, snapshot);
	if (!m) {
		throw std::runtime_error(WifiError::GONE);
	}
	if (m->row_backend == Backend::NetworkManager && nm_device_path.empty()) {
		LogFmt("net/wifi: Connect: no WiFi device after scan");
		throw std::runtime_error(WifiError::GONE);
	}
	try {
		if (m->row_backend == Backend::ConnMan) {
			/* If the user did not type a PSK, try Connect with stored
			   secret (SetPassphrase only when a new key is given). */
			if (psk != nullptr && psk[0] != '\0') {
				CmWifi::SetPassphrase(c, m->cm.path.c_str(), psk);
			}
			CmWifi::Connect(c, m->cm.path.c_str());
			CmWifi::WaitForServiceConnected(c, m->cm.path.c_str());
		} else {
			/* If already on another BSSID, deactivate and wait for #DISCONNECTED
			   so a saved “autoconnect” profile does not re-win before
			   #AddAndActivate for the row the user selected.  Compare by BSSID
			   (#HwAddress) as well, because D-Bus AP paths are recreated on scan. */
			const std::string cur = NmWifi::GetActiveAccessPointPath(
				c, nm_device_path.c_str());
			if (!LinuxNetWifi::DbusObjectPathIsEmpty(cur) &&
			    !NmWifi::IsSameBssidAsActive(c, cur.c_str(), m->nm)) {
				NmWifi::Disconnect(c, nm_device_path.c_str());
				NmWifi::WaitUntilWifiDisconnected(c, nm_device_path.c_str());
			}
			NmWifi::ConnectToAp(
				c, nm_device_path.c_str(), m->nm, psk);
		}
		/* Rebuild #rows so #is_connected and labels match the association we
		   just made (the pre-connect #Scan is stale in the dialog). */
		Scan();
	} catch (const std::exception &e) {
		LogFmt("net/wifi: D-Bus Connect failed: {}", e.what());
		throw;
	} catch (...) {
		LogFmt("net/wifi: D-Bus Connect failed: unknown error");
		throw;
	}
}

void
WifiService::ReconnectRadio()
{
	auto c = ODBus::Connection::GetSystem();
	if (!c) {
		return;
	}
	if (backend == Backend::NetworkManager && !nm_device_path.empty()) {
		NmWifi::SetWirelessEnabled(c, false);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		NmWifi::SetWirelessEnabled(c, true);
	} else {
		Scan();
	}
}

void
WifiService::Disconnect()
{
	auto c = ODBus::Connection::GetSystem();
	if (!c) {
		return;
	}
	if (backend == Backend::NetworkManager && !nm_device_path.empty()) {
		try {
			NmWifi::Disconnect(c, nm_device_path.c_str());
		} catch (const std::exception &e) {
			LogFmt("net/wifi: D-Bus NM Disconnect failed: {}", e.what());
			throw;
		}
	} else if (backend == Backend::ConnMan) {
		try {
			for (const auto &s : CmWifi::ListServices(c)) {
				if (CmWifi::IsActiveServiceState(s.state)) {
					CmWifi::Disconnect(c, s.path.c_str());
				}
			}
		} catch (const std::exception &e) {
			LogFmt("net/wifi: D-Bus CM Disconnect failed: {}", e.what());
			throw;
		}
	}
	Scan();
}
