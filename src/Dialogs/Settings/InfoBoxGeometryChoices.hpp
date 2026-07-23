// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/DataField/Enum.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"

inline constexpr StaticEnumChoice info_box_geometry_choices[] = {
  { InfoBoxSettings::Geometry::SPLIT_8,
    N_("8 Split") },
  { InfoBoxSettings::Geometry::SPLIT_10,
    N_("10 Split") },
  { InfoBoxSettings::Geometry::SPLIT_3X4,
    N_("12 Split in 3 rows") },
  { InfoBoxSettings::Geometry::SPLIT_3X5,
    N_("15 Split in 3 rows") },
  { InfoBoxSettings::Geometry::SPLIT_3X6,
    N_("18 Split in 3 rows") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_8,
    N_("8 Bottom or Right") },
  { InfoBoxSettings::Geometry::BOTTOM_8_VARIO,
    N_("8 Bottom + Vario (Portrait)") },
  { InfoBoxSettings::Geometry::TOP_LEFT_8,
    N_("8 Top or Left") },
  { InfoBoxSettings::Geometry::TOP_8_VARIO,
    N_("8 Top + Vario (Portrait)") },
  { InfoBoxSettings::Geometry::RIGHT_9_VARIO,
    N_("9 Right + Vario (Landscape)") },
  { InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO,
    N_("9 Left + Right + Vario (Landscape)") },
  { InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO,
    N_("12 Left + 3 Right Vario (Landscape)") },
  { InfoBoxSettings::Geometry::RIGHT_5,
    N_("5 Right (Square)") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_10,
    N_("10 Bottom or Right") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_12,
    N_("12 Bottom or Right") },
  { InfoBoxSettings::Geometry::TOP_LEFT_10,
    N_("10 Top or Left") },
  { InfoBoxSettings::Geometry::TOP_LEFT_12,
    N_("12 Top or Left") },
  { InfoBoxSettings::Geometry::RIGHT_16,
    N_("16 Right (Landscape)") },
  { InfoBoxSettings::Geometry::RIGHT_24,
    N_("24 Bottom or Right") },
  { InfoBoxSettings::Geometry::TOP_LEFT_4,
    N_("4 Top or Left") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_4,
    N_("4 Bottom or Right") },
  nullptr
};
