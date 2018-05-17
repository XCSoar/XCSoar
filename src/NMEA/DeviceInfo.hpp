/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_VERSION_INFO_H
#define XCSOAR_VERSION_INFO_H

#include "Util/StaticString.hxx"

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
  NarrowString<16> product;

  /**
   * The serial number.  This is a string because we're not sure if a
   * device sends non-numeric data here.
   */
  NarrowString<16> serial;

  /**
   * The hardware version number.
   */
  NarrowString<16> hardware_version;

  /**
   * The software (or firmware) version number.
   */
  NarrowString<16> software_version;

  void Clear() {
    product.clear();
    serial.clear();
    hardware_version.clear();
    software_version.clear();
  }
};

static_assert(std::is_trivial<DeviceInfo>::value, "type is not trivial");

#endif
