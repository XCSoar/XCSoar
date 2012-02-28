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

#include "Device/Driver/LX/Internal.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Internal.hpp"
#include "Atmosphere/Pressure.hpp"

#include <cstdio>

/**
 * Enable pass-through mode on the LX1600.  This command was provided
 * by Crtomir Rojnik (LX Navigation) in an email without further
 * explanation.  Tests have shown that this command can be sent at
 * either 4800 baud or the current vario baud rate.  Since both works
 * equally well, we don't bother to switch.
 */
static bool
ModeColibri(Port &port)
{
  return PortWriteNMEA(port, "PFLX0,COLIBRI");
}

/**
 * Cancel pass-through mode on the LX1600.  This command was provided
 * by Crtomir Rojnik (LX Navigation) in an email.  It must always be
 * sent at 4800 baud.  After this command has been sent, we switch
 * back to the "real" baud rate.
 */
static bool
ModeLX1600(Port &port)
{
  const unsigned old_baud_rate = port.SetBaudrate(4800);
  const bool success = PortWriteNMEA(port, "PFLX0,LX1600");

  if (old_baud_rate != 0 && old_baud_rate != 4800)
    port.SetBaudrate(old_baud_rate);

  return success;
}

static bool
EnableLXWP(Port &port)
{
  return PortWriteNMEA(port, "PFLX0,LXWP0,1,LXWP2,3,LXWP3,4");
}

bool
LXDevice::Open(gcc_unused OperationEnvironment &env)
{
  /* just in case the LX1600 is still in pass-through mode: */
  ModeLX1600(port);

  // This line initiates the Color Vario to send out LXWP2 and LXWP3
  // LXWP0 once started, is repeated every second
  // This is a copy of the initiation done in LK8000, realized by Lx developers
  // We have no documentation and so we do not know what this exactly means
  EnableLXWP(port);

  return true;
}

bool
LXDevice::PutBugs(fixed bugs)
{
  // This is a copy of the routine done in LK8000 for LX MiniMap, realized
  // by Lx developers. We have no documentation and so we do not know what this
  // exactly means.
  char tmp[100];
  int TransformedBugsValue = 100 - (int)(bugs*100);
  sprintf(tmp, "PFLX2,,,%d,,,", TransformedBugsValue);
  PortWriteNMEA(port, tmp);
  return true;
}

bool
LXDevice::PutMacCready(fixed MacCready)
{
  char szTmp[32];
  sprintf(szTmp, "PFLX2,%1.1f,,,,,,", (double)MacCready);
  PortWriteNMEA(port, szTmp);
  return true;
}

bool
LXDevice::PutQNH(const AtmosphericPressure &pres)
{
  fixed altitude_offset =
    pres.StaticPressureToQNHAltitude(AtmosphericPressure::Standard());
  char buffer[100];
  sprintf(buffer, "PFLX3,%.2f,,,,,,,,,,,,", (double)altitude_offset / 0.3048);
  PortWriteNMEA(port, buffer);
  return true;
}

bool
LXDevice::EnablePassThrough(OperationEnvironment &env)
{
  return ModeColibri(port);
}

void
LXDevice::DisablePassThrough()
{
  ModeLX1600(port);
}
