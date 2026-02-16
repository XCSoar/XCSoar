// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/GeoPointEntry.hpp"
#include "Geo/GeoPoint.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/Macros.hpp"

#include <stdio.h>

static constexpr CoordinateFormat format = CoordinateFormat::DDMMSS;

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  GeoPoint value = GeoPoint(Angle::Degrees(7.7061111111111114),
                            Angle::Degrees(51.051944444444445));

  if (!GeoPointEntryDialog("The caption", value, format, true))
    return;

  if (value.IsValid())
    printf("%s\n",
             FormatGeoPoint(value, CoordinateFormat::DDMMSS).c_str());
  else
    printf("invalid\n");
}
