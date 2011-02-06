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

#ifndef XCSOAR_PROFILE_DEVICE_CONFIG_HPP
#define XCSOAR_PROFILE_DEVICE_CONFIG_HPP

#include <tchar.h>

/**
 * Configuration structure for serial devices
 */
struct DeviceConfig {
  enum port_type {
    /**
     * Serial port, i.e. COMx / RS-232.
     */
    SERIAL,

    /**
     * Attempt to auto-discover the GPS source.
     *
     * On Windows CE, this opens the GPS Intermediate Driver Multiplexer.
     * @see http://msdn.microsoft.com/en-us/library/bb202042.aspx
     */
    AUTO,

    /**
     * The built-in GPS receiver.
     */
    INTERNAL,
  };

  port_type port_type;          /**< Type of the port */
  unsigned port_index;          /**< Index of the port */
  unsigned speed_index;         /**< Speed index (baud rate) */
  TCHAR driver_name[32];        /**< Name of the driver */
};

namespace Profile
{
  void GetDeviceConfig(unsigned n, DeviceConfig &config);
  void SetDeviceConfig(unsigned n, const DeviceConfig &config);
};

#endif
