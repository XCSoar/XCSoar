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
struct ComPort;

/**
 * This is the interface for a device driver.
 */
class Device {
public:
  virtual ~Device();

  virtual bool Open() = 0;

  virtual void LinkTimeout() = 0;

  virtual bool IsLogger() = 0;
  virtual bool IsGPSSource() = 0;
  virtual bool IsBaroSource() = 0;

  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro) = 0;

  virtual bool PutMcCready(double mc_cready) = 0;
  virtual bool PutBugs(double bugs) = 0;
  virtual bool PutBallast(double ballast) = 0;
  virtual bool PutQNH(double qnh) = 0;
  virtual bool PutVoice(const TCHAR *sentence) = 0;
  virtual bool PutVolume(int volume) = 0;
  virtual bool PutActiveFrequency(double frequency) = 0;
  virtual bool PutStandbyFrequency(double frequency) = 0;

  virtual bool Declare(const struct Declaration *declaration) = 0;

  virtual void OnSysTicker() = 0;
};

/**
 * This class implements all #Device methods.  You may use it as a
 * base class for specific device drivers.
 */
class AbstractDevice : public Device {
  virtual bool Open();

  virtual void LinkTimeout();

  virtual bool IsLogger();
  virtual bool IsGPSSource();
  virtual bool IsBaroSource();

  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);

  virtual bool PutMcCready(double mc_cready);
  virtual bool PutBugs(double bugs);
  virtual bool PutBallast(double ballast);
  virtual bool PutQNH(double qnh);
  virtual bool PutVoice(const TCHAR *sentence);
  virtual bool PutVolume(int volume);
  virtual bool PutActiveFrequency(double frequency);
  virtual bool PutStandbyFrequency(double frequency);

  virtual bool Declare(const struct Declaration *declaration);

  virtual void OnSysTicker();
};

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
  Device *(*CreateOnComPort)(ComPort *com_port);
};

#endif
