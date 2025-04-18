// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * The state of a #Port object.
 *
 * @see Port::GetState()
 */
enum class PortState {
  /**
   * The port is open and ready to be used.
   */
  READY,

  /**
   * There was an error.  This object cannot be used anymore, and
   * should be deleted.
   */
  FAILED,

  /**
   * Waiting for a connection to the device.
   */
  LIMBO,
};
