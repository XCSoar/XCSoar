// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * An interface that allows requesting Android app permissions
 * asynchronously.  This decouples and simplifies the
 * Activity.requestPermissions() method and allows multiplexing the
 * Activity.onRequestPermissionsResult() to multiple registered
 * handlers.
 */
interface PermissionManager {
  interface PermissionHandler {
    public void onRequestPermissionsResult(boolean granted);
  }

  boolean requestPermission(String permission, PermissionHandler handler);
  void cancelRequestPermission(PermissionHandler handler);
  
  /**
   * Check if all required location permissions are granted.
   * On Android 10+ (API 29+), this checks both foreground and background
   * location permissions. On older versions, only foreground location is checked.
   * @return true if all required location permissions are granted, false otherwise
   */
  boolean areLocationPermissionsGranted();

  /**
   * Check if the notification permission is granted.
   * On Android 13+ (API 33+), this checks POST_NOTIFICATIONS.
   * On older versions, notifications are always permitted.
   * @return true if notification permission is granted
   */
  boolean isNotificationPermissionGranted();

  /**
   * Request all location permissions directly (foreground, background,
   * foreground service location) without showing rationale dialogs.
   * The caller (onboarding disclosure page) has already shown the
   * prominent disclosure as required by Google Play policy.
   *
   * Chains: ACCESS_FINE_LOCATION -> ACCESS_BACKGROUND_LOCATION (API 29+)
   *         -> FOREGROUND_SERVICE_LOCATION (API 34+)
   */
  void requestAllLocationPermissionsDirect();

  /**
   * Request the notification permission directly without showing a
   * rationale dialog.  The caller has already shown the prominent
   * disclosure.
   *
   * Only effective on Android 13+ (API 33+).
   */
  void requestNotificationPermissionDirect();

  /**
   * Suppress location and notification permission request dialogs
   * for the current session.  Call this when the user clicks
   * "Not Now" on the onboarding disclosure page, so that the
   * lazy permission flow (InternalGPS, MyService) does not
   * immediately re-prompt.
   */
  void suppressPermissionDialogs();

  /**
   * Called from native code when the user responds to the native
   * permission disclosure dialog.
   *
   * @param accepted true if the user clicked "Continue", false for "Not Now"
   */
  void onDisclosureResult(boolean accepted);
}
