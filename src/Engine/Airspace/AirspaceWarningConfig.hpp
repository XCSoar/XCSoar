/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef AIRSPACE_WARNING_CONFIG_HPP
#define AIRSPACE_WARNING_CONFIG_HPP

#include "AirspaceClass.hpp"

#include <assert.h>
#include <algorithm>

struct AirspaceWarningConfig {
  /** Warning time before airspace entry */
  unsigned WarningTime;

  /** Time an acknowledgement will persist before a warning is reissued */
  unsigned AcknowledgementTime;

  /** Class-specific warning flags */
  bool class_warnings[AIRSPACECLASSCOUNT];

  AirspaceWarningConfig()
    :WarningTime(30), AcknowledgementTime(30) {
    std::fill(class_warnings, class_warnings + AIRSPACECLASSCOUNT, true);
  }

  bool class_enabled(AirspaceClass_t cls) const {
    assert(cls >= 0 && cls < AIRSPACECLASSCOUNT);

    return class_warnings[cls];
  }
};

#endif
