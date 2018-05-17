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

#include "MacCreadyRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Util/StaticString.hxx"
#include "GlidePolarInfoRenderer.hpp"

#define MAX_MACCREADY 5.2
#define STEPS_MACCREADY 25

void
MacCreadyCaption(TCHAR *sTmp, const GlidePolar &glide_polar)
{
  if (!glide_polar.IsValid()) {
    *sTmp = _T('\0');
    return;
  }

  _stprintf(sTmp,
            _T("%s: %d %s\r\n%s: %d %s"),
            _("Vopt"),
            (int)Units::ToUserSpeed(glide_polar.GetVBestLD()),
            Units::GetSpeedName(),
            _("Vave"),
            (int)Units::ToUserTaskSpeed(glide_polar.GetAverageSpeed()),
            Units::GetTaskSpeedName());
}


void
RenderMacCready(Canvas &canvas, const PixelRect rc,
                 const ChartLook &chart_look,
                 const GlidePolar &glide_polar)
{
  ChartRenderer chart(chart_look, canvas, rc);

  if (!glide_polar.IsValid()) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(MAX_MACCREADY);
  chart.ScaleYFromValue(0);
  chart.ScaleYFromValue(glide_polar.GetVMax());

  chart.DrawXGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);
  chart.DrawYGrid(Units::ToSysSpeed(10), 10, ChartRenderer::UnitFormat::NUMERIC);

  GlidePolar gp = glide_polar;
  double m = 0;
  double m_last;
  gp.SetMC(m);
  double v_last = gp.GetVBestLD();
  double vav_last = 0;
  do {
    m_last = m;
    m+= MAX_MACCREADY/STEPS_MACCREADY;
    gp.SetMC(m);
    const double v = gp.GetVBestLD();
    const double vav = gp.GetAverageSpeed();
    chart.DrawLine(m_last, v_last, m, v, ChartLook::STYLE_BLACK);
    chart.DrawLine(m_last, vav_last, m, vav, ChartLook::STYLE_BLUETHINDASH);
    v_last = v;
    vav_last = vav;
  } while (m<MAX_MACCREADY);

  // draw current MC setting
  chart.DrawLine(glide_polar.GetMC(), 0, glide_polar.GetMC(), glide_polar.GetVMax(),
                 ChartLook::STYLE_REDTHICKDASH);

  // draw labels and other overlays

  gp.SetMC(0.9*MAX_MACCREADY);
  chart.DrawLabel(_T("Vopt"), 0.9*MAX_MACCREADY, gp.GetVBestLD());
  gp.SetMC(0.9*MAX_MACCREADY);
  chart.DrawLabel(_T("Vave"), 0.9*MAX_MACCREADY, gp.GetAverageSpeed());

  chart.DrawYLabel(_T("V"), Units::GetSpeedName());
  chart.DrawXLabel(_T("MC"), Units::GetVerticalSpeedName());

  RenderGlidePolarInfo(canvas, rc, chart_look, glide_polar);
}
