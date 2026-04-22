// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConnmanWifi.hpp"
#include "NetworkManagerWifi.hpp"
#include "lib/dbus/Connection.hxx"

#include <cstddef>
#include <string>
#include <vector>

/**
 * D-Bus WiFi control: NetworkManager is preferred over ConnMan when both
 * are on the bus (see implementation).
 */
class WifiService {
public:
	enum class Backend : std::uint8_t { None, NetworkManager, ConnMan };

	struct Row {
		std::string list_label;
		/** True if this row is the current connection (ConnMan state or NM AP). */
		bool is_connected{false};
		Backend row_backend{Backend::None};
		NmWifi::AccessPoint nm{};
		CmWifi::ServiceEntry cm{};
	};

private:
	Backend backend{Backend::None};
	/** Set when using NetworkManager. */
	std::string nm_device_path;
	std::vector<Row> rows;

public:
	WifiService() noexcept = default;

	/**
	 * Detect NetworkManager or ConnMan on the system D-Bus.
	 * @return false if neither is available
	 */
	bool
	Detect();

	[[gnu::pure]]
	Backend
	GetBackend() const noexcept
	{
		return backend;
	}

	std::string
	GetStatusLine();

	void
	SetRadio(bool on);

	/**
	 * Rescan; updates #rows. Run off the UI thread.
	 */
	void
	Scan();

	const std::vector<Row> &
	GetRows() const noexcept
	{
		return rows;
	}

	/**
	 * True if this row can use a system-stored PSK (NetworkManager profile for
	 * the SSID). ConnMan does not support this check here; call only for NM.
	 */
	bool
	HasSavedSystemPsk(ODBus::Connection &c, const Row &r) const noexcept;

	/**
	 * @param psk null or empty to use a saved key (or open / ConnMan
	 *        stored), where supported
	 * @throws std::runtime_error; pass `what()` to `FormatWifiErrorForUser` for
	 *        a translated user message
	 */
	void
	Connect(const Row &snapshot, const char *psk);

	void
	Disconnect();

	/**
	 * Nudge the radio (NetworkManager: brief off/on) or rescan (ConnMan).
	 * May be slow; run off the UI thread.
	 */
	void
	ReconnectRadio();
};
