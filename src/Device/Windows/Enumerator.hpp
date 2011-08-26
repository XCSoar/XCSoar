/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_SERIAL_PORT_HPP
#define XCSOAR_DEVICE_SERIAL_PORT_HPP

#include "Config/Registry.hpp"
#include "Util/StaticString.hpp"

/**
 * A class that can enumerate COMM ports on Windows CE, by reading the
 * registry.
 */
class PortEnumerator {
  RegistryKey drivers_active;
  unsigned i;

  StaticString<64> name;
  StaticString<256> display_name;

public:
  PortEnumerator();

  /**
   * Has this object failed to open the Windows Registry?
   */
  bool Error() const {
    return drivers_active.error();
  }

  /**
   * Find the next device (or the first one, if this method hasn't
   * been called so far).
   *
   * @return true if a device was found, false if there are no more
   * devices
   */
  bool Next();

  const TCHAR *GetName() const {
    return name;
  }

  const TCHAR *GetDisplayName() const {
    return display_name;
  }
};

#endif
