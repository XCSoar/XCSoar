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

#include "Device/Driver/Leonardo.hpp"
#include "Device/Parser.hpp"
#include "Device/Internal.hpp"
#include "NMEA/Info.hpp"
#include "Protection.hpp"

#include <tchar.h>
#include <stdlib.h>
#include <math.h>

class LeonardoDevice : public AbstractDevice {
private:
  ComPort *port;

public:
  LeonardoDevice(ComPort *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
};

static bool
ColumnIsEmpty(const TCHAR *line, unsigned column)
{
  TCHAR buffer[80];
  NMEAParser::ExtractParameter(line, buffer, column);
  return buffer[0] == _T('\0');
}

#ifdef FIXED_MATH
static bool
ParseNumber(const TCHAR *line, unsigned column, fixed &value_r)
{
  TCHAR buffer[80];
  NMEAParser::ExtractParameter(line, buffer, column);

  TCHAR *endptr;
  value_r = _tcstol(buffer, &endptr, 10);
  return endptr > buffer && *endptr == '\0';
}
#endif

static bool
ParseNumber(const TCHAR *line, unsigned column, double &value_r)
{
  TCHAR buffer[80];
  NMEAParser::ExtractParameter(line, buffer, column);

  TCHAR *endptr;
  value_r = _tcstol(buffer, &endptr, 10);
  return endptr > buffer && *endptr == '\0';
}

/**
 * Parse a "$C" sentence.
 *
 * Example: "$C,+2025,-7,+18,+25,+29,122,314,314,0,-356,+25,45,T*3D"
 */
static bool
LeonardoParseC(const TCHAR *line, NMEA_INFO &info, bool enable_baro)
{
  // 0 = altitude [m]
  if (enable_baro)
    info.BaroAltitudeAvailable = ParseNumber(line, 0, info.BaroAltitude);

  // 1 = vario [dm/s]
  info.TotalEnergyVarioAvailable = ParseNumber(line, 1, info.TotalEnergyVario);
  if (info.TotalEnergyVarioAvailable)
    info.TotalEnergyVario /= 10;

  // 2 = airspeed [km/h]
  /* XXX is that TAS or IAS? */
  info.AirspeedAvailable = ParseNumber(line, 2, info.TrueAirspeed);
  if (info.AirspeedAvailable) {
    info.TrueAirspeed /= 3.6;
    info.IndicatedAirspeed = info.TrueAirspeed; // XXX convert properly
  }

  if (ColumnIsEmpty(line, 3))
    /* short "$C" sentence ends after airspeed */
    return true;

  // 3 = netto vario [m/s]
  info.NettoVarioAvailable = ParseNumber(line, 3, info.NettoVario);
  if (info.NettoVarioAvailable)
    info.NettoVario /= 10;

  // 4 = temperature [deg C]
  info.TemperatureAvailable = ParseNumber(line, 4, info.OutsideAirTemperature);

  // 10 = wind speed [km/h]
  // 11 = wind direction [degrees]
  info.ExternalWindAvailable = ParseNumber(line, 10, info.wind.norm)
    && ParseNumber(line, 11, info.wind.bearing);

  TriggerVarioUpdate();

  return true;
}

/**
 * Parse a "$D" sentence.
 *
 * Example: "$D,+0,100554,+25,18,+31,,0,-356,+25,+11,115,96*6A"
 */
static bool
LeonardoParseD(const TCHAR *line, NMEA_INFO &info, bool enable_baro)
{
  // 0 = vario [dm/s]
  info.TotalEnergyVarioAvailable = ParseNumber(line, 0, info.TotalEnergyVario);
  if (info.TotalEnergyVarioAvailable)
    info.TotalEnergyVario /= 10;

  if (ColumnIsEmpty(line, 1))
    /* short "$D" sentence ends after airspeed */
    return true;

  // 1 = air pressure [Pa]

  // 2 = netto vario [dm/s]
  info.NettoVarioAvailable = ParseNumber(line, 2, info.NettoVario);
  if (info.NettoVarioAvailable)
    info.NettoVario /= 10;

  // 3 = airspeed [km/h]
  /* XXX is that TAS or IAS? */
  info.AirspeedAvailable = ParseNumber(line, 3, info.TrueAirspeed) / 3.6;
  if (info.AirspeedAvailable) {
    info.TrueAirspeed /= 3.6;
    info.IndicatedAirspeed = info.TrueAirspeed; // XXX convert properly
  }

  // 4 = temperature [deg C]
  info.TemperatureAvailable = ParseNumber(line, 4, info.OutsideAirTemperature);

  // 5 = compass [degrees]
  /* XXX unsupported by XCSoar */

  // 6 = optimal speed [km/h]
  /* XXX unsupported by XCSoar */

  // 7 = equivalent MacCready [cm/s]
  /* XXX unsupported by XCSoar */

  // 8 = wind speed [km/h]
  /* not used here, the "$C" record repeats it together with the
     direction */

  TriggerVarioUpdate();

  return true;
}

bool
LeonardoDevice::ParseNMEA(const TCHAR *line, NMEA_INFO *info, bool enable_baro)
{
  if (_tcsncmp(_T("$C,"), line, 3) == 0 || _tcsncmp(_T("$c,"), line, 3) == 0)
    return LeonardoParseC(line + 3, *info, enable_baro);

  if (_tcsncmp(_T("$D,"), line, 3) == 0 || _tcsncmp(_T("$d,"), line, 3) == 0)
    return LeonardoParseD(line + 3, *info, enable_baro);

  return false;
}

static Device *
LeonardoCreateOnComPort(ComPort *com_port)
{
  return new LeonardoDevice(com_port);
}

const struct DeviceRegister leonardo_device_driver = {
  _T("Leonardo"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario,
  LeonardoCreateOnComPort,
};
