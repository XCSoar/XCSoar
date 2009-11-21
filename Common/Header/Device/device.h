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

#ifndef	DEVICE_H
#define	DEVICE_H

#include "Sizes.h"
#include "Port.h"
#include "WayPoint.hpp"

#include <stdio.h>

struct NMEA_INFO;

#define DEVNAMESIZE  32
#define	NUMDEV		 2

#define	devA()	    (&DeviceList[0])
#define	devB()	    (&DeviceList[1])

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

struct Declaration {
  TCHAR PilotName[64];
  TCHAR AircraftType[32];
  TCHAR AircraftRego[32];
  int num_waypoints;
  const WAYPOINT *waypoint[MAXTASKPOINTS];
};

struct DeviceRegister;

struct DeviceDescriptor {
  int	Port;
  FILE  *fhLogFile;
  ComPort *Com;
  TCHAR	Name[DEVNAMESIZE+1];
  struct DeviceDescriptor *pDevPipeTo;
  const struct DeviceRegister *Driver;
  bool ticker;

  /* Warning: the following methods do not lock mutexComm */
  bool Open(int Port);
  void Close();

  bool IsLogger() const;
  bool IsGPSSource() const;
  bool IsBaroSource() const;

  bool PutMcCready(double mc_cready);
  bool PutBugs(double bugs);
  bool PutBallast(double ballast);
  bool PutVolume(int volume);
  bool PutActiveFrequency(double frequency);
  bool PutStandbyFrequency(double frequency);
  bool PutQNH(double qnh);
  bool PutVoice(const TCHAR *sentence);

  void LinkTimeout();
  bool Declare(const struct Declaration *declaration);

  void OnSysTicker();
};

#define Port1WriteNMEA(s)	devWriteNMEAString(devA(), s)
#define Port2WriteNMEA(s)	devWriteNMEAString(devB(), s)

void devWriteNMEAString(struct DeviceDescriptor *d, const TCHAR *Text);
void VarioWriteNMEA(const TCHAR *Text);
struct DeviceDescriptor *devVarioFindVega(void);

struct DeviceRegister {
  const TCHAR	*Name;
  unsigned int	Flags;
  BOOL (*ParseNMEA)(struct DeviceDescriptor *d, const TCHAR *String,
                    NMEA_INFO *GPS_INFO);
  BOOL (*PutMacCready)(struct DeviceDescriptor *d, double McReady);
  BOOL (*PutBugs)(struct DeviceDescriptor *d, double Bugs);
  BOOL (*PutBallast)(struct DeviceDescriptor *d, double Ballast);
  BOOL (*PutQNH)(struct DeviceDescriptor *d, double NewQNH);
  BOOL (*PutVoice)(struct DeviceDescriptor *d, const TCHAR *Sentence);
  BOOL (*PutVolume)(struct DeviceDescriptor *d, int Volume);
  BOOL (*PutFreqActive)(struct DeviceDescriptor *d, double Freq);
  BOOL (*PutFreqStandby)(struct DeviceDescriptor *d, double Standby);
  BOOL (*Open)(struct DeviceDescriptor *d, int Port);
  BOOL (*Close)(struct DeviceDescriptor *d);
  BOOL (*LinkTimeout)(struct DeviceDescriptor *d);
  BOOL (*Declare)(struct DeviceDescriptor *d, const struct Declaration *decl);
  BOOL (*IsLogger)(const struct DeviceDescriptor *d);
  BOOL (*IsGPSSource)(const struct DeviceDescriptor *d);
  BOOL (*IsBaroSource)(const struct DeviceDescriptor *d);
  BOOL (*OnSysTicker)(struct DeviceDescriptor *d);
};



extern struct DeviceDescriptor DeviceList[NUMDEV];

/**
 * NULL terminated array of available device drivers.
 */
extern const struct DeviceRegister *const DeviceRegister[];

extern struct DeviceDescriptor *pDevPrimaryBaroSource;
extern struct DeviceDescriptor *pDevSecondaryBaroSource;

BOOL devRegisterGetName(int Index, TCHAR *Name);

BOOL ExpectString(struct DeviceDescriptor *d, const TCHAR *token);
BOOL devHasBaroSource(void);

BOOL devParseNMEA(struct DeviceDescriptor *d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO);

bool
devDeclare(struct DeviceDescriptor *d, const struct Declaration *decl);

BOOL devIsLogger(const struct DeviceDescriptor *d);
BOOL devIsGPSSource(const struct DeviceDescriptor *d);
BOOL devIsBaroSource(const struct DeviceDescriptor *d);
BOOL devIsRadio(const struct DeviceDescriptor *d);
BOOL devIsCondor(const struct DeviceDescriptor *d);

void devTick(void);

BOOL devGetBaroAltitude(double *Value);

void AllDevicesPutMcCready(double mc_cready);
void AllDevicesPutBugs(double bugs);
void AllDevicesPutBallast(double ballast);
void AllDevicesPutVolume(int volume);
void AllDevicesPutActiveFrequency(double frequency);
void AllDevicesPutStandbyFrequency(double frequency);
void AllDevicesPutQNH(double qnh);
void AllDevicesPutVoice(const TCHAR *sentence);

void AllDevicesLinkTimeout();

BOOL devIsFalseReturn(const struct DeviceDescriptor *d);
BOOL devIsTrueReturn(const struct DeviceDescriptor *d);

void devStartup(LPTSTR lpCmdLine);
void devShutdown();
void devRestart(void);
void devConnectionMonitor();

#endif
