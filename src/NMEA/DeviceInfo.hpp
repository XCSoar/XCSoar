// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <type_traits>

/**
 * Some generic information about the connected device.
 *
 * This struct was initially modeled after the $LXWP1 sentence (LXNav
 * and LX Navigation).
 *
 * There is no Validity attribute.  All strings that are non-empty can
 * be assumed to be valid.  Also note that there is no Expire()
 * method, because we assume the device may send the data once on
 * startup and never again.  Having this initial value is better than
 * nothing.
 */
struct DeviceInfo {
  /**
   * The name of the product.
   */
  StaticString<16> product;

  /**
   * The serial number.  This is a string because we're not sure if a
   * device sends non-numeric data here.
   */
  StaticString<16> serial;

  /**
   * The hardware version number.
   */
  StaticString<16> hardware_version;

  /**
   * The software (or firmware) version number.
   */
  StaticString<16> software_version;

  void Clear() {
    product.clear();
    serial.clear();
    hardware_version.clear();
    software_version.clear();
  }
};

static_assert(std::is_trivial<DeviceInfo>::value, "type is not trivial");
