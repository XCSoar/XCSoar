// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar.testing;

/**
 * Wrapper class to move org.xcsoar.XCSoar forward to package
 * org.xcsoar.testing.
 */
public class MyService extends org.xcsoar.MyService {
  @Override public void onCreate() {
    if (mainActivityClass == null)
      mainActivityClass = XCSoar.class;

    super.onCreate();
  }
}
