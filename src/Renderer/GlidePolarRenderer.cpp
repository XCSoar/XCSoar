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

#include "GlidePolarRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "NMEA/ClimbHistory.hpp"
#include "Formatter/UserUnits.hpp"
#include "Util/StaticString.hxx"

#include <stdio.h>

void
GlidePolarCaption(TCHAR *sTmp, const GlidePolar &glide_polar)
{
  if (!glide_polar.IsValid()) {
    *sTmp = _T('\0');
    return;
  }

  _stprintf(sTmp, Layout::landscape ?
                  _T("%s:\r\n  %d\r\n  at %d %s\r\n\r\n%s:\r\n  %3.2f %s\r\n  at %d %s") :
                  _T("%s:\r\n  %d at %d %s\r\n%s:\r\n  %3.2f %s at %d %s"),
            _("L/D"),
            (int)glide_polar.GetBestLD(),
            (int)Units::ToUserSpeed(glide_polar.GetVBestLD()),
            Units::GetSpeedName(),
            _("Min. sink"),
            (double)Units::ToUserVSpeed(glide_polar.GetSMin()),
            Units::GetVerticalSpeedName(),
            (int)Units::ToUserSpeed(glide_polar.GetVMin()),
            Units::GetSpeedName());
}

void
RenderGlidePolar(Canvas &canvas, const PixelRect rc,
                 const ChartLook &chart_look,
                 const ClimbHistory &climb_history,
                 const GlidePolar &glide_polar)
{
  ChartRenderer chart(chart_look, canvas, rc);

  if (!glide_polar.IsValid()) {
    chart.DrawNoData();
    return;
  }

  Pen blue_pen(2, COLOR_BLUE);

  chart.ScaleYFromValue(0);
  chart.ScaleYFromValue(-glide_polar.GetSMax() * 1.1);
  chart.ScaleXFromValue(glide_polar.GetVMin() * 0.8);
  chart.ScaleXFromValue(glide_polar.GetVMax() + 2);

  chart.DrawXGrid(Units::ToSysSpeed(10),
                  ChartLook::STYLE_THINDASHPAPER, 10, true);
  chart.DrawYGrid(Units::ToSysVSpeed(1),
                  ChartLook::STYLE_THINDASHPAPER, 1, true);

  double v0 = 0;
  bool v0valid = false;
  unsigned i0 = 0;

  const unsigned vmin = (unsigned)glide_polar.GetVMin();
  const unsigned vmax = (unsigned)glide_polar.GetVMax();
  for (unsigned i = vmin; i <= vmax; ++i) {
    auto sinkrate0 = -glide_polar.SinkRate(i);
    auto sinkrate1 = -glide_polar.SinkRate(i + 1);
    chart.DrawLine(i, sinkrate0, i + 1, sinkrate1,
                   ChartLook::STYLE_MEDIUMBLACK);

    if (climb_history.Check(i)) {
      auto v1 = climb_history.Get(i);

      if (v0valid)
        chart.DrawLine(i0, v0, i, v1, blue_pen);

      v0 = v1;
      i0 = i;
      v0valid = true;
    }
  }

  auto MACCREADY = glide_polar.GetMC();
  auto sb = -glide_polar.GetSBestLD();
  auto ff = (sb - MACCREADY) / glide_polar.GetVBestLD();

  chart.DrawLine(0, MACCREADY, glide_polar.GetVMax(),
                 MACCREADY + ff * glide_polar.GetVMax(),
                 ChartLook::STYLE_REDTHICK);

  chart.DrawXLabel(_T("V"), Units::GetSpeedName());
  chart.DrawYLabel(_T("w"), Units::GetVerticalSpeedName());

  canvas.Select(chart_look.label_font);

  StaticString<80> text;
  StaticString<20> value;
  canvas.SetBackgroundTransparent();

  FormatUserMass(glide_polar.GetTotalMass(), value.buffer(), true);

  text.Format(_T("%s: %s"), _("Mass"), value.c_str());
  canvas.DrawText(rc.left + Layout::Scale(30),
                  rc.bottom - Layout::Scale(55),
                  text);

  double wl = glide_polar.GetWingLoading();
  if (wl != 0) {
    FormatUserWingLoading(wl, value.buffer(), true);

    text.Format(_T("%s: %s"), _("Wing loading"), value.c_str());

    canvas.DrawText(rc.left + Layout::Scale(30),
                    rc.bottom - Layout::Scale(40),
                    text);
  }
}
