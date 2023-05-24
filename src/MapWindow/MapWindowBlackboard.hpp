// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BaseBlackboard.hpp"
#include "Blackboard/ComputerSettingsBlackboard.hpp"
#include "Blackboard/MapSettingsBlackboard.hpp"
#include "thread/Debug.hpp"
#include "UIState.hpp"

/**
 * Blackboard used by map window: provides read-only access to local
 * copies of data required by map window
 * 
 */
class MapWindowBlackboard:
  public BaseBlackboard,
  public ComputerSettingsBlackboard,
  public MapSettingsBlackboard
{
  UIState ui_state;

protected:
  [[gnu::const]]
  const MoreData &Basic() const noexcept {
    assert(InDrawThread());

    return BaseBlackboard::Basic();
  }

  [[gnu::const]]
  const DerivedInfo &Calculated() const noexcept {
    assert(InDrawThread());

    return BaseBlackboard::Calculated();
  }

  [[gnu::const]]
  const ComputerSettings &GetComputerSettings() const noexcept {
    assert(InDrawThread());

    return ComputerSettingsBlackboard::GetComputerSettings();
  }

  [[gnu::const]]
  const MapSettings &GetMapSettings() const noexcept {
    assert(InDrawThread());

    return settings_map;
  }

  [[gnu::const]]
  const UIState &GetUIState() const noexcept {
    assert(InDrawThread());

    return ui_state;
  }

  void ReadBlackboard(const MoreData &nmea_info,
                      const DerivedInfo &derived_info) noexcept;
  void ReadComputerSettings(const ComputerSettings &settings) noexcept;
  void ReadMapSettings(const MapSettings &settings) noexcept;

  void ReadUIState(const UIState &new_value) noexcept {
    ui_state = new_value;
  }
};
