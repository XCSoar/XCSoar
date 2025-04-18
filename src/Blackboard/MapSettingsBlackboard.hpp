// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapSettings.hpp"
#include "util/Compiler.h"

/**
 * Blackboard for clients requiring read access to map settings
 */
class MapSettingsBlackboard
{
protected:
  MapSettings settings_map;

public:
  [[gnu::const]]
  const MapSettings& GetMapSettings() const {
    return settings_map;
  }
};
