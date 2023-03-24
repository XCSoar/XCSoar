// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package de.opensoar.testing;

/**
 * Wrapper class to move de.opensoar.OpenSoar forward to package
 * de.opensoar.testing.
 */
public class MyService extends de.opensoar.MyService {
  @Override public void onCreate() {
    if (mainActivityClass == null)
      mainActivityClass = XCSoar.class;

    super.onCreate();
  }
}
