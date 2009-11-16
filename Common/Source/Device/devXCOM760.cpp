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

#include "Device/devXCOM760.h"
#include "Device/Internal.hpp"
#include "Device/Port.h"

#include <tchar.h>
#include <stdio.h>

class XCOM760Device : public AbstractDevice {
private:
  ComPort *port;

public:
  XCOM760Device(ComPort *_port):port(_port) {}

public:
  virtual bool PutVolume(int volume);
  virtual bool PutActiveFrequency(double frequency);
  virtual bool PutStandbyFrequency(double frequency);
};

bool
XCOM760Device::PutVolume(int Volume)
{
  TCHAR  szTmp[32];
  _stprintf(szTmp, _T("$RVOL=%d\r\n"), Volume);
  port->WriteString(szTmp);
  return true;
}

bool
XCOM760Device::PutActiveFrequency(double Freq)
{
  TCHAR  szTmp[32];
  _stprintf(szTmp, _T("$TXAF=%.3f\r\n"), Freq);
  port->WriteString(szTmp);
  return true;
}

bool
XCOM760Device::PutStandbyFrequency(double Freq)
{
  TCHAR  szTmp[32];
  _stprintf(szTmp, _T("$TXSF=%.3f\r\n"), Freq);
  port->WriteString(szTmp);
  return true;
}

static Device *
XCOM760CreateOnComPort(ComPort *com_port)
{
  return new XCOM760Device(com_port);
}

const struct DeviceRegister xcom760Device = {
  _T("XCOM760"),
  drfRadio,
  XCOM760CreateOnComPort,
};

/* Commands

  $TOGG: return to main screen or toggle active and standby
  $DUAL=on/off
*/
