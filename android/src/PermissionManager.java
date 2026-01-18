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
}
