// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * Enumeration of factory types.  This is the set of
 * types of ordered task that can be created.
 */
enum class TaskFactoryType: uint8_t {
  FAI_GENERAL = 0,
  FAI_TRIANGLE,
  FAI_OR,
  FAI_GOAL,
  RACING,
  AAT,

  /**
   * Modified Area Task.
   */
  MAT,

  MIXED,
  TOURING,

  /**
   * This special value is used to determine the number of items
   * above.
   */
  COUNT
};

/**
 * returns true if task is an FAI type
 * @param ftype. task type being checked
 */
constexpr
static bool
IsFai(TaskFactoryType ftype)
{
  return ftype == TaskFactoryType::FAI_GENERAL ||
    ftype == TaskFactoryType::FAI_GOAL ||
    ftype == TaskFactoryType::FAI_OR ||
    ftype == TaskFactoryType::FAI_TRIANGLE ||
    ftype == TaskFactoryType::RACING;
}
