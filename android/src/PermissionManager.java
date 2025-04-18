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
}
