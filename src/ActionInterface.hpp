/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_ACTION_INTERFACE_HPP
#define XCSOAR_ACTION_INTERFACE_HPP

#include "Interface.hpp"

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
  void SetBallast(double ballast, bool to_devices=true);

  /**
   * Configure a new Bugs setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetBugs(double mc, bool to_devices=true);

  /**
   * Configure a new MacCready setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetMacCready(double mc, bool to_devices=true);

  /**
   * Configure a new MacCready setting in #ComputerSettings, and
   * forward it to all XCSoar modules that want it. Also switch
   * to manual MC mode.
   *
   * @param to_devices send the new settings to all devices?
   */
  void SetManualMacCready(double mc, bool to_devices=true);

  /**
   * Same as SetManualMacCready(), but adds the given value to the
   * current MacCready setting.  It performs bounds checking.
   */
  void OffsetManualMacCready(double offset, bool to_devices=true);

  /**
   * Call this after MapSettings has been modified with
   * SetMapSettings().  It sends the new values to all sub systems,
   * and optionally forces a redraw.
   *
   * @param trigger_draw triggers a map redraw immediately if true,
   * rather than waiting for eventual redraw
   */
  void SendMapSettings(const bool trigger_draw = false);

  /**
   * Call this after #UIState has been modified with SetUIState().  It
   * sends the new values to all sub systems, and optionally forces a
   * redraw.
   *
   * @param trigger_draw triggers a map redraw immediately if true,
   * rather than waiting for eventual redraw
   */
  void SendUIState(const bool trigger_draw);

  /**
   * Update UIState::display_mode and other attributes related to it.
   * You may have to call SendUIState() after this.
   */
  void UpdateDisplayMode();

  /**
   * Call this after UIState has been modified (via SetUIState() or
   * UpdateDisplayMode()).  It sends the new values to all subsystems,
   * and redraws relevant parts of the screen.
   */
  void SendUIState();
};

/** 
 * Class to hold data/methods accessible by interface subsystems
 * of main program
 */
namespace XCSoarInterface {
  using namespace ActionInterface;

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
