// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RangePlot.hpp"
#include "FLARM/Range.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"
#include "Look/DialogLook.hpp"
#include "Look/FlarmTrafficLook.hpp"
#include "Look/TrafficLook.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Math/Angle.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>
#include <cmath>

/**
 * The bearing of the middle of sector @p i of @p n, relative to the
 * direction of flight.
 */
[[gnu::const]]
static Angle
SectorBearing(unsigned i, unsigned n) noexcept
{
  return Angle::FullCircle() * ((i + 0.5) / n);
}

bool
IsBelowMinimum(const FlarmRangeSector &sector,
               unsigned i, unsigned n) noexcept
{
  return sector.significant && sector.range &&
    *sector.range < FlarmMinimumRange(SectorBearing(i, n));
}

/**
 * The distance between two range rings, in the user's distance unit:
 * 1, 2 or 5 times a power of ten, so that up to four rings cover
 * @p max_range.
 */
[[gnu::const]]
static double
RingStep(double max_range) noexcept
{
  const double magnitude = std::pow(10, std::floor(std::log10(max_range)));
  for (const double factor : {0.5, 1., 2., 5., 10.})
    if (max_range / (factor * magnitude) <= 4)
      return factor * magnitude;

  return 10 * magnitude;
}

FlarmRangePlot::FlarmRangePlot(const DialogLook &_dialog_look,
                               const FlarmTrafficLook &_look) noexcept
  :dialog_look(_dialog_look), look(_look),
   radar(Layout::GetTextPadding()),
   minimum_pen(Pen::DASH2, Layout::ScalePenWidth(2),
               TrafficLook::warning_color),
   limit_pen(Pen::DASH1, Layout::ScaleFinePenWidth(1), COLOR_BLACK)
{
}

void
FlarmRangePlot::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);
  radar.UpdateLayout(GetClientRect());
}

void
FlarmRangePlot::PaintSectors(Canvas &canvas,
                             double full_range) const noexcept
{
  const unsigned n = sectors.size();
  const Angle width = Angle::FullCircle() / n;
  const unsigned radius = radar.GetRadius();

  for (unsigned i = 0; i < n; ++i) {
    const auto &sector = sectors[i];
    if (!sector.range)
      continue;

    if (sector.significant) {
      canvas.SelectNullPen();
      canvas.Select(IsBelowMinimum(sector, i, n)
                    ? look.warning_brush
                    : look.radar_brush);
    } else {
      canvas.Select(look.radar_pen);
      canvas.SelectHollowBrush();
    }

    const Angle start = width * i;
    canvas.DrawSegment(radar.GetCenter(),
                       unsigned(std::min(*sector.range / full_range, 1.)
                                * radius),
                       start, start + width);
  }
}

void
FlarmRangePlot::PaintMinimum(Canvas &canvas,
                             double full_range) const noexcept
{
  constexpr unsigned STEPS = 72;
  BulkPixelPoint points[STEPS];
  const unsigned radius = radar.GetRadius();
  for (unsigned i = 0; i < STEPS; ++i) {
    const Angle bearing = Angle::FullCircle() * (double(i) / STEPS);
    points[i] = radar.At(bearing,
                         unsigned(FlarmMinimumRange(bearing) / full_range
                                  * radius));
  }

  canvas.Select(minimum_pen);
  canvas.SelectHollowBrush();
  canvas.DrawPolygon(points, STEPS);
}

/**
 * The device does not report targets beyond its RANGE setting.  The
 * plot is scaled to the data, not to the setting, so the circle only
 * shows when the setting is close enough to limit what was received.
 */
void
FlarmRangePlot::PaintLimit(Canvas &canvas,
                           double full_range) const noexcept
{
  if (!limit || *limit >= full_range)
    return;

  canvas.Select(limit_pen);
  canvas.SelectHollowBrush();
  radar.DrawCircle(canvas, unsigned(*limit / full_range * radar.GetRadius()));
}

void
FlarmRangePlot::PaintRings(Canvas &canvas, double step,
                           unsigned rings) const noexcept
{
  canvas.Select(look.radar_pen);
  canvas.SelectHollowBrush();
  canvas.Select(look.label_font);
  canvas.SetTextColor(dialog_look.text_color);
  canvas.SetBackgroundTransparent();

  const unsigned radius = radar.GetRadius();
  for (unsigned k = 1; k <= rings; ++k) {
    const unsigned r = radius * k / rings;
    radar.DrawCircle(canvas, r);

    /* a step below 1 is 0.5, which needs one decimal */
    const auto label =
      FormatUserDistance(Units::ToSysDistance(step * k), true,
                         step < 1 ? 1 : 0);
    canvas.DrawText(radar.At(Angle::Degrees(135), r), label.c_str());
  }
}

void
FlarmRangePlot::PaintAircraft(Canvas &canvas) const noexcept
{
  /* an arrow pointing in the direction of flight */
  const int size = Layout::Scale(6);
  const BulkPixelPoint arrow[] = {
    radar.At(0, -2 * size),
    radar.At(size, size),
    radar.At(0, 0),
    radar.At(-size, size),
  };

  canvas.Select(look.plane_pen);
  canvas.Select(look.default_brush);
  canvas.DrawPolygon(arrow, std::size(arrow));
}

void
FlarmRangePlot::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(dialog_look.background_color);

  unsigned max_range = 0;
  for (const auto &sector : sectors)
    if (sector.range)
      max_range = std::max(max_range, *sector.range);

  if (max_range == 0) {
    /* no sector has a value, so there is nothing to scale to */
    canvas.Select(look.label_font);
    canvas.SetTextColor(dialog_look.text_color);
    canvas.SetBackgroundTransparent();
    const char *text = _("No data");
    const auto size = canvas.CalcTextSize(text);
    canvas.DrawText(radar.GetCenter().At(-int(size.width) / 2,
                                         -int(size.height) / 2),
                    text);
    return;
  }

  /* keep the minimum range in view */
  max_range = std::max(max_range,
                       unsigned(FlarmMinimumRange(Angle::Zero())));

  const double step = RingStep(Units::ToUserDistance(max_range));
  const unsigned rings =
    unsigned(std::ceil(Units::ToUserDistance(max_range) / step));

  const double full_range = Units::ToSysDistance(step * rings);
  PaintSectors(canvas, full_range);
  PaintMinimum(canvas, full_range);
  PaintRings(canvas, step, rings);
  PaintLimit(canvas, full_range);
  PaintAircraft(canvas);
}
