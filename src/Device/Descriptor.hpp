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

#ifndef XCSOAR_DEVICE_DESCRIPTOR_HPP
#define XCSOAR_DEVICE_DESCRIPTOR_HPP

#include "Device/Port.hpp"
#include "Device/Parser.hpp"

#include <tchar.h>
#include <stdio.h>

struct NMEA_INFO;
class Port;
class Device;
class AtmosphericPressure;
struct DeviceRegister;

class DeviceDescriptor : public Port::Handler {
public:
  Port *Com;
  DeviceDescriptor *pDevPipeTo;
  const struct DeviceRegister *Driver;

  Device *device;

  bool enable_baro;
  NMEAParser parser;

  bool ticker;

public:
  DeviceDescriptor();

public:
  /* Warning: the following methods do not lock mutexComm */

  void Clear();

  bool Open();
  void Close();

  const TCHAR *GetName() const;

  /**
   * Compares the driver's name.
   */
  bool IsDriver(const TCHAR *name) const;

  bool IsLogger() const;
  bool IsGPSSource() const;
  bool IsBaroSource() const;
  bool IsRadio() const;

  bool IsCondor() const {
    return IsDriver(_T("Condor"));
  }

  bool IsVega() const {
    return IsDriver(_T("Vega"));
  }

private:
  bool ParseNMEA(const char *line, struct NMEA_INFO *info);

public:
  bool PutMacCready(double MacCready);
  bool PutBugs(double bugs);
  bool PutBallast(double ballast);
  bool PutVolume(int volume);
  bool PutActiveFrequency(double frequency);
  bool PutStandbyFrequency(double frequency);
  bool PutQNH(const AtmosphericPressure& pres);
  bool PutVoice(const TCHAR *sentence);

  void LinkTimeout();
  bool Declare(const struct Declaration *declaration);

  void OnSysTicker();

  virtual void LineReceived(const char *line);
};

#endif
