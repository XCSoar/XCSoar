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
#include "GlidePolarInfoRenderer.hpp"

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

  const auto MACCREADY = glide_polar.GetMC();
  const auto s_min = -glide_polar.GetSMax();
  const auto vmin = glide_polar.GetVMin();
  const auto vmax = glide_polar.GetVMax();

  chart.ScaleYFromValue(MACCREADY);
  chart.ScaleYFromValue(s_min);
  chart.ScaleXFromValue(vmin);
  chart.ScaleXFromValue(vmax);

  chart.DrawXGrid(Units::ToSysSpeed(10), 10, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawYGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);

  // draw dolphin speed command
  auto w = glide_polar.GetSMin()+MACCREADY;
  bool inrange = true;
  auto v_dolphin_last = vmin;
  auto w_dolphin_last = MACCREADY;
  double v_dolphin_last_l = 0;
  double w_dolphin_last_l = 0;
  do {
    w += s_min*0.05;
    auto v_dolphin = glide_polar.SpeedToFly(-w, 0);
    auto w_dolphin = -glide_polar.SinkRate(v_dolphin)+w;
    inrange = w_dolphin > s_min;
    if ((v_dolphin > v_dolphin_last) && inrange) {
      chart.DrawLine(v_dolphin_last, w_dolphin_last, v_dolphin, w_dolphin,
                     ChartLook::STYLE_REDTHICKDASH);
      v_dolphin_last = v_dolphin;
      w_dolphin_last = w_dolphin;
      if ((w_dolphin < 0.8*s_min) && (w_dolphin_last_l >= 0)) {
        v_dolphin_last_l = v_dolphin;
        w_dolphin_last_l = w_dolphin;
      }
    }
  } while (inrange);

  // draw glide polar and climb rate history
  double v0 = 0;
  bool v0valid = false;
  double i0 = 0;

  const auto dv = (vmax-vmin)/50;
  for (auto i = vmin; i <= vmax; i+= dv) {
    auto sinkrate0 = -glide_polar.SinkRate(i);
    auto sinkrate1 = -glide_polar.SinkRate(i+dv);
    chart.DrawLine(i, sinkrate0, i + dv, sinkrate1,
                   ChartLook::STYLE_BLACK);

    if (climb_history.Check(i)) {
      auto v1 = climb_history.Get(i);

      if (v0valid)
        chart.DrawLine(i0, v0, i, v1, ChartLook::STYLE_BLUE);

      v0 = v1;
      i0 = i;
      v0valid = true;
    }
  }

  // draw best L/D line
  auto sb = -glide_polar.GetSBestLD();
  auto slope = (sb - MACCREADY) / glide_polar.GetVBestLD();

  chart.DrawLine(vmin, MACCREADY + slope * vmin,
                 vmax, MACCREADY + slope * vmax,
                 ChartLook::STYLE_BLUETHINDASH);

  // draw labels and other overlays

  double vv = 0.9*vmax+0.1*vmin;
  chart.DrawLabel(_T("Polar"), vv, -glide_polar.SinkRate(vv));
  vv = 0.8*vmax+0.2*vmin;
  chart.DrawLabel(_T("Best glide"), vv, MACCREADY + slope * vv);
  chart.DrawLabel(_T("Dolphin"), v_dolphin_last_l, w_dolphin_last_l);

  chart.DrawXLabel(_T("V"), Units::GetSpeedName());
  chart.DrawYLabel(_T("w"), Units::GetVerticalSpeedName());

  RenderGlidePolarInfo(canvas, rc, chart_look, glide_polar);
}
