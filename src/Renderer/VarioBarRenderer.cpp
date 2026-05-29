// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioBarRenderer.hpp"
#include "TextInBox.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Look/VarioBarLook.hpp"
#include "Formatter/UserUnits.hpp"

#include <algorithm>
#include <cmath>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

void
VarioBarRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                            const MoreData &basic,
                            const DerivedInfo &calculated,
                            const GlidePolar &glide_polar,
                            const bool vario_bar_avg_enabled) const
{
#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif

  BulkPixelPoint VarioBar[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };
  BulkPixelPoint VarioBarAvg[4] = {
      { 0, 0 }, { 9, -9 }, { 9, 0 }, { 0, 0 }
  };
  BulkPixelPoint clipping_arrow[6] = {
      { 0, 0 }, { 9, 9 }, { 18, 0 }, { 18, 6 }, { 9, 15 }, { 0, 6 }
  };
  BulkPixelPoint clipping_arrow_av[4] = {
      { 0, 0 }, { 9, 9 }, { 9, 15 }, { 0, 6 }
  };
  BulkPixelPoint mc_arrow[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, -2 }, { 9, -11 }, { 0, -2 }
  };

  char Value[10];

  const int y0 = (rc.bottom + rc.top) / 2;

  /* NOTE: size_divisor replaces the fixed value 9 that was used throughout
   * the code sink which caused fixed size rendering regardless of
   * map size (except the effects of Layout::Scale()). This method
   * is not usable with variable map sizes (e.g. because of the cross-section
   * area). size_divisor is used to introduce a screen size dependent scaling.
   * That workaround is an ugly hack and needs a rework. */
  const auto size_divisor =
    std::max((double) Layout::Scale(70u / rc.GetHeight()), 0.15);

  int dy_variobar = 0;
  int dy_variobar_av = 0;
  int dy_variobar_mc = 0;

  int clipping_arrow_offset = Layout::Scale(4);
  int clipping_arrow_av_offset = Layout::Scale(4);
//  int clipping_arrow_mc_offset = Layout::Scale(4);

  auto vario_gross = basic.brutto_vario;

  FormatUserVerticalSpeed(vario_gross, Value, false, true);
  canvas.Select(*look.font);
  const PixelSize text_size = canvas.CalcTextSize(Value);

  auto vario_avg = calculated.average;

  // cut vario_gross at +- 5 meters/s
  if (vario_gross > 5)
    vario_gross = 5;
  if (vario_gross < -5)
    vario_gross = -5;

  int Offset = (int)(vario_gross / size_divisor);

  Offset = Layout::Scale(Offset);
  if (vario_gross <= 0) {
    VarioBar[1].y = Layout::Scale(9);
    dy_variobar = text_size.height + 2;
  } else {
    VarioBar[1].y = -Layout::Scale(9);
    clipping_arrow[1].y = -clipping_arrow[1].y;
    clipping_arrow[3].y = -clipping_arrow[3].y;
    clipping_arrow[4].y = -clipping_arrow[4].y;
    clipping_arrow[5].y = -clipping_arrow[5].y;
    clipping_arrow_offset = -clipping_arrow_offset;
    dy_variobar = -1;
  }

  // cut vario_avg at +- 5 meters/s
  if (vario_avg > 5)
    vario_avg = 5;
  if (vario_avg < -5)
    vario_avg = -5;

  int OffsetAvg = int(vario_avg / size_divisor);

  OffsetAvg = Layout::Scale(OffsetAvg);
  if (vario_avg <= 0) {
    VarioBarAvg[1].y = Layout::Scale(9);
    dy_variobar_av = text_size.height + 2;
  } else {
    VarioBarAvg[1].y = -Layout::Scale(9);
    clipping_arrow_av[1].y = -clipping_arrow_av[1].y;
    clipping_arrow_av[2].y = -clipping_arrow_av[2].y;
    clipping_arrow_av[3].y = -clipping_arrow_av[3].y;
    clipping_arrow_av_offset = -clipping_arrow_av_offset;
    dy_variobar_av = -1;
  }


  //clip MC Value
  auto mc_val = glide_polar.GetMC();
  if (mc_val > 5)
    mc_val = 5;
  if (mc_val <= 0) {
    dy_variobar_mc = text_size.height + 2;
  }else{
    dy_variobar_mc = -1;
  }

  int OffsetMC = int(mc_val / size_divisor);
  OffsetMC = Layout::Scale(OffsetMC);

  for (unsigned i = 0; i < 6; i++) {
    VarioBar[i].y += y0 + dy_variobar;
    VarioBar[i].x = rc.right - Layout::Scale(VarioBar[i].x);
  }

  VarioBar[0].y -= Offset;
  VarioBar[1].y -= Offset;
  VarioBar[2].y -= Offset;

  for (unsigned i = 0; i < 4; i++) {
    VarioBarAvg[i].y += y0 + dy_variobar_av;
    VarioBarAvg[i].x = rc.right-Layout::Scale(VarioBarAvg[i].x);
  }

  VarioBarAvg[0].y -= OffsetAvg;
  VarioBarAvg[1].y -= OffsetAvg;

  // prepare mc arrow
  for (unsigned i = 0; i < 6; i++) {
    mc_arrow[i].y = Layout::Scale(mc_arrow[i].y) + y0
      - OffsetMC + dy_variobar_mc;
    mc_arrow[i].x = rc.right - Layout::Scale(mc_arrow[i].x);
  }

  // prepare clipping arrow
  for (unsigned i = 0; i < 6; i++) {
    clipping_arrow[i].y = Layout::Scale(clipping_arrow[i].y) + y0 - Offset
      + clipping_arrow_offset + dy_variobar;
    clipping_arrow[i].x = rc.right - Layout::Scale(clipping_arrow[i].x);
  }

  // prepare clipping avg
  for (unsigned i = 0; i < 4; i++) {
    clipping_arrow_av[i].y = Layout::Scale(clipping_arrow_av[i].y) + y0 - OffsetAvg
      + clipping_arrow_av_offset + dy_variobar_av;
    clipping_arrow_av[i].x = rc.right-Layout::Scale(clipping_arrow_av[i].x);
  }

  // draw actual vario bar
  if (vario_gross <= 0) {
    canvas.Select(look.pen_sink);
    canvas.Select(look.brush_sink);
  } else {
    canvas.Select(look.pen_climb);
    canvas.Select(look.brush_climb);
  }

  canvas.DrawPolygon(VarioBar, 6);

  // draw clipping arrow
  if (vario_gross <= -5 || vario_gross >= 5)
    canvas.DrawPolygon(clipping_arrow, 6);

  // draw avg vario bar
  if (vario_avg <= 0 && vario_bar_avg_enabled) {
      canvas.Select(look.pen_sink);
      canvas.Select(look.brush_sink_avg);
  } else {
    canvas.Select(look.pen_climb);
    canvas.Select(look.brush_climb_avg);
  }

  canvas.DrawPolygon(VarioBarAvg, 4);

  if (vario_avg <= -5.0 || vario_avg >= 5.0)
      canvas.DrawPolygon(clipping_arrow_av, 4);

  //draw MC arrow
  canvas.Select(look.pen_mc);
  canvas.Select(look.brush_mc);

  canvas.DrawPolygon(mc_arrow, 6);

  //draw text
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);

  TextInBoxMode style;
  style.shape = LabelShape::ROUNDED_BLACK;
  style.move_in_view = true;

  if (text_size.width < Layout::Scale(18u)) {
    style.align = TextInBoxMode::Alignment::RIGHT;
    TextInBox(canvas, Value, {rc.right, y0}, style, rc);
  } else
    TextInBox(canvas, Value, {rc.right - Layout::Scale(18), y0}, style, rc);
}

