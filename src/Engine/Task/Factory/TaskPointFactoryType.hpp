// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * Legal types of points with observation zones.
 */
enum class TaskPointFactoryType : uint8_t {
  START_SECTOR = 0,
  START_LINE,
  START_CYLINDER,
  FAI_SECTOR,
  CUSTOM_KEYHOLE,
  DAEC_KEYHOLE,
  BGAFIXEDCOURSE_SECTOR,
  BGAENHANCEDOPTION_SECTOR,
  AST_CYLINDER,
  MAT_CYLINDER,
  AAT_CYLINDER,
  AAT_SEGMENT,
  FINISH_SECTOR,
  FINISH_LINE,
  FINISH_CYLINDER,
  START_BGA,
  AAT_ANNULAR_SECTOR,
  SYMMETRIC_QUADRANT,
  AAT_KEYHOLE,

  /**
   * This special value is used to determine the number of types
   * above.
   */
  COUNT
};
