// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct InfoBoxSettings;
struct UIState;
struct DerivedInfo;

enum class DisplayMode: uint8_t {
  NONE,
  CIRCLING,
  CRUISE,
  FINAL_GLIDE,
};

[[gnu::pure]]
DisplayMode
GetNewDisplayMode(const InfoBoxSettings &settings, const UIState &ui_state,
                  const DerivedInfo &derived_info);
