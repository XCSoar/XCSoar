/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "AirspaceWarningConfig.hpp"

#include <algorithm>

void
AirspaceWarningConfig::SetDefaults()
{
  warning_time = 30;
  repetitive_sound = false;
  acknowledgement_time = 30;
  altitude_warning_margin = 100;

  std::fill_n(class_warnings, unsigned(AIRSPACECLASSCOUNT), true);
  class_warnings[CLASSE] = false;
  class_warnings[CLASSF] = false;
  class_warnings[CLASSG] = false;
  class_warnings[AATASK] = false;
}
