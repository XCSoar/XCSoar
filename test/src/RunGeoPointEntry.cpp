/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/GeoPointEntry.hpp"
#include "Geo/GeoPoint.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>

static void
Main()
{
  GeoPoint value = GeoPoint(Angle::Degrees(7.7061111111111114),
                            Angle::Degrees(51.051944444444445));

  if (!GeoPointEntryDialog(_T("The caption"), value, true))
    return;

  if (value.IsValid()) {
    TCHAR buffer[64];
    _tprintf(_T("%s\n"), FormatGeoPoint(value, buffer, ARRAY_SIZE(buffer),
                                        CoordinateFormat::DDMMSS));
  } else
    printf("invalid\n");
}
