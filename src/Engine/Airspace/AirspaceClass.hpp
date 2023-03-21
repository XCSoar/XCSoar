// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum AirspaceClass : uint8_t
{
  OTHER = 0,
  RESTRICT,
  PROHIBITED,
  DANGER,
  CLASSA,
  CLASSB,
  CLASSC,
  CLASSD,
  NOGLIDER,
  CTR,
  WAVE,
  AATASK,
  CLASSE,
  CLASSF,
  TMZ,
  CLASSG,
  MATZ,
  RMZ,
  AIRSPACECLASSCOUNT
};
