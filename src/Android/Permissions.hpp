// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <functional>

/**
 * Check if all required location permissions are granted.
 * On Android 10+ (API 29+), this checks both foreground and
 * background location permissions.
 */
bool
AreLocationPermissionsGranted() noexcept;

/**
 * Check if the notification permission is granted.
 * On Android 13+ (API 33+), this checks POST_NOTIFICATIONS.
 * On older versions, returns true (always permitted).
 */
bool
IsNotificationPermissionGranted() noexcept;

/**
 * Request all location permissions directly without showing
 * rationale dialogs.  The caller must have already shown the
 * Google-Play-required prominent disclosure.
 *
 * Chains: ACCESS_FINE_LOCATION -> ACCESS_BACKGROUND_LOCATION
 *         -> FOREGROUND_SERVICE_LOCATION
 */
void
RequestLocationPermissions() noexcept;

/**
 * Like RequestLocationPermissions(), but calls @p callback on the
 * native UI thread when the permission chain completes.
 */
void
RequestLocationPermissions(std::function<void(bool)> callback) noexcept;

/**
 * Request the notification permission directly without showing
 * a rationale dialog.  The caller must have already shown the
 * prominent disclosure.
 *
 * Only effective on Android 13+ (API 33+).
 */
void
RequestNotificationPermission() noexcept;

/**
 * Like RequestNotificationPermission(), but calls @p callback on the
 * native UI thread when the permission result arrives.
 */
void
RequestNotificationPermission(std::function<void(bool)> callback) noexcept;

/**
 * Suppress location and notification permission request dialogs
 * for the current session.  Call this when the user clicks
 * "Not Now" on the Quick Guide disclosure page, so that the
 * lazy permission flow does not immediately re-prompt.
 */
void
SuppressPermissionDialogs() noexcept;
