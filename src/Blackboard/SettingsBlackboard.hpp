// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ComputerSettingsBlackboard.hpp"
#include "SystemSettings.hpp"
#include "UISettings.hpp"

/**
 * A blackboard which contains all settings.
 */
class SettingsBlackboard : public ComputerSettingsBlackboard {
protected:
  SystemSettings system_settings;
  UISettings ui_settings;

public:
  constexpr const SystemSettings &GetSystemSettings() const noexcept {
    return system_settings;
  }

  constexpr const UISettings &GetUISettings() const noexcept {
    return ui_settings;
  }

  constexpr const MapSettings &GetMapSettings() const noexcept {
    return ui_settings.map;
  }
};
