// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package de.opensoar.testing;

import android.os.Bundle;

/**
 * Wrapper class to move de.opensoar.OpenSoar forward to package
 * de.opensoar.testing.
 */
public class OpenSoar extends de.opensoar.OpenSoar {
  @Override protected void onCreate(Bundle savedInstanceState) {
    if (serviceClass == null)
      serviceClass = MyService.class;

    super.onCreate(savedInstanceState);
  }
}
