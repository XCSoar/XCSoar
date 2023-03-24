// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlidePolarInfoRenderer.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "util/StaticString.hxx"


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
  canvas.DrawText({left, rc.bottom - (int)Layout::Scale(50u)}, text);

  double wl = glide_polar.GetWingLoading();
  if (wl != 0) {
    FormatUserWingLoading(wl, value.buffer(), true);

    text.Format(_T("%s: %s"), _("Wing loading"), value.c_str());

    canvas.DrawText({left, rc.bottom - (int)Layout::Scale(35u)},
                    text);
  }
}
