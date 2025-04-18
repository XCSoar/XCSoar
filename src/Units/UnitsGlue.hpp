// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct UnitSetting;

namespace Units
{
  [[gnu::const]]
  const UnitSetting &LoadFromOSLanguage();
}
