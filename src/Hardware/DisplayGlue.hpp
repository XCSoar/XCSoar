// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayOrientation.hpp"

class VerboseOperationEnvironment;

namespace Display
{
  void LoadOrientation(VerboseOperationEnvironment &env);
  void RestoreOrientation();
  DisplayOrientation DetectInitialOrientation();
}
