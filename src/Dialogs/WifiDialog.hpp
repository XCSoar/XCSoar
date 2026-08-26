// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/wifi/WifiBackend.hpp"

#include <functional>

void
ShowWifiDialog(UniqueWifiBackend backend);

/**
 * Open the platform WiFi list / system WiFi settings (same as Config →
 * Setup → Network → WiFi List).
 *
 * @param after optional callback after an in-app WiFi dialog closes
 * (Kobo / Linux); ignored on Android / Windows / iOS.
 */
void
OpenWifiList(std::function<void()> after = {}) noexcept;
