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

#if !defined(XCSOAR_INTERFACE_HPP)
#define XCSOAR_INTERFACE_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "InterfaceBlackboard.hpp"

class MainWindow;
class StatusMessageList;

class CommonInterface {
public:
  // instance of main program
  static HINSTANCE hInst;
// window.. make this protected TODO so have to subclass to get access
  static StatusMessageList status_messages;
  static MainWindow main_window;

  // TODO: make this protected
  static const NMEA_INFO& Basic() { return blackboard.Basic(); }
  static const DERIVED_INFO& Calculated() { return blackboard.Calculated(); }
  static const MapWindowProjection& MapProjection()
  { return blackboard.MapProjection(); }

  static const SETTINGS_COMPUTER& SettingsComputer()
  { return blackboard.SettingsComputer(); }
  static SETTINGS_COMPUTER& SetSettingsComputer()
  { return blackboard.SetSettingsComputer(); }

  static const SETTINGS_MAP& SettingsMap()
  { return blackboard.SettingsMap(); }
  static SETTINGS_MAP& SetSettingsMap()
  { return blackboard.SetSettingsMap(); }

  static void ReadMapProjection(const MapWindowProjection &map) {
    blackboard.ReadMapProjection(map);
  }
  static void ReadBlackboardBasic(const NMEA_INFO& nmea_info) {
    blackboard.ReadBlackboardBasic(nmea_info);
  }
  static void ReadBlackboardCalculated(const DERIVED_INFO& derived_info) {
    blackboard.ReadBlackboardCalculated(derived_info);
  }
private:
  static InterfaceBlackboard blackboard;
public:
  // settings
  static bool VirtualKeys;
  static bool EnableAutoBacklight; 
  static bool EnableAutoSoundVolume; 
  static int  ActiveAlternate;
  static bool EnableAnimation;
};


class ActionInterface: public CommonInterface {
public:
  // settings
  static int  MenuTimeoutMax;
protected:
  static void DisplayModes();
  static void SendSettingsComputer();
  static void SendSettingsMap(const bool trigger_draw=false);
public: 
  // ideally these should be protected
  static void on_key_None(int UpDown);
  static void on_key_WindSpeed(int UpDown);
  static void on_key_WindDirection(int UpDown);
  static void on_key_MacCready(int UpDown);
  static void on_key_Accelerometer(int UpDown);
  static void on_key_Waypoint(int UpDown);
  static void on_key_Speed(int UpDown);
  static void on_key_Direction(int UpDown);
  static void on_key_Altitude(int UpDown);
  static void on_key_QFEAltitude(int UpDown); // VENTA3
  static void on_key_Alternate1(int UpDown); // VENTA3
  static void on_key_Alternate2(int UpDown); // VENTA3
  static void on_key_BestAlternate(int UpDown); // VENTA3
  static void on_key_Airspeed(int UpDown);
  static void on_key_TeamCode(int UpDown);
  static void on_key_ForecastTemperature(int UpDown);
  static void StartHourglassCursor();
  static void StopHourglassCursor();
  static void SignalShutdown(bool force);
  static bool RequestAirspaceWarningForce;
  static bool LockSettingsInFlight;
  static unsigned UserLevel;
private:
  static HCURSOR oldCursor;
};


class XCSoarInterface: public ActionInterface {
public:
  // settings
  static unsigned debounceTimeout;
public:
  static bool Debounce();

  static bool InterfaceTimeoutZero(void);
  static void InterfaceTimeoutReset(void);
  static bool InterfaceTimeoutCheck(void);
  static bool CheckShutdown(void);

  static void AfterStartup();
  static void Shutdown();
  static bool Startup (HINSTANCE, LPTSTR lpCmdLine);

  static HWND CreateProgressDialog(const TCHAR *text);
  static void CloseProgressDialog();
  static void StepProgressDialog();
  static BOOL SetProgressStepSize(int nSize);

  static void ExchangeBlackboard();
  static void ReceiveMapProjection();
  static void ReceiveBlackboard();
private:
  static void PreloadInitialisation(bool ask);
  static void StartupInfo();
  static HWND hProgress;
  static HWND hWndCurtain;
};

#endif
