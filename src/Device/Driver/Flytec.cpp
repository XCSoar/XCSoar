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

#include "Device/Driver/Flytec.hpp"
#include "Device/Parser.hpp"
#include "Device/Internal.hpp"
#include "NMEA/Info.hpp"
#include "Protection.hpp"
#include "Units.hpp"

#include <tchar.h>
#include <stdlib.h>
#include <math.h>

class FlytecDevice : public AbstractDevice {
private:
  ComPort *port;

public:
  FlytecDevice(ComPort *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
};

#ifdef FIXED_MATH
static bool
ParseNumber(const TCHAR *line, unsigned column, fixed &value_r)
{
  TCHAR buffer[80];
  NMEAParser::ExtractParameter(line, buffer, column);

  TCHAR *endptr;
  value_r = _tcstod(buffer, &endptr);
  return endptr > buffer && *endptr == '\0';
}
#endif

static bool
ParseNumber(const TCHAR *line, unsigned column, double &value_r)
{
  TCHAR buffer[80];
  NMEAParser::ExtractParameter(line, buffer, column);

  TCHAR *endptr;
  value_r = _tcstod(buffer, &endptr);
  return endptr > buffer && *endptr == '\0';
}

/**
 * Parse a "$BRSF" sentence.
 *
 * Example: "$BRSF,063,-013,-0035,1,193,00351,535,485*38"
 */
static bool
FlytecParseBRSF(const TCHAR *line, NMEA_INFO &info, bool enable_baro)
{
  // 0 = indicated or true airspeed [km/h]
  // XXX is that TAS or IAS?  Documentation isn't clear.
  info.AirspeedAvailable = ParseNumber(line, 0, info.TrueAirspeed);
  if (info.AirspeedAvailable) {
    info.TrueAirspeed = Units::ToSysUnit(info.TrueAirspeed, unKiloMeterPerHour);
    info.IndicatedAirspeed = info.TrueAirspeed;
  }

  // 1 = integrated vario [dm/s]
  // 2 = altitude A2 [m] (XXX what's this?)
  // 3 = waypoint
  // 4 = bearing to waypoint [degrees]
  // 5 = distance to waypoint [100m]
  // 6 = MacCready speed to fly [100m/h]
  // 7 = speed to fly, best glide [100m/h]

  return true;
}

/**
 * Parse a "$VMVABD" sentence.
 *
 * Example: "$VMVABD,0000.0,M,0547.0,M,-0.0,,,MS,0.0,KH,22.4,C*65"
 */
static bool
FlytecParseVMVABD(const TCHAR *line, NMEA_INFO &info, bool enable_baro)
{
  TCHAR unit[20];

  // 0,1 = GPS altitude, unit
  NMEAParser::ExtractParameter(line, unit, 1);
  if (_tcscmp(unit, _T("M")) == 0) {
    // XXX is that TAS or IAS?  Documentation isn't clear.
    ParseNumber(line, 0, info.GPSAltitude);
  }

  // 2,3 = baro altitude, unit
  if (enable_baro) {
    NMEAParser::ExtractParameter(line, unit, 3);
    if (_tcscmp(unit, _T("M")) == 0) {
      // XXX is that TAS or IAS?  Documentation isn't clear.
      info.BaroAltitudeAvailable = ParseNumber(line, 2, info.BaroAltitude);
    }
  }

  // 4-7 = integrated vario, unit

  // 8,9 = indicated or true airspeed, unit
  NMEAParser::ExtractParameter(line, unit, 9);
  if (_tcscmp(unit, _T("KH")) == 0) {
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.AirspeedAvailable = ParseNumber(line, 8, info.TrueAirspeed);
    if (info.AirspeedAvailable) {
      info.TrueAirspeed = Units::ToSysUnit(info.TrueAirspeed,
                                           unKiloMeterPerHour);
      info.IndicatedAirspeed = info.TrueAirspeed;
    }
  }

  // 10,11 = temperature, unit
  NMEAParser::ExtractParameter(line, unit, 11);
  if (_tcscmp(unit, _T("C")) == 0)
    info.TemperatureAvailable = ParseNumber(line, 10,
                                            info.OutsideAirTemperature);

  return true;
}

bool
FlytecDevice::ParseNMEA(const TCHAR *line, NMEA_INFO *info, bool enable_baro)
{
  if (_tcsncmp(_T("$BRSF,"), line, 6) == 0)
    return FlytecParseBRSF(line + 6, *info, enable_baro);

  if (_tcsncmp(_T("$VMVABD,"), line, 8) == 0)
    return FlytecParseVMVABD(line + 8, *info, enable_baro);

  return false;
}

static Device *
FlytecCreateOnComPort(ComPort *com_port)
{
  return new FlytecDevice(com_port);
}

const struct DeviceRegister flytec_device_driver = {
  _T("Flytec"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario,
  FlytecCreateOnComPort,
};
