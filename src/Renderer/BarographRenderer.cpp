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

#include "BarographRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Look/CrossSectionLook.hpp"
#include "Screen/Canvas.hpp"
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
  ScopeLock lock(fs.mutex);
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
  ScopeLock lock(fs.mutex);
  ChartRenderer chart(chart_look, canvas, rc, false);

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
    const auto &s = fs.altitude.GetSlots()[fs.altitude.GetCount()-1];
    chart.DrawDot(s.x, s.y, Layout::Scale(2));
  }
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

  if (!fs.altitude.HasResult()) {
    chart.DrawNoData();
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
    chart.ScaleXFromValue(derived_info.flight.flight_time/3600);

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
  chart.DrawYGrid(Units::ToSysAltitude(1000), 1000, ChartRenderer::UnitFormat::NUMERIC);

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

  chart.DrawXLabel(_T("t"), _T("hr"));
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}

