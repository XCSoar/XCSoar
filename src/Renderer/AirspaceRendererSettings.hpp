/*
Copyright_License {

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

#ifndef XCSOAR_AIRSPACE_RENDERER_SETTINGS_HPP
#define XCSOAR_AIRSPACE_RENDERER_SETTINGS_HPP

#include "Airspace/AirspaceClass.hpp"
#include "Screen/Features.hpp"
#include "Screen/PortableColor.hpp"

#include <stdint.h>

/** Airspace display modes */
enum class AirspaceDisplayMode: uint8_t
{
  ALLON = 0,
  CLIP,
  AUTO,
  ALLBELOW,
  INSIDE,
  ALLOFF
};

struct AirspaceClassRendererSettings
{
  /** Class-specific display flags */
  bool display;

#ifdef HAVE_HATCHED_BRUSH
  uint8_t brush;
#endif

  RGB8Color border_color;
  RGB8Color fill_color;

  unsigned border_width;

  /**
   * What portion of the airspace area should be filled with the
   * airspace brush?
   *
   * (Only used if the parent FillMode is not ALL)
   */
  enum class FillMode: uint8_t
  {
    /** fill all of the area */
    ALL,

    /** fill only a thick padding (like on ICAO maps) */
    PADDING,

    /** don't fill anything */
    NONE,
  } fill_mode;

  void SetDefaults();

  void SetColors(RGB8Color color) {
    border_color = fill_color = color;
  }
};

/**
 * Settings for airspace options
 */
struct AirspaceRendererSettings {
  bool enable;

  /** Airspaces are drawn with black border (otherwise in airspace color) */
  bool black_outline;

  /** Mode controlling how airspaces are filtered for display */
  AirspaceDisplayMode altitude_mode;

  /** Altitude (m) above which airspace is not drawn for clip mode */
  unsigned clip_altitude;

#if defined(HAVE_HATCHED_BRUSH) && defined(HAVE_ALPHA_BLEND)
  /**
   * Should the airspace be rendered with a transparent brush instead
   * of a pattern brush?
   */
  bool transparency;
#endif

  /**
   * What portion of the airspace area should be filled with the
   * airspace brush?
   */
  enum class FillMode: uint8_t {
    /** the platform specific default is used */
    DEFAULT,

    /** fill all of the area */
    ALL,

    /** fill only a thick padding (like on ICAO maps) */
    PADDING,

    /** don't fill anything */
    NONE,
  } fill_mode;

  /** What type of airspace labels to render */
  enum class LabelSelection : uint8_t {
    NONE,
    ALL,
  } label_selection;

  AirspaceClassRendererSettings classes[AIRSPACECLASSCOUNT];

  void SetDefaults();
};

#endif
