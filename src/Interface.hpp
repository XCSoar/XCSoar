/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef XCSOAR_INTERFACE_HPP
#define XCSOAR_INTERFACE_HPP

#include "Blackboard/InterfaceBlackboard.hpp"
#include "Thread/Debug.hpp"
#include "Compiler.h"

struct UIState;
class MainWindow;
class StatusMessageList;

/** 
 * Class to hold data/methods accessible by all interface subsystems
 */
class CommonInterface {
  static UIState ui_state;
  static InterfaceBlackboard blackboard;

protected:
  /**
   * True if movement was detected on a real GPS.
   */
  static bool movement_detected;

public:
  // window.. make this protected TODO so have to subclass to get access
  static StatusMessageList status_messages;
  static MainWindow *main_window;

  static bool MovementDetected() {
    return movement_detected;
  }

  // TODO: make this protected
  /**
   * Returns InterfaceBlackboard.Basic (NMEA_INFO) (read-only)
   * @return InterfaceBlackboard.Basic
   */
  gcc_const
  static const MoreData &Basic() {
    assert(InMainThread());

    return blackboard.Basic();
  }

  /**
   * Returns InterfaceBlackboard.Calculated (DERIVED_INFO) (read-only)
   * @return InterfaceBlackboard.Calculated
   */
  gcc_const
  static const DerivedInfo& Calculated() {
    assert(InMainThread());

    return blackboard.Calculated();
  }

  gcc_const
  static const SystemSettings& GetSystemSettings() {
    assert(InMainThread());

    return blackboard.GetSystemSettings();
  }

  gcc_const
  static SystemSettings &SetSystemSettings() {
    assert(InMainThread());

    return blackboard.SetSystemSettings();
  }

  /**
   * Returns the InterfaceBlackboard.ComputerSettings (read-only)
   * @return The InterfaceBlackboard.ComputerSettings
   */
  gcc_const
  static const ComputerSettings& GetComputerSettings() {
    assert(InMainThread());

    return blackboard.GetComputerSettings();
  }

  /**
   * Returns the InterfaceBlackboard.ComputerSettings (read-write)
   * @return The InterfaceBlackboard.ComputerSettings
   */
  gcc_const
  static ComputerSettings& SetComputerSettings() {
    assert(InMainThread());

    return blackboard.SetComputerSettings();
  }

  gcc_const
  static const UISettings &GetUISettings() {
    assert(InMainThread());

    return blackboard.GetUISettings();
  }

  /**
   * Returns the InterfaceBlackboard.MapSettings (read-only)
   * @return The InterfaceBlackboard.MapSettings
   */
  gcc_const
  static const MapSettings& GetMapSettings() {
    assert(InMainThread());

    return GetUISettings().map;
  }

  gcc_const
  static const FullBlackboard &Full() {
    assert(InMainThread());

    return blackboard;
  }

  gcc_const
  static LiveBlackboard &GetLiveBlackboard() {
    assert(InMainThread());

    return blackboard;
  }

  gcc_const
  static UISettings &SetUISettings() {
    assert(InMainThread());

    return blackboard.SetUISettings();
  }

  /**
   * Returns the InterfaceBlackboard.MapSettings (read-write)
   * @return The InterfaceBlackboard.MapSettings
   */
  gcc_const
  static MapSettings& SetMapSettings() {
    assert(InMainThread());

    return SetUISettings().map;
  }

  static const UIState &GetUIState() {
    assert(InMainThread());

    return ui_state;
  }

  static UIState &SetUIState() {
    assert(InMainThread());

    return ui_state;
  }

  static void ReadBlackboardBasic(const MoreData &nmea_info) {
    assert(InMainThread());
    blackboard.ReadBlackboardBasic(nmea_info);
  }

  static void ReadBlackboardCalculated(const DerivedInfo& derived_info) {
    assert(InMainThread());

    blackboard.ReadBlackboardCalculated(derived_info);
  }

  static void AddListener(BlackboardListener &listener) {
    assert(InMainThread());

    blackboard.AddListener(listener);
  }

  static void RemoveListener(BlackboardListener &listener) {
    assert(InMainThread());

    blackboard.RemoveListener(listener);
  }

  static void BroadcastGPSUpdate() {
    assert(InMainThread());

    blackboard.BroadcastGPSUpdate();
  }

  static void BroadcastCalculatedUpdate() {
    assert(InMainThread());

    blackboard.BroadcastCalculatedUpdate();
  }

  static void BroadcastComputerSettingsUpdate() {
    assert(InMainThread());

    blackboard.BroadcastComputerSettingsUpdate();
  }

  static void BroadcastUISettingsUpdate() {
    assert(InMainThread());

    blackboard.BroadcastUISettingsUpdate();
  }
};

/** 
 * Class to hold data/methods accessible by interface subsystems
 * that can perform actions
 */
class ActionInterface: public CommonInterface {
  friend class ProcessTimer;

protected:
  static void SendGetComputerSettings();

  static bool force_shutdown;

public:
  /**
   * Configure a new Ballast setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  static void SetBallast(fixed ballast, bool to_devices=true);

  /**
   * Configure a new Bugs setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  static void SetBugs(fixed mc, bool to_devices=true);

  /**
   * Configure a new MacCready setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  static void SetMacCready(fixed mc, bool to_devices=true);

  /**
   * Configure a new MacCready setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it. Also switch
   * to manual MC mode.
   *
   * @param to_devices send the new settings to all devices?
   */
  static void SetManualMacCready(fixed mc, bool to_devices=true);

  /**
   * Call this after MapSettings has been modified with
   * SetMapSettings().  It sends the new values to all sub systems,
   * and optionally forces a redraw.
   * @param trigger_draw Triggers the draw event after sending if true
   */
  static void SendMapSettings(const bool trigger_draw = false);

public:
  // ideally these should be protected
  static void SignalShutdown(bool force);
};

/** 
 * Class to hold data/methods accessible by interface subsystems
 * of main program
 */
class XCSoarInterface: public ActionInterface {
public:
  static bool CheckShutdown();

  static void AfterStartup();
  static void Shutdown();
  static bool Startup();

  /**
   * Receive GPS data (#MoreData) from the DeviceBlackboard.
   */
  static void ReceiveGPS();

  /**
   * Receive calculated data (#DerivedInfo) from the DeviceBlackboard.
   */
  static void ReceiveCalculated();

  static void ExchangeBlackboard();

  /**
   * Copy data from and to the DeviceBlackboard.
   */
  static void ExchangeDeviceBlackboard();

private:
  static bool LoadProfile();
};

#endif
