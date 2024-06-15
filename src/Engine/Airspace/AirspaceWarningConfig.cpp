// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningConfig.hpp"

#include <algorithm>

void
AirspaceWarningConfig::SetDefaults()
{
  warning_time = std::chrono::seconds{30};
  repetitive_sound = false;
  acknowledgement_time = std::chrono::seconds{30};
  altitude_warning_margin = 100;

  std::fill_n(class_warnings, unsigned(AIRSPACECLASSCOUNT), true);
  class_warnings[CLASSE] = false;
  class_warnings[CLASSF] = false;
  class_warnings[CLASSG] = false;
  class_warnings[AATASK] = false;
  class_warnings[FIS_SECTOR] = false;
}
