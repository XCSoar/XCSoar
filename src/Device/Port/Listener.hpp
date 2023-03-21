// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A listener for #Port objects.
 */
class PortListener {
public:
  /**
   * The state has changed, and Port::GetState() will provide the new
   * value.
   */
  virtual void PortStateChanged() noexcept = 0;

  /**
   * An error has occurred, and the #Port is now permanently
   * PortState::FAILED.
   *
   * @param msg a human-readable error message (probably not
   * localised)
   */
  virtual void PortError([[maybe_unused]] const char *msg) noexcept {}
};
