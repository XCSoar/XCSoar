/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_AIRSPACE_RENDERER_SETTINGS_HPP
#define XCSOAR_AIRSPACE_RENDERER_SETTINGS_HPP

#include "Airspace/AirspaceClass.hpp"

/** Airspace display modes */
enum AirspaceDisplayMode_t
{
  ALLON = 0,
  CLIP,
  AUTO,
  ALLBELOW,
  INSIDE,
  ALLOFF
};


/**
 * Settings for airspace options
 */
struct AirspaceRendererSettings {
  bool enable;

  /** Airspaces are drawn with black border (otherwise in airspace color) */
  bool black_outline;

  /** Mode controlling how airspaces are filtered for display */
  AirspaceDisplayMode_t altitude_mode;

  /**< Altitude (m) above which airspace is not drawn for clip mode */
  unsigned clip_altitude;

  /** Class-specific display flags */
  bool display[AIRSPACECLASSCOUNT];

#ifndef ENABLE_OPENGL
  /**
   * Should the airspace be rendered with a transparent brush instead
   * of a pattern brush?
   */
  bool transparency;

  /**
   * What portion of the airspace area should be filled with the
   * airspace brush?
   */
  enum AirspaceFillMode {
    /** the platform specific default is used */
    AS_FILL_DEFAULT,

    /** fill all of the area */
    AS_FILL_ALL,

    /** fill only a thick padding (like on ICAO maps) */
    AS_FILL_PADDING,
  } fill_mode;
#endif

  int brushes[AIRSPACECLASSCOUNT];
  int colours[AIRSPACECLASSCOUNT];

  void SetDefaults();
};

#endif