void
VarioBarRenderer::DrawSpeedToFly(Canvas &canvas, const PixelRect &rc,
                                 const MoreData &basic,
                                 const DerivedInfo &calculated,
                                 unsigned max_bar_half_length,
                                 Color pull_color, Color push_color) const
{
  if (!basic.airspeed_available || !calculated.flight.flying)
    return;

  const unsigned min_w = Layout::Scale(4u);
  const unsigned min_h = Layout::Scale(8u);
  if (rc.GetWidth() < min_w || rc.GetHeight() < min_h)
    return;

  double speed_cmd = basic.indicated_airspeed - calculated.V_stf;
  speed_cmd = std::clamp(speed_cmd, -5.0, 5.0);

  const int cx = (rc.left + rc.right) / 2;
  const int y0 = (rc.top + rc.bottom) / 2;

  const int tri_h = Layout::Scale(5);
  const int bar_half_w = std::max(1, int(rc.GetWidth()) / 2);
  const int tri_half_w = bar_half_w;
  const int center_tick = std::max(2, int(rc.GetHeight()) / 12);

  const int max_half = max_bar_half_length > 0
    ? int(max_bar_half_length)
    : int(rc.GetHeight()) / 2;
  const int max_shaft = std::max(0, max_half - tri_h - center_tick / 2);

  canvas.Select(look.pen_mc);
  canvas.DrawLine({cx, y0 - center_tick}, {cx, y0 + center_tick});

  if (std::abs(speed_cmd) < 0.05)
    return;

  const double magnitude = std::abs(speed_cmd) / 5.0;
  const int bar_length = std::max(0, int(max_shaft * magnitude + 0.5));
  if (bar_length < 1)
    return;

  const bool pull_up = speed_cmd > 0;
  const Color fill = pull_up ? pull_color : push_color;
  canvas.Select(Brush(fill));
  canvas.Select(Pen(Layout::ScaleFinePenWidth(1), fill));

  if (pull_up) {
    const int shaft_top = y0 - bar_length;
    canvas.DrawFilledRectangle({
      cx - bar_half_w,
      shaft_top,
      cx + bar_half_w,
      y0,
    }, fill);

    const BulkPixelPoint tri[] = {
      { cx - tri_half_w, shaft_top },
      { cx, shaft_top - tri_h },
      { cx + tri_half_w, shaft_top },
    };
    canvas.DrawPolygon(tri, 3);
  } else {
    const int shaft_bottom = y0 + bar_length;
    canvas.DrawFilledRectangle({
      cx - bar_half_w,
      y0,
      cx + bar_half_w,
      shaft_bottom,
    }, fill);

    const BulkPixelPoint tri[] = {
      { cx - tri_half_w, shaft_bottom },
      { cx, shaft_bottom + tri_h },
      { cx + tri_half_w, shaft_bottom },
    };
    canvas.DrawPolygon(tri, 3);
  }
}
