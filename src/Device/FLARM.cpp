/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Device/FLARM.hpp"
#include "Device/Internal.hpp"
#include "Device/device.h"

#include <assert.h>
#include <tchar.h>

static bool
FlarmDeclareSetGet(ComPort *port, TCHAR *Buffer) {
  assert(port != NULL);

  //PortWriteNMEA(d->Com, Buffer);

  TCHAR tmp[512];

  _sntprintf(tmp, 512, _T("$%s\r\n"), Buffer);

  port->WriteString(tmp);

  Buffer[6]= _T('A');
  return ExpectString(port, Buffer);
}

bool
FlarmDeclare(ComPort *port, const struct Declaration *decl)
{
  assert(port != NULL);

  bool result = true;

  TCHAR Buffer[256];

  port->StopRxThread();
  port->SetRxTimeout(500);                     // set RX timeout to 500[ms]

  _stprintf(Buffer, _T("PFLAC,S,PILOT,%s"),decl->PilotName);
  if (!FlarmDeclareSetGet(port, Buffer))
    result = false;

  _stprintf(Buffer, _T("PFLAC,S,GLIDERID,%s"),decl->AircraftRego);
  if (!FlarmDeclareSetGet(port, Buffer))
    result = false;

  _stprintf(Buffer, _T("PFLAC,S,GLIDERTYPE,%s"),decl->AircraftType);
  if (!FlarmDeclareSetGet(port, Buffer))
    result = false;

  _stprintf(Buffer, _T("PFLAC,S,NEWTASK,Task"));
  if (!FlarmDeclareSetGet(port, Buffer))
    result = false;

  _stprintf(Buffer, _T("PFLAC,S,ADDWP,0000000N,00000000E,TAKEOFF"));
  if (!FlarmDeclareSetGet(port, Buffer))
    result = false;

#ifdef OLD_TASK

  for (int i = 0; i < decl->num_waypoints; i++) {
    int DegLat, DegLon;
    double tmp, MinLat, MinLon;
    char NoS, EoW;

    tmp = decl->waypoint[i]->Location.Latitude;
    NoS = 'N';
    if(tmp < 0)
      {
	NoS = 'S';
	tmp = -tmp;
      }
    DegLat = (int)tmp;
    MinLat = (tmp - DegLat) * 60 * 1000;

    tmp = decl->waypoint[i]->Location.Longitude;
    EoW = 'E';
    if(tmp < 0)
      {
	EoW = 'W';
	tmp = -tmp;
      }
    DegLon = (int)tmp;
    MinLon = (tmp - DegLon) * 60 * 1000;

    _stprintf(Buffer,
	      _T("PFLAC,S,ADDWP,%02d%05.0f%c,%03d%05.0f%c,%s"),
	      DegLat, MinLat, NoS, DegLon, MinLon, EoW,
	      decl->waypoint[i]->Name);
    if (!FlarmDeclareSetGet(port, Buffer))
      result = false;
  }
#endif

  _stprintf(Buffer, _T("PFLAC,S,ADDWP,0000000N,00000000E,LANDING"));
  if (!FlarmDeclareSetGet(port, Buffer))
    result = false;

  // PFLAC,S,KEY,VALUE
  // Expect
  // PFLAC,A,blah
  // PFLAC,,COPIL:
  // PFLAC,,COMPID:
  // PFLAC,,COMPCLASS:

  // PFLAC,,NEWTASK:
  // PFLAC,,ADDWP:

  // TODO bug: JMW, FLARM Declaration checks
  // Note: FLARM must be power cycled to activate a declaration!
  // Only works on IGC approved devices
  // Total data size must not surpass 183 bytes
  // probably will issue PFLAC,ERROR if a problem?

  port->SetRxTimeout(0); // clear timeout
  port->StartRxThread(); // restart RX thread

  return result;
}
