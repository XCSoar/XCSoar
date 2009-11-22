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

#include "Device/devFlymasterF1.h"
#include "Device/Internal.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "NMEA/Info.h"

#include <tchar.h>
#include <stdlib.h>
#include <math.h>

class FlymasterF1Device : public AbstractDevice {
public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
};

static bool
VARIO(const TCHAR *String, NMEA_INFO *GPS_INFO);

bool
FlymasterF1Device::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO,
                             bool enable_baro)
{
  if(_tcsncmp(_T("$VARIO"), String, 6)==0)
    {
      return VARIO(&String[7], GPS_INFO);
    }

  return false;
}

static Device *
FlymasterF1CreateOnComPort(ComPort *com_port)
{
  return new FlymasterF1Device();
}

const struct DeviceRegister flymasterf1Device = {
  _T("FlymasterF1"),
  drfGPS | drfBaroAlt | drfVario,
  FlymasterF1CreateOnComPort,
};

// *****************************************************************************
// local stuff

static bool
VARIO(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  // $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*CS

  TCHAR ctemp[80];
  NMEAParser::ExtractParameter(String,ctemp,0);
  double ps = _tcstod(ctemp, NULL);
  GPS_INFO->BaroAltitude = (1 - pow(fabs(ps / QNH),  0.190284)) * 44307.69;
  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->Vario = _tcstod(ctemp, NULL) / 10.0;
  // JMW vario is in dm/s

  GPS_INFO->VarioAvailable = true;
  GPS_INFO->BaroAltitudeAvailable = true;

  TriggerVarioUpdate();

  return true;
}
