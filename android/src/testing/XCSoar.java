// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar.testing;

import android.os.Bundle;

/**
 * Wrapper class to move org.xcsoar.XCSoar forward to package
 * org.xcsoar.testing.
 */
public class XCSoar extends org.xcsoar.XCSoar {
  @Override protected void onCreate(Bundle savedInstanceState) {
    if (serviceClass == null)
      serviceClass = MyService.class;

    super.onCreate(savedInstanceState);
  }
}
