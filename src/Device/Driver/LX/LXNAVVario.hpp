/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DRIVER_LX_LXVARIO_HPP
#define XCSOAR_DEVICE_DRIVER_LX_LXVARIO_HPP

#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

/**
 * Code specific to LXNav varios (e.g. V7).
 *
 * Source: LXNav Vario Dataport specification Version 1.03
 */
namespace LXNAVVario {
  /**
   * Enable direct link with GPS port.
   */
  static inline void
  ModeDirect(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXV0,CONNECTION,W,DIRECT", env);
  }

  /**
   * Enable communication with the vario.
   */
  static inline void
  ModeNormal(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXV0,CONNECTION,W,VSEVEN", env);
  }

  /**
   * Set up the NMEA sentences sent by the vario:
   *
   * - PLXVF at 2 Hz
   * - PLXVS every 5 seconds
   * - LXWP0 every second
   * - LXWP1 every 60 seconds
   * - LXWP2 every 30 seconds
   * - LXWP3 disabled (we don't parse it)
   * - LXWP5 disabled (we don't parse it)
   */
  static inline void
  SetupNMEA(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXV0,NMEARATE,W,2,5,1,60,30,0,0", env);
  }

  /**
   * Set the MC setting of the vario
   * @param mc in m/s
   */
  static inline void
  SetMacCready(Port &port, OperationEnvironment &env, double mc)
  {
    char buffer[32];
    sprintf(buffer, "PLXV0,MC,W,%.1f", mc);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the ballast setting of the vario
   * @param overload 1.0 - 1.4 (100 - 140%)
   */
  static inline void
  SetBallast(Port &port, OperationEnvironment &env, double overload)
  {
    char buffer[100];
    sprintf(buffer, "PLXV0,BAL,W,%.2f", overload);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the bugs setting of the vario
   * @param bugs 0 - 30 %
   */
  static inline void
  SetBugs(Port &port, OperationEnvironment &env, unsigned bugs)
  {
    char buffer[100];
    sprintf(buffer, "PLXV0,BUGS,W,%u", bugs);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the QNH setting of the vario
   */
  static inline void
  SetQNH(Port &port, OperationEnvironment &env, const AtmosphericPressure &qnh)
  {
    char buffer[100];
    unsigned QNHinPascal = uround(qnh.GetPascal());
    sprintf(buffer, "PLXV0,QNH,W,%u", QNHinPascal); 
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Send pilotevent to vario 
   * (needs S10x/S8x firmware 8.01 or newer)
   */
  static inline void
  PutPilotEvent(OperationEnvironment &env, Port &port)
  {
    const char *sentence = "PFLAI,PILOTEVENT";

    PortWriteNMEA(port, sentence, env);
  }
}

#endif
