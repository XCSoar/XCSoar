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
#include "Units/Units.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "FlightStatistics.hpp"
#include "Language/Language.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"

void
BarographCaption(TCHAR *sTmp, const FlightStatistics &fs)
{
  ScopeLock lock(fs.mutex);
  if (!fs.altitude_ceiling.HasResult() || fs.altitude_base.IsEmpty()) {
    sTmp[0] = _T('\0');
  } else if (fs.altitude_ceiling.GetCount() < 4) {
    StringFormatUnsafe(sTmp, _T("%s:\r\n  %.0f-%.0f %s"),
                       _("Working band"),
                       (double)Units::ToUserAltitude(fs.altitude_base.GetAverageY()),
                       (double)Units::ToUserAltitude(fs.altitude_ceiling.GetAverageY()),
                       Units::GetAltitudeName());
  } else {
    StringFormatUnsafe(sTmp, _T("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
                       _("Working band"),
                       (double)Units::ToUserAltitude(fs.altitude_base.GetAverageY()),
                       (double)Units::ToUserAltitude(fs.altitude_ceiling.GetAverageY()),
                       Units::GetAltitudeName(),
                       _("Ceiling trend"),
                       (double)Units::ToUserAltitude(fs.altitude_ceiling.GetGradient()),
                       Units::GetAltitudeName());
  }
}

static bool
IsTaskLegVisible(const OrderedTaskPoint &tp)
{
  switch (tp.GetType()) {
  case TaskPointType::START:
    return tp.HasExited();

  case TaskPointType::FINISH:
  case TaskPointType::AAT:
  case TaskPointType::AST:
    return tp.HasEntered();

  case TaskPointType::UNORDERED:
    break;
  }

  gcc_unreachable();
}

static void
DrawLegs(ChartRenderer &chart,
         const TaskManager &task_manager,
         const NMEAInfo& basic,
         const DerivedInfo& calculated,
         const bool task_relative)
{
  const TaskStats &task_stats = calculated.ordered_task_stats;

  if (!task_stats.start.task_started)
    return;

  const auto start_time = task_relative
    ? basic.time - task_stats.total.time_elapsed
    : calculated.flight.takeoff_time;

  const OrderedTask &task = task_manager.GetOrderedTask();
  for (unsigned i = 0, n = task.TaskSize(); i < n; ++i) {
    const OrderedTaskPoint &tp = task.GetTaskPoint(i);
    if (!IsTaskLegVisible(tp))
      continue;

    auto x = tp.GetEnteredState().time - start_time;
    if (x >= 0) {
      x /= 3600;
      chart.DrawLine(x, chart.GetYMin(), x, chart.GetYMax(),
                     ChartLook::STYLE_REDTHICK);
    }
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
  ChartRenderer chart(chart_look, canvas, rc);
  chart.padding_bottom = 0;
  chart.padding_left = 0;

  if (!fs.altitude.HasResult())
    return;

  chart.ScaleXFromData(fs.altitude);
  chart.ScaleYFromData(fs.altitude);
  chart.ScaleYFromValue(0);

  if (_task != nullptr) {
    ProtectedTaskManager::Lease task(*_task);
    canvas.SelectHollowBrush();
    DrawLegs(chart, task, nmea_info, derived_info, false);
  }

  canvas.SelectNullPen();
  canvas.Select(cross_section_look.terrain_brush);

  chart.DrawFilledLineGraph(fs.altitude_terrain);

  Pen pen(2, inverse ? COLOR_WHITE : COLOR_BLACK);
  chart.DrawLineGraph(fs.altitude, pen);
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

  chart.ScaleXFromData(fs.altitude);
  chart.ScaleYFromData(fs.altitude);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromValue(fs.altitude.GetMinX());
  if (!fs.altitude_ceiling.IsEmpty()) {
    chart.ScaleYFromValue(fs.altitude_ceiling.GetMaxY());
  }

  if (_task != nullptr) {
    ProtectedTaskManager::Lease task(*_task);
    DrawLegs(chart, task, nmea_info, derived_info, false);
  }

  canvas.SelectNullPen();
  canvas.Select(cross_section_look.terrain_brush);

  chart.DrawFilledLineGraph(fs.altitude_terrain);
  canvas.SelectWhitePen();
  canvas.SelectWhiteBrush();

  chart.DrawXGrid(0.5, 0.5, true);
  chart.DrawYGrid(Units::ToSysAltitude(1000), 1000, true);

  if (fs.altitude_base.HasResult()) {
    chart.DrawLineGraph(fs.altitude_base, ChartLook::STYLE_BLUETHIN);
  } else if (!fs.altitude_base.IsEmpty()) {
    chart.DrawTrend(fs.altitude_base, ChartLook::STYLE_BLUETHIN);
  }
  if (fs.altitude_ceiling.HasResult()) {
    chart.DrawLineGraph(fs.altitude_ceiling, ChartLook::STYLE_BLUETHIN);
  } else if (!fs.altitude_ceiling.IsEmpty()) {
    chart.DrawTrend(fs.altitude_ceiling, ChartLook::STYLE_BLUETHIN);
  }

  chart.DrawLineGraph(fs.altitude, ChartLook::STYLE_MEDIUMBLACK);

  chart.DrawXLabel(_T("t"), _T("hr"));
  chart.DrawYLabel(_T("h"), Units::GetAltitudeName());
}

void
RenderSpeed(Canvas &canvas, const PixelRect rc,
            const ChartLook &chart_look,
            const FlightStatistics &fs,
            const NMEAInfo &nmea_info,
            const DerivedInfo &derived_info,
            const TaskManager &task)
{
  ChartRenderer chart(chart_look, canvas, rc);

  if (!fs.task_speed.HasResult() || !task.CheckOrderedTask()) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(fs.task_speed);
  chart.ScaleYFromData(fs.task_speed);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromValue(fs.task_speed.GetMinX());

  DrawLegs(chart, task, nmea_info, derived_info, true);

  chart.DrawXGrid(0.5, 0.5, true);
  chart.DrawYGrid(Units::ToSysTaskSpeed(10), 10, true);
  chart.DrawLineGraph(fs.task_speed, ChartLook::STYLE_MEDIUMBLACK);
  chart.DrawTrend(fs.task_speed, ChartLook::STYLE_BLUETHIN);

  chart.DrawXLabel(_T("t"), _T("hr"));
  chart.DrawYLabel(_T("h"), Units::GetTaskSpeedName());
}
