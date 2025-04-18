// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BarographRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Look/CrossSectionLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "FlightStatistics.hpp"
#include "Language/Language.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "TaskLegRenderer.hpp"
#include "GradientRenderer.hpp"

void
BarographCaption(TCHAR *sTmp, const FlightStatistics &fs)
{
  const std::lock_guard lock{fs.mutex};
  if (!fs.altitude_ceiling.HasResult() || fs.altitude_base.IsEmpty()) {
    sTmp[0] = _T('\0');
  } else if (fs.altitude_ceiling.GetCount() < 4) {
    StringFormatUnsafe(sTmp, _T("%s:\r\n  %.0f-%.0f %s"),
                       _("Working band"),
                       (double)Units::ToUserAltitude(fs.GetMinWorkingHeight()),
                       (double)Units::ToUserAltitude(fs.GetMaxWorkingHeight()),
                       Units::GetAltitudeName());
  } else {
    StringFormatUnsafe(sTmp, _T("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
                       _("Working band"),
                       (double)Units::ToUserAltitude(fs.GetMinWorkingHeight()),
                       (double)Units::ToUserAltitude(fs.GetMaxWorkingHeight()),
                       Units::GetAltitudeName(),
                       _("Ceiling trend"),
                       (double)Units::ToUserAltitude(fs.altitude_ceiling.GetGradient()),
                       Units::GetAltitudeName());
  }
}

void
RenderBarographSpark(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const CrossSectionLook &cross_section_look,
                     bool inverse,
                     const FlightStatistics &fs,
                     const NMEAInfo &nmea_info,
                     const DerivedInfo &derived_info,
                     const ProtectedTaskManager *_task)
{
  const std::lock_guard lock{fs.mutex};
  ChartRenderer chart(chart_look, canvas, rc, false);
  chart.Begin();

  if (!fs.altitude.HasResult())
    return;

  chart.ScaleXFromData(fs.altitude);
  chart.ScaleYFromData(fs.altitude);
  chart.ScaleYFromValue(0);

  if (_task != nullptr) {
    ProtectedTaskManager::Lease task(*_task);
    canvas.SelectHollowBrush();
    RenderTaskLegs(chart, task, nmea_info, derived_info, -1);
  }

  canvas.SelectNullPen();
  canvas.Select(cross_section_look.terrain_brush);

  chart.DrawFilledLineGraph(fs.altitude_terrain);

  chart.DrawLineGraph(fs.altitude, inverse? ChartLook::STYLE_WHITE: ChartLook::STYLE_BLACK);

  // draw dot
  if (fs.altitude.GetCount()) {
    if (inverse)
      chart.GetCanvas().SelectWhiteBrush();
    else
      chart.GetCanvas().SelectBlackBrush();

    const auto &slots = fs.altitude.GetSlots();
    const auto &s = slots[fs.altitude.GetCount()-1];
    chart.DrawDot(s, Layout::Scale(2));
  }

  chart.Finish();
}

void
RenderBarograph(Canvas &canvas, const PixelRect rc,
                const ChartLook &chart_look,
                const CrossSectionLook &cross_section_look,
                const FlightStatistics &fs,
                const NMEAInfo &nmea_info,
                const DerivedInfo &derived_info,
                const ProtectedTaskManager *_task)
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.SetXLabel(_T("t"), _T("hr"));
  chart.SetYLabel(_T("h"), Units::GetAltitudeName());
  chart.Begin();

  if (!fs.altitude.HasResult()) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  DrawVerticalGradient(canvas, chart.GetChartRect(),
                       cross_section_look.sky_color, cross_section_look.background_color,
                       cross_section_look.background_color);

  chart.ScaleXFromData(fs.altitude);
  chart.ScaleYFromData(fs.altitude);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromValue(fs.altitude.GetMinX());
  if (derived_info.flight.flying)
    chart.ScaleXFromValue(derived_info.flight.flight_time / std::chrono::hours{1});

  if (!fs.altitude_ceiling.IsEmpty()) {
    chart.ScaleYFromValue(fs.altitude_ceiling.GetMaxY());
  }

  if (_task != nullptr) {
    ProtectedTaskManager::Lease task(*_task);
    RenderTaskLegs(chart, task, nmea_info, derived_info, 0.33);
  }

  canvas.SelectNullPen();
  canvas.Select(cross_section_look.terrain_brush);

  chart.DrawFilledLineGraph(fs.altitude_terrain);
  canvas.SelectWhitePen();
  canvas.SelectWhiteBrush();

  chart.DrawXGrid(0.25, 0.25, ChartRenderer::UnitFormat::TIME);
  chart.DrawYGrid(Units::ToSysAltitude(250), 250, ChartRenderer::UnitFormat::NUMERIC);

  if (fs.altitude_base.HasResult()) {
    chart.DrawLineGraph(fs.altitude_base, ChartLook::STYLE_REDTHICKDASH);
  } else if (!fs.altitude_base.IsEmpty()) {
    chart.DrawTrend(fs.altitude_base, ChartLook::STYLE_REDTHICKDASH);
  }
  if (fs.altitude_ceiling.HasResult()) {
    chart.DrawLineGraph(fs.altitude_ceiling, ChartLook::STYLE_BLUETHINDASH);
  } else if (!fs.altitude_ceiling.IsEmpty()) {
    chart.DrawTrend(fs.altitude_ceiling, ChartLook::STYLE_BLUETHINDASH);
  }

  chart.DrawLineGraph(fs.altitude, ChartLook::STYLE_BLACK);
  chart.Finish();
}

