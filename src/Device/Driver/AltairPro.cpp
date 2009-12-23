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

#include "Device/Driver/AltairPro.hpp"
#include "Device/Internal.hpp"
#include "Math/Units.h"
#include "NMEA/Info.h"

#include <tchar.h>
#include <stdlib.h>

class AltairProDevice : public AbstractDevice {
private:
  double lastAlt;
  bool last_enable_baro;

public:
  AltairProDevice():lastAlt(0), last_enable_baro(false) {}

public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool PutQNH(const AtmosphericPressure& pres);
  virtual bool Declare(const struct Declaration *declaration);
  virtual void OnSysTicker();
};

bool
AltairProDevice::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO,
                           bool enable_baro)
{

  // no propriatary sentence

  if (_tcsncmp(_T("$PGRMZ"), String, 6) == 0){
    // eat $PGRMZ

    String = _tcschr(String + 6, ',');
    if (String == NULL)
      return false;

    ++String;

    // get <alt>

    lastAlt = _tcstod(String, NULL);

    String = _tcschr(String, ',');
    if (String == NULL)
      return false;

    ++String;

    // get <unit>

    if (*String == 'f' || *String== 'F')
      lastAlt /= TOFEET;

    if (enable_baro) {
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = GPS_INFO->pressure.AltitudeToQNHAltitude(fixed(lastAlt));
    }

    last_enable_baro = enable_baro;

    return true;
  }

  return false;

}

bool
AltairProDevice::Declare(const struct Declaration *decl)
{
  (void) decl;

  // TODO feature: Altair declaration

  return true;
}

#include "DeviceBlackboard.hpp"

bool
AltairProDevice::PutQNH(const AtmosphericPressure &pres)
{
  // TODO code: JMW check sending QNH to Altair
  if (last_enable_baro)
    device_blackboard.SetBaroAlt(pres.AltitudeToQNHAltitude(fixed(lastAlt)));

  return true;
}

void
AltairProDevice::OnSysTicker()
{
  // Do To get IO data like temp, humid, etc
}

static Device *
AltairProCreateOnComPort(ComPort *com_port)
{
  return new AltairProDevice();
}

const struct DeviceRegister atrDevice = {
  _T("Altair Pro"),
  drfGPS | drfBaroAlt, // drfLogger - ToDo
  AltairProCreateOnComPort,
};
