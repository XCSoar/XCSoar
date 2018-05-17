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

#ifndef XCSOAR_AIRSPACE_PREVIEW_RENDERER_HPP
#define XCSOAR_AIRSPACE_PREVIEW_RENDERER_HPP

#include "Engine/Airspace/AirspaceClass.hpp"

struct PixelPoint;
class Canvas;
class AbstractAirspace;
struct AirspaceRendererSettings;
struct AirspaceLook;

namespace AirspacePreviewRenderer
{
  bool PrepareFill(Canvas &canvas, AirspaceClass type,
                   const AirspaceLook &look,
                   const AirspaceRendererSettings &settings);

  void UnprepareFill(Canvas &canvas);

  bool PrepareOutline(Canvas &canvas, AirspaceClass type,
                      const AirspaceLook &look,
                      const AirspaceRendererSettings &settings);

  /** Draw a scaled preview of the given airspace */
  void Draw(Canvas &canvas, const AbstractAirspace &airspace,
            const PixelPoint pt, unsigned radius,
            const AirspaceRendererSettings &settings,
            const AirspaceLook &look);
}

#endif
