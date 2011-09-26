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

#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver.hpp"
#include "Device/Internal.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Units.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <tchar.h>
#ifdef _UNICODE
#include <windows.h>
#endif

#define DECELWPNAMESIZE   24                        // max size of taskpoint name
#define DECELWPSIZE       DECELWPNAMESIZE + 25      // max size of WP declaration

class AltairProDevice : public AbstractDevice {
private:
  Port *port;

  bool DeclareInternal(const struct Declaration &declaration);
  void PutTurnPoint(const TCHAR *name, const Waypoint *waypoint);
  bool PropertySetGet(Port *port, char *Buffer, size_t size);
#ifdef _UNICODE
  bool PropertySetGet(Port *port, TCHAR *Buffer, size_t size);
#endif

public:
  AltairProDevice(Port *_port):port(_port){}

public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);
  virtual bool Declare(const struct Declaration &declaration,
                       OperationEnvironment &env);
};

static bool
ReadAltitude(NMEAInputLine &line, fixed &value_r)
{
  fixed value;
  bool available = line.read_checked(value);
  char unit = line.read_first_char();
  if (!available)
    return false;

  if (unit == _T('f') || unit == _T('F'))
    value = Units::ToSysUnit(value, unFeet);

  value_r = value;
  return true;
}

bool
AltairProDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  // no propriatary sentence

  if (strcmp(type, "$PGRMZ") == 0) {
    fixed value;
    if (ReadAltitude(line, value))
      info.ProvidePressureAltitude(value);

    return true;
  }

  return false;
}

bool
AltairProDevice::Declare(const struct Declaration &declaration,
                         OperationEnvironment &env)
{
  port->SetRxTimeout(500); // set RX timeout to 500[ms]

  bool result = DeclareInternal(declaration);

  return result;
}

bool
AltairProDevice::DeclareInternal(const struct Declaration &declaration)
{
  TCHAR Buffer[256];

  _stprintf(Buffer, _T("PDVSC,S,Pilot,%s"), declaration.pilot_name.c_str());
  if (!PropertySetGet(port, Buffer, ARRAY_SIZE(Buffer)))
    return false;

  _stprintf(Buffer, _T("PDVSC,S,GliderID,%s"), declaration.aircraft_registration.c_str());
  if (!PropertySetGet(port, Buffer, ARRAY_SIZE(Buffer)))
    return false;

  _stprintf(Buffer, _T("PDVSC,S,GliderType,%s"), declaration.aircraft_type.c_str());
  if (!PropertySetGet(port, Buffer, ARRAY_SIZE(Buffer)))
    return false;

  /* TODO currently not supported by XCSOAR
   * Pilot2
   * CompetitionID
   * CompetitionClass
   * ObserverID
   * DeclDescription
   * DeclFlightDate
   */

  if (declaration.Size() > 1) {
    PutTurnPoint(_T("DeclTakeoff"), NULL);
    PutTurnPoint(_T("DeclLanding"), NULL);

    PutTurnPoint(_T("DeclStart"), &declaration.GetFirstWaypoint());
    PutTurnPoint(_T("DeclFinish"), &declaration.GetLastWaypoint());

    for (unsigned int index=1; index <= 10; index++){
      TCHAR TurnPointPropertyName[32];
      _stprintf(TurnPointPropertyName, _T("DeclTurnPoint%d"), index);

      if (index < declaration.Size() - 1) {
        PutTurnPoint(TurnPointPropertyName, &declaration.GetWaypoint(index));
      } else {
        PutTurnPoint(TurnPointPropertyName, NULL);
      }
    }
  }

  _stprintf(Buffer, _T("PDVSC,S,DeclAction,DECLARE"));
  if (!PropertySetGet(port, Buffer, ARRAY_SIZE(Buffer)))
    return false;

  if (_tcscmp(&Buffer[9], _T("LOCKED")) == 0)
    // FAILED! try to declare a task on a airborn recorder
    return false;

  // Buffer holds the declaration ticket.
  // but no one is intresting in that
  // eg "2010-11-21 13:01:43 (1)"

  return true;
}



bool
AltairProDevice::PropertySetGet(Port *port, char *Buffer, size_t size)
{
  assert(port != NULL);
  assert(Buffer != NULL);

  // eg $PDVSC,S,FOO,BAR*<cr>\r\n
  PortWriteNMEA(port, Buffer);

  Buffer[6] = _T('A');
  char *comma = strchr(&Buffer[8], ',');

  if (comma != NULL){
    comma[1] = '\0';

    // expect eg $PDVSC,A,FOO,
    if (port->ExpectString(Buffer)){

      // read value eg bar
      while(--size){
        int ch = port->GetChar();
        if (ch == -1)
          break;

        if (ch == '*'){
          Buffer = '\0';
          return true;
        }

        *Buffer++ = (char) ch;

      }

    }
  }

  *Buffer = '\0';
  return false;
}

#ifdef _UNICODE
bool
AltairProDevice::PropertySetGet(Port *port, TCHAR *s, size_t size)
{
  assert(port != NULL);
  assert(s != NULL);

  char buffer[_tcslen(s) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, s, -1, buffer, sizeof(buffer),
                               NULL, NULL) <= 0)
    return false;

  if (!PropertySetGet(port, buffer, _tcslen(s) * 4 + 1))
    return false;

  if (::MultiByteToWideChar(CP_ACP, 0, buffer, -1, s, size) <= 0)
    return false;

  return true;

}
#endif

void
AltairProDevice::PutTurnPoint(const TCHAR *propertyName, const Waypoint *waypoint)
{

  TCHAR Name[DECELWPNAMESIZE];
  TCHAR Buffer[DECELWPSIZE*2];

  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  if (waypoint != NULL){

    CopyString(Name, waypoint->name.c_str(), ARRAY_SIZE(Name));

    tmp = (double)waypoint->location.latitude.Degrees();

    if(tmp < 0){
      NoS = 'S';
      tmp *= -1;
    } else NoS = 'N';

    DegLat = (int)tmp;
    MinLat = tmp - DegLat;
    MinLat *= 60;
    MinLat *= 1000;

    tmp = (double)waypoint->location.longitude.Degrees();

    if (tmp < 0){
      EoW = 'W';
      tmp *= -1;
    } else EoW = 'E';

    DegLon = (int)tmp;
    MinLon = tmp  - DegLon;
    MinLon *= 60;
    MinLon *= 1000;

  } else {

    Name[0] = '\0';
    DegLat = 0;
    MinLat = 0;
    DegLon = 0;
    MinLon = 0;
    NoS = 'N';
    EoW = 'E';
  }

  _stprintf(Buffer, _T("PDVSC,S,%s,%02d%05.0f%c%03d%05.0f%c%s"),
            propertyName,
            DegLat, MinLat, NoS, DegLon, MinLon, EoW, Name
  );

  PropertySetGet(port, Buffer, ARRAY_SIZE(Buffer));

}

static Device *
AltairProCreateOnPort(const DeviceConfig &config, Port *com_port)
{
  return new AltairProDevice(com_port);
}

const struct DeviceRegister atrDevice = {
  _T("Altair RU"),
  _T("Altair Recording Unit"),
  DeviceRegister::DECLARE,
  AltairProCreateOnPort,
};
