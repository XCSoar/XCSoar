// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class TaskType : uint8_t {
  NONE,
  ORDERED,
  ABORT,
  GOTO,
};
