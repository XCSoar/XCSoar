// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * A listener for #AndroidPort objects.
 */
interface PortListener {
  /**
   * The state has changed, and AndroidPort.getState() will provide
   * the new value.
   */
  void portStateChanged();

  /**
   * An error has occurred, and the #Port is now permanently
   * PortState::FAILED.
   *
   * @param msg a human-readable error message (probably not
   * localised)
   */
  void portError(String msg);
}
