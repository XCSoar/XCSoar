/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"


class BaseBlackboard 
{
  // all blackboards can be read as const
public:
  const NMEA_INFO& Basic() const { return gps_info; }
  const DERIVED_INFO& Calculated() const { return calculated_info; }
protected:
  NMEA_INFO     gps_info;
  DERIVED_INFO  calculated_info;
};


class SettingsComputerBlackboard 
{
public:
  const SETTINGS_COMPUTER& SettingsComputer() const 
  { return settings_computer; }
protected:
  SETTINGS_COMPUTER settings_computer;
};


class SettingsMapBlackboard 
{
public:
  const SETTINGS_MAP& SettingsMap() const 
  { return settings_map; };
protected:
  SETTINGS_MAP settings_map;
};


class GlideComputerBlackboard: 
  public BaseBlackboard,
  public SettingsComputerBlackboard
{
public:
  void ReadBlackboard(const NMEA_INFO &nmea_info);
  void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);
  const NMEA_INFO& LastBasic() const { return last_gps_info; }
  const DERIVED_INFO& LastCalculated() const { return last_calculated_info; }
protected:
  bool time_advanced() {
    return (Basic().Time-LastBasic().Time>0);
  }
  bool time_retreated() {
    return _time_retreated;
  }
  void ResetFlight(const bool full=true);
  void StartTask();
  void Initialise();
  void SaveFinish();
  void RestoreFinish();

  virtual double GetAverageThermal() const;
  virtual void OnClimbBase(double StartAlt) = 0;
  virtual void OnClimbCeiling() = 0;
  virtual void OnDepartedThermal() = 0;

  // only the glide computer can write to calculated
  DERIVED_INFO& SetCalculated() { return calculated_info; }
private:
  DERIVED_INFO Finish_Derived_Info;
  NMEA_INFO     last_gps_info;
  DERIVED_INFO last_calculated_info;
  bool _time_retreated;
};


// the deviceblackboard is used as the global ground truth-state
// since it is accessed quickly with only one mutex (flight)
class DeviceBlackboard: 
  public BaseBlackboard,
  public SettingsComputerBlackboard,
  public SettingsMapBlackboard
{
public:
  void Initialise();
  void ReadBlackboard(const DERIVED_INFO &derived_info);
  void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);
  void ReadSettingsMap(const SETTINGS_MAP &settings);

  // only the device blackboard can write to gps
  friend class ComPort;
protected:
  NMEA_INFO& SetBasic() { return gps_info; }
public:
  void SetStartupLocation(double lon, double lat, double alt);
  // used by replay logger
  void SetLocation(double lon, double lat, double speed, double bearing,
		   double alt, double baroalt, double t);
  void ProcessSimulation();
  bool LowerConnection(); // decrement
  void RaiseConnection(); // set to 2
  void StopReplay();
  void FLARM_RefreshSlots();
  void SetBaroAlt(double x) {
    SetBasic().BaroAltitude = x;
  }
  void SetNAVWarning(bool val);
  void SetTrackBearing(double val);
  void SetSpeed(double val);
  void SetAltitude(double alt);
};


class MapWindowBlackboard: 
  public BaseBlackboard,
  public SettingsComputerBlackboard,
  public SettingsMapBlackboard
{
protected:
  virtual void ReadBlackboard(const NMEA_INFO &nmea_info,
			      const DERIVED_INFO &derived_info);
  virtual void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);
  virtual void ReadSettingsMap(const SETTINGS_MAP &settings);
};


class InterfaceBlackboard: 
  public BaseBlackboard,
  public SettingsComputerBlackboard,
  public SettingsMapBlackboard
{
public:
  void ReadBlackboardBasic(const NMEA_INFO &nmea_info);
  void ReadBlackboardCalculated(const DERIVED_INFO &derived_info);
  SETTINGS_COMPUTER& SetSettingsComputer() { return settings_computer; }
  SETTINGS_MAP& SetSettingsMap() { return settings_map; }
  void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);
};


class InstrumentBlackboard {
protected:
  static const NMEA_INFO& Basic() { return blackboard.Basic(); }
  static const DERIVED_INFO& Calculated() { return blackboard.Calculated(); }
  const SETTINGS_COMPUTER& SettingsComputer()  
  { return blackboard.SettingsComputer(); }
public:
  static void ReadBlackboardBasic(const NMEA_INFO& nmea_info) {
    blackboard.ReadBlackboardBasic(nmea_info);
  }
  static void ReadBlackboardCalculated(const DERIVED_INFO& derived_info) {
    blackboard.ReadBlackboardCalculated(derived_info);
  }
  static void ReadSettingsComputer(const SETTINGS_COMPUTER &settings) {
    blackboard.ReadSettingsComputer(settings);
  }
private:
  static InterfaceBlackboard blackboard;
};

// master flight data
//extern NMEA_INFO GPS_INFO;
//extern DERIVED_INFO CALCULATED_INFO;

// gps detection
extern BOOL GPSCONNECT;
extern BOOL VARIOCONNECT;

extern BOOL extGPSCONNECT;

extern DeviceBlackboard device_blackboard;

#endif
