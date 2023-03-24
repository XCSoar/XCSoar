// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PolarInfo;

namespace PolarGlue
{
  PolarInfo GetDefault();

  bool LoadFromProfile(PolarInfo &polar);

  PolarInfo LoadFromProfile();
}
