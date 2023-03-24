// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class TaskPointType : uint8_t {
  UNORDERED,
  START,
  AST,
  AAT,
  FINISH,
};
