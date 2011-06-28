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

#include "Device/FLARM.hpp"
#include "Device/device.hpp"
#include "Device/Port.hpp"
#include "Device/Declaration.hpp"

#include "Operation.hpp"

#include <assert.h>
#include <tchar.h>

#ifdef _UNICODE
#include <windows.h>
#endif

static bool
FlarmDeclareSetGet(Port *port, char *Buffer)
{
  assert(port != NULL);
  assert(Buffer != NULL);

  port->Write('$');
  port->Write(Buffer);
  port->Write("\r\n");

  Buffer[6] = _T('A');
  return port->ExpectString(Buffer);
}

#ifdef _UNICODE
static bool
FlarmDeclareSetGet(Port *port, TCHAR *s)
{
  assert(port != NULL);
  assert(s != NULL);

  char buffer[_tcslen(s) * 4 + 1];
  return ::WideCharToMultiByte(CP_ACP, 0, s, -1, buffer, sizeof(buffer),
                               NULL, NULL) > 0 &&
         FlarmDeclareSetGet(port, buffer);
}
#endif

static bool
FlarmDeclareInternal(Port *port, const Declaration *decl,
                     OperationEnvironment &env)
{
  TCHAR Buffer[256];
  unsigned size = decl->size();

  env.SetProgressRange(6 + size);
  env.SetProgressPosition(0);

  _stprintf(Buffer, _T("PFLAC,S,PILOT,%s"), decl->PilotName.c_str());
  if (!FlarmDeclareSetGet(port, Buffer))
    return false;

  env.SetProgressPosition(1);

  _stprintf(Buffer, _T("PFLAC,S,GLIDERID,%s"), decl->AircraftReg.c_str());
  if (!FlarmDeclareSetGet(port, Buffer))
    return false;

  env.SetProgressPosition(2);

  _stprintf(Buffer, _T("PFLAC,S,GLIDERTYPE,%s"), decl->AircraftType.c_str());
  if (!FlarmDeclareSetGet(port, Buffer))
    return false;

  env.SetProgressPosition(3);

  _stprintf(Buffer, _T("PFLAC,S,NEWTASK,Task"));
  if (!FlarmDeclareSetGet(port, Buffer))
    return false;

  env.SetProgressPosition(4);

  _stprintf(Buffer, _T("PFLAC,S,ADDWP,0000000N,00000000E,TAKEOFF"));
  if (!FlarmDeclareSetGet(port, Buffer))
    return false;

  env.SetProgressPosition(5);

  for (unsigned i = 0; i < size; ++i) {
    int DegLat, DegLon;
    double tmp, MinLat, MinLon;
    char NoS, EoW;

    tmp = decl->get_location(i).Latitude.value_degrees();
    if (tmp < 0) {
      NoS = 'S';
      tmp = -tmp;
    } else {
      NoS = 'N';
    }
    DegLat = (int)tmp;
    MinLat = (tmp - DegLat) * 60 * 1000;

    tmp = decl->get_location(i).Longitude.value_degrees();
    if (tmp < 0) {
      EoW = 'W';
      tmp = -tmp;
    } else {
      EoW = 'E';
    }
    DegLon = (int)tmp;
    MinLon = (tmp - DegLon) * 60 * 1000;

    _stprintf(Buffer, _T("PFLAC,S,ADDWP,%02d%05.0f%c,%03d%05.0f%c,%s"), DegLat,
              MinLat, NoS, DegLon, MinLon, EoW, decl->get_name(i));

    if (!FlarmDeclareSetGet(port, Buffer))
      return false;

    env.SetProgressPosition(6 + i);
  }

  _stprintf(Buffer, _T("PFLAC,S,ADDWP,0000000N,00000000E,LANDING"));
  if (!FlarmDeclareSetGet(port, Buffer))
    return false;

  env.SetProgressPosition(6 + size);

  // PFLAC,S,KEY,VALUE
  // Expect
  // PFLAC,A,blah
  // PFLAC,,COPIL:
  // PFLAC,,COMPID:
  // PFLAC,,COMPCLASS:

  // PFLAC,,NEWTASK:
  // PFLAC,,ADDWP:

  // Reset the FLARM to activate the declaration
  port->Write("$PFLAR,0\r\n");

  return true;
}

bool
FlarmDeclare(Port *port, const Declaration *decl, OperationEnvironment &env)
{
  assert(port != NULL);

  port->StopRxThread();
  port->SetRxTimeout(500); // set RX timeout to 500[ms]

  bool result = FlarmDeclareInternal(port, decl, env);

  // TODO bug: JMW, FLARM Declaration checks
  // Only works on IGC approved devices
  // Total data size must not surpass 183 bytes
  // probably will issue PFLAC,ERROR if a problem?

  port->SetRxTimeout(0); // clear timeout
  port->StartRxThread(); // restart RX thread

  return result;
}
