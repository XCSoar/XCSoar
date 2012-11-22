/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
namespace CommonInterface {
  namespace Private {
    extern UIState ui_state;
    extern InterfaceBlackboard blackboard;

    /**
     * True if movement was detected on a real GPS.
     */
    extern bool movement_detected;
  }

  // window.. make this protected TODO so have to subclass to get access
  extern StatusMessageList status_messages;
  extern MainWindow *main_window;

  static inline bool MovementDetected() {
    return Private::movement_detected;
  }

  // TODO: make this protected
  /**
   * Returns InterfaceBlackboard.Basic (NMEA_INFO) (read-only)
   * @return InterfaceBlackboard.Basic
   */
  gcc_const
  static inline const MoreData &Basic() {
    assert(InMainThread());

    return Private::blackboard.Basic();
  }

  /**
   * Returns InterfaceBlackboard.Calculated (DERIVED_INFO) (read-only)
   * @return InterfaceBlackboard.Calculated
   */
  gcc_const
  static inline const DerivedInfo &Calculated() {
    assert(InMainThread());

    return Private::blackboard.Calculated();
  }

  gcc_const
  static inline const SystemSettings &GetSystemSettings() {
    assert(InMainThread());

    return Private::blackboard.GetSystemSettings();
  }

  gcc_const
  static inline SystemSettings &SetSystemSettings() {
    assert(InMainThread());

    return Private::blackboard.SetSystemSettings();
  }

  /**
   * Returns the InterfaceBlackboard.ComputerSettings (read-only)
   * @return The InterfaceBlackboard.ComputerSettings
   */
  gcc_const
  static inline const ComputerSettings& GetComputerSettings() {
    assert(InMainThread());

    return Private::blackboard.GetComputerSettings();
  }

  /**
   * Returns the InterfaceBlackboard.ComputerSettings (read-write)
   * @return The InterfaceBlackboard.ComputerSettings
   */
  gcc_const
  static inline ComputerSettings &SetComputerSettings() {
    assert(InMainThread());

    return Private::blackboard.SetComputerSettings();
  }

  gcc_const
  static inline const UISettings &GetUISettings() {
    assert(InMainThread());

    return Private::blackboard.GetUISettings();
  }

  /**
   * Returns the InterfaceBlackboard.MapSettings (read-only)
   * @return The InterfaceBlackboard.MapSettings
   */
  gcc_const
  static inline const MapSettings& GetMapSettings() {
    assert(InMainThread());

    return GetUISettings().map;
  }

  gcc_const
  static inline const FullBlackboard &Full() {
    assert(InMainThread());

    return Private::blackboard;
  }

  gcc_const
  static inline LiveBlackboard &GetLiveBlackboard() {
    assert(InMainThread());

    return Private::blackboard;
  }

  gcc_const
  static inline UISettings &SetUISettings() {
    assert(InMainThread());

    return Private::blackboard.SetUISettings();
  }

  /**
   * Returns the InterfaceBlackboard.MapSettings (read-write)
   * @return The InterfaceBlackboard.MapSettings
   */
  gcc_const
  static inline MapSettings &SetMapSettings() {
    assert(InMainThread());

    return SetUISettings().map;
  }

  static inline const UIState &GetUIState() {
    assert(InMainThread());

    return Private::ui_state;
  }

  static inline UIState &SetUIState() {
    assert(InMainThread());

    return Private::ui_state;
  }

  static inline void ReadBlackboardBasic(const MoreData &nmea_info) {
    assert(InMainThread());

    Private::blackboard.ReadBlackboardBasic(nmea_info);
  }

  static inline void ReadBlackboardCalculated(const DerivedInfo &derived_info) {
    assert(InMainThread());

    Private::blackboard.ReadBlackboardCalculated(derived_info);
  }

  static inline void AddListener(BlackboardListener &listener) {
    assert(InMainThread());

    Private::blackboard.AddListener(listener);
  }

  static inline void RemoveListener(BlackboardListener &listener) {
    assert(InMainThread());

    Private::blackboard.RemoveListener(listener);
  }

  static inline void BroadcastGPSUpdate() {
    assert(InMainThread());

    Private::blackboard.BroadcastGPSUpdate();
  }

  static inline void BroadcastCalculatedUpdate() {
    assert(InMainThread());

    Private::blackboard.BroadcastCalculatedUpdate();
  }

  static inline void BroadcastComputerSettingsUpdate() {
    assert(InMainThread());

    Private::blackboard.BroadcastComputerSettingsUpdate();
  }

  static inline void BroadcastUISettingsUpdate() {
    assert(InMainThread());

    Private::blackboard.BroadcastUISettingsUpdate();
  }
};

/** 
 * Class to hold data/methods accessible by interface subsystems
 * that can perform actions
 */
namespace ActionInterface {
  using namespace CommonInterface;

  /**
   * Configure a new Ballast setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetBallast(fixed ballast, bool to_devices=true);

  /**
   * Configure a new Bugs setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetBugs(fixed mc, bool to_devices=true);

  /**
   * Configure a new MacCready setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetMacCready(fixed mc, bool to_devices=true);

  /**
   * Configure a new MacCready setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it. Also switch
   * to manual MC mode.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetManualMacCready(fixed mc, bool to_devices=true);

  /**
   * Same as SetManualMacCready(), but adds the given value to the
   * current MacCready setting.  It performs bounds checking.
   */
  void OffsetManualMacCready(fixed offset, bool to_devices=true);

  /**
   * Call this after MapSettings has been modified with
   * SetMapSettings().  It sends the new values to all sub systems,
   * and optionally forces a redraw.
   * @param trigger_draw Triggers the draw event after sending if true
   */
  void SendMapSettings(const bool trigger_draw = false);
};

/** 
 * Class to hold data/methods accessible by interface subsystems
 * of main program
 */
namespace XCSoarInterface {
  using namespace ActionInterface;

  void AfterStartup();
  void Shutdown();
  bool Startup();

  /**
   * Receive GPS data (#MoreData) from the DeviceBlackboard.
   */
  void ReceiveGPS();

  /**
   * Receive calculated data (#DerivedInfo) from the DeviceBlackboard.
   */
  void ReceiveCalculated();

  void ExchangeBlackboard();

  /**
   * Copy data from and to the DeviceBlackboard.
   */
  void ExchangeDeviceBlackboard();
};

#endif
