/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_PAN_HPP
#define XCSOAR_PAN_HPP

#include "util/Compiler.h"

struct GeoPoint;

gcc_pure
bool
IsPanning();

void
EnterPan();

bool
PanTo(const GeoPoint &location);

/**
 * Low-level version of LeavePan().  It disables panning in the map
 * and updates the input mode, but does not restore the page layout.
 * Only to be used by the pages library.
 */
void
DisablePan();

void
LeavePan();

void
TogglePan();

#endif
