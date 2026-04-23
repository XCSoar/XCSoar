// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/wifi/WifiBackend.hpp"

/**
 * CreatePlatformWifiBackend() returns the Kobo-specific UniqueWifiBackend
 * factory result backed by wpa_supplicant.
 *
 * The returned UniqueWifiBackend owns the created implementation. Unsupported
 * builds return an empty UniqueWifiBackend, and backend construction may throw
 * if initialization fails.
 */
UniqueWifiBackend
CreatePlatformWifiBackend();