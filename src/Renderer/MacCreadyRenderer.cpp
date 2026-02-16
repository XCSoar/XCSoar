// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MacCreadyRenderer.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "util/StaticString.hxx"
#include "GlidePolarInfoRenderer.hpp"

static constexpr double MAX_MACCREADY = 5.2;
static constexpr unsigned STEPS_MACCREADY = 25;

void
MacCreadyCaption(char *sTmp, const GlidePolar &glide_polar)
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
  chart.SetYLabel(_T("V"), Units::GetSpeedName());
  chart.SetXLabel(_T("MC"), Units::GetVerticalSpeedName());
  chart.Begin();

  if (!glide_polar.IsValid()) {
    chart.DrawNoData();
    chart.Finish();
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
  gp.SetMC(m);
  double v_last = gp.GetVBestLD();
  double vav_last = 0;
  do {
    double m_last = m;
    m+= MAX_MACCREADY/STEPS_MACCREADY;
    gp.SetMC(m);
    const double v = gp.GetVBestLD();
    const double vav = gp.GetAverageSpeed();
    chart.DrawLine({m_last, v_last}, {m, v}, ChartLook::STYLE_BLACK);
    chart.DrawLine({m_last, vav_last}, {m, vav}, ChartLook::STYLE_BLUETHINDASH);
    v_last = v;
    vav_last = vav;
  } while (m<MAX_MACCREADY);

  // draw current MC setting
  chart.DrawLine({glide_polar.GetMC(), 0},
                 {glide_polar.GetMC(), glide_polar.GetVMax()},
                 ChartLook::STYLE_REDTHICKDASH);

  // draw labels and other overlays

  gp.SetMC(0.9*MAX_MACCREADY);
  chart.DrawLabel({0.9*MAX_MACCREADY, gp.GetVBestLD()}, _T("Vopt"));
  gp.SetMC(0.9*MAX_MACCREADY);
  chart.DrawLabel({0.9*MAX_MACCREADY, gp.GetAverageSpeed()}, _T("Vave"));

  chart.Finish();

  RenderGlidePolarInfo(canvas, rc, chart_look, glide_polar);
}
