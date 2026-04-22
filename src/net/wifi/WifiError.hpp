// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>

/**
 * @brief Fixed exception messages from net/wifi code. Translated in
 * #FormatWifiErrorForUser for message boxes; keep stable for mapping.
 */
namespace WifiError {
inline constexpr const char GONE[] = "XCSOAR_WIFI_GONE";
inline constexpr const char NEED_KEY[] = "XCSOAR_WIFI_NEED_KEY";
/** NetworkManager: device in FAILED state. */
inline constexpr const char NM_FAIL[] = "XCSOAR_WIFI_NM_FAIL";
/** NetworkManager: no ACTIVATED before timeout. */
inline constexpr const char NM_TIMEOUT[] = "XCSOAR_WIFI_NM_TIMEOUT";
/** ConnMan: #Service.State failure / disconnect after connect. */
inline constexpr const char CM_FAIL[] = "XCSOAR_WIFI_CM_FAIL";
inline constexpr const char CM_TIMEOUT[] = "XCSOAR_WIFI_CM_TIMEOUT";
} // namespace WifiError

/**
 * Map #std::exception::what() to a string suitable for the UI (translated
 * for known D-Bus / tagged errors, English for unknown short technical text).
 */
std::string
FormatWifiErrorForUser(const char *what) noexcept;
