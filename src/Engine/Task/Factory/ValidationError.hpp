// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/EnumBitSet.hxx"

#include <cstdint>

/** Task Validation Error Types */
enum class TaskValidationErrorType : uint8_t {
  NO_VALID_START,
  NO_VALID_FINISH,
  TASK_NOT_CLOSED,
  TASK_NOT_HOMOGENEOUS,
  INCORRECT_NUMBER_TURNPOINTS,
  EXCEEDS_MAX_TURNPOINTS,
  UNDER_MIN_TURNPOINTS,
  TURNPOINTS_NOT_UNIQUE,
  EMPTY_TASK,
  NON_FAI_OZS,
  NON_MAT_OZS,

  /**
   * The task doesn't have the required shape, e.g. for FAI triangle.
   */
  WRONG_SHAPE,

  /**
   * This special value is used to determine the number of items
   * above.
   */
  COUNT
};

using TaskValidationErrorSet = EnumBitSet<TaskValidationErrorType>;

/**
 * Is this an error which is not acceptable?  This ignores warnings.
 */
constexpr bool
IsError(TaskValidationErrorSet set) noexcept
{
  /**
   * These codes are just warnings; they may be displayed, but the
   * user is allowed to proceed.
   */
  constexpr TaskValidationErrorSet warnings{
    TaskValidationErrorType::TURNPOINTS_NOT_UNIQUE,
  };

  constexpr TaskValidationErrorSet errors = ~warnings;

  return !(set & errors).IsEmpty();
}
