// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.Closeable;

/**
 * The Java interface of the C++ AndroidSensor class.
 */
interface AndroidSensor extends Closeable {
  int STATE_READY = 0;
  int STATE_FAILED = 1;
  int STATE_LIMBO = 2;

  int getState();
}
