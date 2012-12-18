/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DRIVER_LX_LX1600_HPP
#define XCSOAR_DEVICE_DRIVER_LX_LX1600_HPP

#include "Device/Port/Port.hpp"
#include "Device/Internal.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Units/System.hpp"

#include <assert.h>

/**
 * Code specific to LX Navigation varios (e.g. LX1600).
 */
namespace LX1600 {
  /**
   * Enable pass-through mode on the LX1600.  This command was provided
   * by Crtomir Rojnik (LX Navigation) in an email without further
   * explanation.  Tests have shown that this command can be sent at
   * either 4800 baud or the current vario baud rate.  Since both works
   * equally well, we don't bother to switch.
   */
  static inline bool
  ModeColibri(Port &port, OperationEnvironment &env)
  {
    return PortWriteNMEA(port, "PFLX0,COLIBRI", env);
  }

  /**
   * Cancel pass-through mode on the LX1600.  This command was provided
   * by Crtomir Rojnik (LX Navigation) in an email.  It must always be
   * sent at 4800 baud.  After this command has been sent, we switch
   * back to the "real" baud rate.
   */
  static inline bool
  ModeLX1600(Port &port, OperationEnvironment &env)
  {
    unsigned old_baud_rate = port.GetBaudrate();
    if (old_baud_rate == 4800)
      old_baud_rate = 0;
    else if (old_baud_rate != 0 && !port.SetBaudrate(4800))
      return false;

    const bool success = PortWriteNMEA(port, "PFLX0,LX1600", env);

    if (old_baud_rate != 0)
      port.SetBaudrate(old_baud_rate);

    return success;
  }

  static inline bool
  SetupNMEA(Port &port, OperationEnvironment &env)
  {
    /*
     * This line sets the requested NMEA sentences on the device.
     * LXWP0: every second
     * LXWP1-3: once and then after each change
     *
     * We have no official documentation.
     * This behavior was tested on an LX1606.
     */
    return PortWriteNMEA(port, "PFLX0,LXWP0,1,LXWP1,2,LXWP2,2,LXWP3,2", env);
  }

  /**
   * Set the MC setting of the LX16xx vario
   * @param mc in m/s
   */
  static inline bool
  SetMacCready(Port &port, OperationEnvironment &env, fixed mc)
  {
    assert(mc >= fixed(0.0) && mc <= fixed(5.0));

    char buffer[32];
    sprintf(buffer, "PFLX2,%1.1f,,,,,,", (double)mc);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the ballast setting of the LX16xx vario
   * @param overload 1.0 - 1.5 (100 - 140%)
   */
  static inline bool
  SetBallast(Port &port, OperationEnvironment &env, fixed overload)
  {
    assert(overload >= fixed(1.0) && overload <= fixed(1.5));

    // This is a copy of the routine done in LK8000 for LX MiniMap, realized
    // by Lx developers.

    char buffer[100];
    sprintf(buffer, "PFLX2,,%.2f,,,,", (double)overload);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the bugs setting of the LX16xx vario
   * @param bugs 0 - 30 %
   */
  static inline bool
  SetBugs(Port &port, OperationEnvironment &env, unsigned bugs)
  {
    assert(bugs <= 30);

    // This is a copy of the routine done in LK8000 for LX MiniMap, realized
    // by Lx developers.

    char buffer[100];
    sprintf(buffer, "PFLX2,,,%u,,,", bugs);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the altitude offset of the LX16xx vario
   * @param altitude_offset offset necessary to set QNE in ft (default=0)
   */
  static inline bool
  SetAltitudeOffset(Port &port, OperationEnvironment &env, fixed altitude_offset)
  {
    char buffer[100];
    sprintf(buffer, "PFLX3,%.2f,,,,,,,,,,,,", (double)altitude_offset);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the QNH setting of the LX16xx vario
   */
  static inline bool
  SetQNH(Port &port, OperationEnvironment &env, const AtmosphericPressure &qnh)
  {
    assert(qnh.IsPlausible());

    fixed altitude_offset = Units::ToUserUnit(
        qnh.StaticPressureToQNHAltitude(AtmosphericPressure::Standard()),
        Unit::FEET);

    return SetAltitudeOffset(port, env, altitude_offset);
  }

  /**
   * Set the audio volume setting of the LX16xx vario
   * @param volume 0 - 100 %
   */
  static inline bool
  SetVolume(Port &port, OperationEnvironment &env, unsigned volume)
  {
    assert(volume <= 100);

    if (volume > 99)
      volume = 99;

    char buffer[100];
    sprintf(buffer, "PFLX2,,,,,,,%u", volume);
    return PortWriteNMEA(port, buffer, env);
  }
}

#endif
