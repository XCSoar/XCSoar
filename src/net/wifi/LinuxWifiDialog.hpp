// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class WifiService;

#if defined(HAVE_LINUX_NET_WIFI)
/**
 * Full-screen list of access points: scan, connect, disconnect, radio nudge.
 * @param service A service that was successfully passed #WifiService::Detect().
 */
void
ShowLinuxWifiDialog(WifiService &service) noexcept;
#endif
