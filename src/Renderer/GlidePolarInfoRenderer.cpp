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

#include "GlidePolarInfoRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Util/StaticString.hxx"


void
RenderGlidePolarInfo(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const GlidePolar &glide_polar)
{
  canvas.Select(chart_look.label_font);

  StaticString<80> text;
  StaticString<20> value;
  canvas.SetBackgroundTransparent();

  FormatUserMass(glide_polar.GetTotalMass(), value.buffer(), true);

  int left = rc.left*0.8 + rc.right*0.2;

  text.Format(_T("%s: %s"), _("Mass"), value.c_str());
  canvas.DrawText(left,
                  rc.bottom - Layout::Scale(50),
                  text);

  double wl = glide_polar.GetWingLoading();
  if (wl != 0) {
    FormatUserWingLoading(wl, value.buffer(), true);

    text.Format(_T("%s: %s"), _("Wing loading"), value.c_str());

    canvas.DrawText(left,
                    rc.bottom - Layout::Scale(35),
                    text);
  }
}
