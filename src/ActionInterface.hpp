// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
void
SetBallast(double ballast, bool to_devices=true) noexcept;

/**
 * Configure a new Bugs setting in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 *
 * @param to_devices send the new settings to all devices?
 */
void
SetBugs(double mc, bool to_devices=true) noexcept;

/**
 * Configure a new MacCready setting in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 *
 * @param to_devices send the new settings to all devices?
 */
void
SetMacCready(double mc, bool to_devices=true) noexcept;

/**
 * Configure a new MacCready setting in #ComputerSettings, and
 * forward it to all XCSoar modules that want it. Also switch
 * to manual MC mode.
 *
 * @param to_devices send the new settings to all devices?
 */
void
SetManualMacCready(double mc, bool to_devices=true) noexcept;

/**
 * Same as SetManualMacCready(), but adds the given value to the
 * current MacCready setting.  It performs bounds checking.
 */
void
OffsetManualMacCready(double offset, bool to_devices=true) noexcept;

/**
 * Call this after MapSettings has been modified with
 * SetMapSettings().  It sends the new values to all sub systems,
 * and optionally forces a redraw.
 *
 * @param trigger_draw triggers a map redraw immediately if true,
 * rather than waiting for eventual redraw
 */
void
SendMapSettings(const bool trigger_draw = false) noexcept;

/**
 * Call this after #UIState has been modified with SetUIState().  It
 * sends the new values to all sub systems, and optionally forces a
 * redraw.
 *
 * @param trigger_draw triggers a map redraw immediately if true,
 * rather than waiting for eventual redraw
 */
void
SendUIState(const bool trigger_draw) noexcept;

/**
 * Update UIState::display_mode and other attributes related to it.
 * You may have to call SendUIState() after this.
 */
void
UpdateDisplayMode() noexcept;

/**
 * Call this after UIState has been modified (via SetUIState() or
 * UpdateDisplayMode()).  It sends the new values to all subsystems,
 * and redraws relevant parts of the screen.
 */
void
SendUIState() noexcept;

/**
 * Update the Active Radio Frequency in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 *
 * @param to_devices send the new setting to all devices?
 */
void
SetActiveFrequency(RadioFrequency freq, const TCHAR *freq_name,
                   bool to_devices=true) noexcept;

/**
 * Update the Standby Radio Frequency in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 *
 * @param to_devices send the new setting to all devices?
 */
void
SetStandbyFrequency(RadioFrequency freq, const TCHAR *freq_name,
                    bool to_devices=true) noexcept;

/**
 * Offset the Active Radio Frequency in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 */
void
OffsetActiveFrequency(double offset_khz, bool to_devices=true) noexcept;

/**
 * Offset the Active Radio Frequency in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 */
void
OffsetStandbyFrequency(double offset_khz, bool to_devices=true) noexcept;

/**
 * Exchange the Active and Standby Radio Frequencies in #ComputerSettings, and
 * forward them to all XCSoar modules that want it.
 */
void
ExchangeRadioFrequencies(bool to_devices=true) noexcept;

/**
 * Update the Transponder Code in #ComputerSettings, and
 * forward it to all XCSoar modules that want it.
 *
 * @param to_devices send the new setting to all devices?
 */
void
SetTransponderCode(TransponderCode code, bool to_devices=true) noexcept;

} // namespace ActionInterface

/**
 * Class to hold data/methods accessible by interface subsystems
 * of main program
 */
namespace XCSoarInterface {
using namespace ActionInterface;

/**
 * Receive GPS data (#MoreData) from the DeviceBlackboard.
 */
void
ReceiveGPS() noexcept;

/**
 * Receive calculated data (#DerivedInfo) from the DeviceBlackboard.
 */
void
ReceiveCalculated() noexcept;

void
ExchangeBlackboard() noexcept;

/**
 * Copy data from and to the DeviceBlackboard.
 */
void
ExchangeDeviceBlackboard() noexcept;

} // namespace XCSoarInterface
