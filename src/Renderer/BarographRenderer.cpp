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
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Language/Language.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "TaskLegRenderer.hpp"
#include "GradientRenderer.hpp"
#include "time/FloatDuration.hxx"
#include "time/RoughTime.hpp"
#include "time/Stamp.hpp"
#include "util/UTF8.hpp"

#include <cmath>
#include <cstring>
#include <fmt/format.h>

using namespace std::chrono;

void
BarographCaption(char *sTmp, size_t buffer_size, const FlightStatistics &fs)
{
  if (sTmp == nullptr || buffer_size == 0)
    return;

  const std::lock_guard lock{fs.mutex};

  if (!fs.altitude_ceiling.HasResult() || fs.altitude_base.IsEmpty()) {
    sTmp[0] = '\0';
  } else if (fs.altitude_ceiling.GetCount() < 4) {
    auto result = fmt::format_to_n(sTmp, buffer_size - 1, "{}:\r\n  {:.0f}-{:.0f} {}",
                                   _("Working band"),
                                   (double)Units::ToUserAltitude(fs.GetMinWorkingHeight()),
                                   (double)Units::ToUserAltitude(fs.GetMaxWorkingHeight()),
                                   Units::GetAltitudeName());
    *result.out = '\0';
    CropIncompleteUTF8(sTmp);
  } else {
    auto result = fmt::format_to_n(sTmp, buffer_size - 1,
                                   "{}:\r\n  {:.0f}-{:.0f} {}\r\n\r\n{}:\r\n  {:.0f} {}/hr",
                                   _("Working band"),
                                   (double)Units::ToUserAltitude(fs.GetMinWorkingHeight()),
                                   (double)Units::ToUserAltitude(fs.GetMaxWorkingHeight()),
                                   Units::GetAltitudeName(),
                                   _("Ceiling trend"),
                                   (double)Units::ToUserAltitude(fs.altitude_ceiling.GetGradient()),
                                   Units::GetAltitudeName());
    *result.out = '\0';
    CropIncompleteUTF8(sTmp);
  }
}

[[gnu::const]]
static double
ToClockHours(TimeStamp time) noexcept
{
  return duration_cast<duration<double, hours::period>>(time.ToDuration()).count();
}

[[gnu::const]]
static TimeStamp
ClockHoursToTimeStamp(double hours) noexcept
{
  return TimeStamp{
    duration_cast<FloatDuration>(duration<double, hours::period>{hours})};
}

/**
 * Hour ticks (short marks only, no full vertical grid) plus end labels.
 */
static void
DrawBarographClockAxis(ChartRenderer &chart,
                       RoughTimeDelta utc_offset) noexcept
{
  Canvas &canvas = chart.GetCanvas();
  const ChartLook &look = chart.GetLook();
  const PixelRect &rc_chart = chart.GetChartRect();

  canvas.Select(look.axis_value_font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(look.text_color);
  canvas.Select(look.GetPen(ChartLook::STYLE_GRIDMINOR));

  const double x_min = chart.GetXMin();
  const double x_max = chart.GetXMax();
  if (!(x_max > x_min))
    return;

  const int tick = Layout::VptScale(4);
  const int label_y = rc_chart.bottom + Layout::GetTextPadding();

  const auto start_text =
    FormatLocalTimeHHMM(ClockHoursToTimeStamp(x_min), utc_offset);
  const auto end_text =
    FormatLocalTimeHHMM(ClockHoursToTimeStamp(x_max), utc_offset);

  canvas.DrawText({rc_chart.left, label_y}, start_text.c_str());
  const auto end_width = canvas.CalcTextWidth(end_text.c_str());
  canvas.DrawText({rc_chart.right - int(end_width), label_y},
                  end_text.c_str());

  const double first_hour = std::ceil(x_min + 1e-9);
  for (double hour = first_hour; hour < x_max - 1e-9; hour += 1.) {
    const int x = chart.ScreenX(hour);
    if (x <= rc_chart.left || x >= rc_chart.right)
      continue;

    canvas.DrawLine({x, rc_chart.bottom}, {x, rc_chart.bottom - tick});
    canvas.DrawLine({x, rc_chart.top}, {x, rc_chart.top + tick});
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
  if (nmea_info.time_available)
    chart.ScaleXFromValue(ToClockHours(nmea_info.time));

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
                const ProtectedTaskManager *_task,
                RoughTimeDelta utc_offset) noexcept
{
  ChartRenderer chart(chart_look, canvas, rc);
  /* Reserve bottom margin for end-time labels (no "t [hr]" caption). */
  chart.SetXLabel(" ");
  chart.SetYLabel("h", Units::GetAltitudeName());
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
  if (nmea_info.time_available)
    chart.ScaleXFromValue(ToClockHours(nmea_info.time));

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

  Pen bg_pen(1, chart_look.background_color);
  Brush bg_brush(chart_look.background_color);
  canvas.Select(bg_pen);
  canvas.Select(bg_brush);

  chart.DrawYGrid(Units::ToSysAltitude(250), 250, ChartRenderer::UnitFormat::NUMERIC);
  DrawBarographClockAxis(chart, utc_offset);

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

