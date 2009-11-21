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

#ifndef XCSOAR_DEVICE_DRIVER_HPP
#define XCSOAR_DEVICE_DRIVER_HPP

#include "Device/Declaration.hpp"

#include <tchar.h>

struct NMEA_INFO;

typedef	enum {dfGPS, dfLogger, dfSpeed,	dfVario, dfBaroAlt,	dfWind, dfVoice, dfNmeaOut, dfRadio, dfCondor } DeviceFlags_t;

#define drfGPS		(1l << dfGPS)
#define drfLogger	(1l << dfLogger)
#define drfSpeed	(1l << dfSpeed)
#define drfVario	(1l << dfVario)
#define drfBaroAlt	(1l << dfBaroAlt)
#define drfWind		(1l << dfWind)
#define drfVoice	(1l << dfVoice)
#define drfNmeaOut	(1l << dfNmeaOut)
#define drfRadio	(1l << dfRadio)
#define drfCondor	(1l << dfCondor)

/**
 * This is the structure exported by a device driver.
 */
struct DeviceRegister {
  const TCHAR *Name;
  unsigned int Flags;
  bool (*ParseNMEA)(struct DeviceDescriptor *d, const TCHAR *String,
                    struct NMEA_INFO *info, bool enable_baro);
  bool (*PutMacCready)(struct DeviceDescriptor *d, double McReady);
  bool (*PutBugs)(struct DeviceDescriptor *d, double Bugs);
  bool (*PutBallast)(struct DeviceDescriptor *d, double Ballast);
  bool (*PutQNH)(struct DeviceDescriptor *d, double NewQNH);
  bool (*PutVoice)(struct DeviceDescriptor *d, const TCHAR *Sentence);
  bool (*PutVolume)(struct DeviceDescriptor *d, int Volume);
  bool (*PutFreqActive)(struct DeviceDescriptor *d, double Freq);
  bool (*PutFreqStandby)(struct DeviceDescriptor *d, double Standby);
  bool (*Open)(struct DeviceDescriptor *d, int Port);
  bool (*Close)(struct DeviceDescriptor *d);
  bool (*LinkTimeout)(struct DeviceDescriptor *d);
  bool (*Declare)(struct DeviceDescriptor *d, const struct Declaration *decl);
  bool (*IsLogger)(const struct DeviceDescriptor *d);
  bool (*IsGPSSource)(const struct DeviceDescriptor *d);
  bool (*IsBaroSource)(const struct DeviceDescriptor *d);
  bool (*OnSysTicker)(struct DeviceDescriptor *d);
};

#endif
