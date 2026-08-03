// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gauge/GaugeVario.hpp"
#include "Computer/STF.hpp"
#include "Look/VarioLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Math/FastRotation.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "lib/fmt/ToBuffer.hxx"

#include <algorithm> // for std::clamp()
#include <array>
#include <cstdlib> // for std::abs()

static constexpr double DELTA_V_STEP = 4.0;
static constexpr double DELTA_V_LIMIT = 16.0;
#define TEXT_BUG "Bug"
#define TEXT_BALLAST "Bal"

inline
GaugeVario::BallastGeometry::BallastGeometry(const VarioLook &look,
                                             const PixelRect &rc) noexcept
{
  PixelSize tSize;

  // position of ballast label
  label_pos.x = 1;
  label_pos.y = rc.top + 2
    + look.label_font.GetCapitalHeight() * 2
    - look.label_font.GetAscentHeight();

  // position of ballast value
  value_pos.x = 1;
  value_pos.y = rc.top + 1
    + look.label_font.GetCapitalHeight()
    - look.label_font.GetAscentHeight();

  // set upper left corner
  label_rect.left = label_pos.x;
  label_rect.top = label_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();

  // set upper left corner
  value_rect.left = value_pos.x;
  value_rect.top = value_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();

  // get max label size
  tSize = look.label_font.TextSize(TEXT_BALLAST);

  // update back rect with max label size
  label_rect.right = label_rect.left + tSize.width;
  label_rect.bottom = label_rect.top +
    look.label_font.GetCapitalHeight();

  // get max value size
  tSize = look.label_font.TextSize("100%");

  value_rect.right = value_rect.left + tSize.width;
  // update back rect with max label size
  value_rect.bottom = value_rect.top +
    look.label_font.GetCapitalHeight();
}

inline
GaugeVario::BugsGeometry::BugsGeometry(const VarioLook &look,
                                       const PixelRect &rc) noexcept
{
  PixelSize tSize;

  label_pos.x = 1;
  label_pos.y = rc.bottom - 2
    - look.label_font.GetCapitalHeight()
    - look.label_font.GetAscentHeight();

  value_pos.x = 1;
  value_pos.y = rc.bottom - 1
    - look.label_font.GetAscentHeight();

  label_rect.left = label_pos.x;
  label_rect.top = label_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();
  value_rect.left = value_pos.x;
  value_rect.top = value_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();

  tSize = look.label_font.TextSize(TEXT_BUG);

  label_rect.right = label_rect.left + tSize.width;
  label_rect.bottom = label_rect.top
    + look.label_font.GetCapitalHeight()
    + look.label_font.GetHeight()
    - look.label_font.GetAscentHeight();

  tSize = look.label_font.TextSize("100%");

  value_rect.right = value_rect.left + tSize.width;
  value_rect.bottom = value_rect.top +
    look.label_font.GetCapitalHeight();
}

inline
GaugeVario::LabelValueGeometry::LabelValueGeometry(const VarioLook &look,
                                                   PixelPoint position) noexcept
  :label_right(position.x),
   label_top(position.y + Layout::Scale(1)),
   label_bottom(label_top + look.label_font.GetCapitalHeight()),
   label_y(label_top + look.label_font.GetCapitalHeight()
           - look.label_font.GetAscentHeight()),
   // TODO: update after units got reconfigured?
   value_right(position.x - UnitSymbolRenderer::GetSize(look.unit_font,
                                                        Units::current.vertical_speed_unit).width),
   value_top(label_bottom + Layout::Scale(2)),
   value_bottom(value_top + look.value_font.GetCapitalHeight()),
   value_y(value_top + look.value_font.GetCapitalHeight()
           - look.value_font.GetAscentHeight())
{
}

inline unsigned
GaugeVario::LabelValueGeometry::GetHeight(const VarioLook &look) noexcept
{
  return Layout::Scale(4) + look.value_font.GetCapitalHeight()
    + look.label_font.GetCapitalHeight();
}

inline
GaugeVario::Geometry::Geometry(const VarioLook &look, const PixelRect &rc) noexcept
  :ballast(look, rc), bugs(look, rc)
{
  // needle sits between minor and major tick:
  // tip at inner end of minor tick, base at inner end of major tick
  const int tick_minor = Layout::GetTextPadding() * 4;
  const int tick_major = Layout::GetTextPadding() * 8;
  nlength1 = tick_minor; // tip = inner end of minor tick
  nlength0 = tick_major; // base = inner end of major tick (needle length = tick_minor)
  nwidth = Layout::GetTextPadding() * 2;
  nline = Layout::Scale(8);

  offset = rc.GetMiddleRight();
  offset.x -= Layout::GetTextPadding();

  const PixelSize value_offset{0u, LabelValueGeometry::GetHeight(look)};

  const PixelPoint gross_position = offset - value_offset / 2u;
  gross = {look, gross_position};
  average = {look, gross_position - value_offset};
  mc = {look, gross_position + value_offset};
}

GaugeVario::GaugeVario(const FullBlackboard &_blackboard,
                       ContainerWindow &parent, const VarioLook &_look,
                       PixelRect rc, const WindowStyle style) noexcept
  :blackboard(_blackboard), look(_look)
{
  Create(parent, rc, style);
}

static constexpr int
WidthToHeight(int width) noexcept
{
  return width * 112 / 100;
}

static constexpr PixelPoint
TransformRotatedPoint(IntPoint2D pt, IntPoint2D offset) noexcept
{
  return { pt.x + offset.x, WidthToHeight(pt.y) + offset.y + 1 };
}

void
GaugeVario::RenderVarioBar(Canvas &canvas, const PixelRect &rc, int ival) noexcept
{
  if (ival == 0) return;

  const int x_radius = rc.GetWidth() - Layout::GetTextPadding();
  const int bar_width = Layout::GetTextPadding() * 4; // same as tick_length_minor

  const int angle_min = std::min(0, ival);
  const int angle_max = std::max(0, ival);
  // ring sector: outer arc (radius x_radius) forward + inner arc (radius x_radius - bar_width) backward
  std::array<BulkPixelPoint, 400> poly;
  std::size_t n = 0;

  // outer arc: angle_min to angle_max
  for (int i = angle_min; i <= angle_max; ++i) {
    const FastIntegerRotation r{Angle::Degrees(i)};
    const auto pt = TransformRotatedPoint(r.Rotate({-x_radius, 0}), geometry.offset);
    poly[n++] = BulkPixelPoint{pt.x, pt.y};
  }
  // inner arc: angle_max back to angle_min
  for (int i = angle_max; i >= angle_min; --i) {
    const FastIntegerRotation r{Angle::Degrees(i)};
    const auto pt = TransformRotatedPoint(r.Rotate({-x_radius + bar_width, 0}), geometry.offset);
    poly[n++] = BulkPixelPoint{pt.x, pt.y};
  }

  canvas.SelectNullPen();
  canvas.Select(ival > 0 ? look.lift_brush : look.sink_brush);
  canvas.DrawPolygon(poly.data(), n);
}

inline void
GaugeVario::RenderBackground(Canvas &canvas, const PixelRect &rc) noexcept
{

  const int x_radius = rc.GetWidth() - Layout::GetTextPadding();

  canvas.Select(look.arc_label_font);
  canvas.SetTextColor(look.dimmed_text_color);
  canvas.SetBackgroundTransparent();

  const auto &unit_descriptor =
    Units::unit_descriptors[(std::size_t)Units::current.vertical_speed_unit];
  const double unit_factor = unit_descriptor.factor_to_user;

  // major_step in user units; minor ticks at half that interval
  int major_step;
  switch (Units::current.vertical_speed_unit) {
  case Unit::FEET_PER_MINUTE:
    major_step = 200;
    break;
  case Unit::KNOTS:
    major_step = 2;
    break;
  default: // m/s
    major_step = 1;
    break;
  }

  const Angle major_angle_step = Angle::QuarterCircle() * major_step
    / unit_factor / GAUGEVARIORANGE;
  const Angle minor_angle_step = major_angle_step / 2;

  const int n_major = GAUGEVARIORANGE / (major_step / unit_factor);

  const int tick_length_major = Layout::GetTextPadding() * 8;
  const int tick_length_minor = Layout::GetTextPadding() * 4;

  // ticks start at the outer edge (where arc was) and go inward
  const IntPoint2D major_outer{-x_radius, 0};
  const IntPoint2D major_inner{-x_radius + tick_length_major, 0};
  const IntPoint2D minor_outer{-x_radius, 0};
  const IntPoint2D minor_inner{-x_radius + tick_length_minor, 0};
  const IntPoint2D label_center{-x_radius + tick_length_major + (int)Layout::GetTextPadding() * 3, 0};

  // draw minor (half-step) ticks
  canvas.Select(look.half_tick_pen);
  for (int i = -n_major; i < n_major; ++i) {
    const Angle angle = major_angle_step * i + minor_angle_step;
    const FastIntegerRotation r{angle};
    canvas.DrawLine(TransformRotatedPoint(r.Rotate(minor_outer), geometry.offset),
                    TransformRotatedPoint(r.Rotate(minor_inner), geometry.offset));
  }

  // draw major ticks as filled rectangles (sharp square ends), including extremes
  const int major_half_w = 1; // 2px total width, sharp rectangular tick
  canvas.SelectNullPen();
  canvas.SelectBlackBrush();
  if (look.inverse)
    canvas.SelectWhiteBrush();
  for (int i = -n_major; i <= n_major; ++i) {
    const Angle angle = major_angle_step * i;
    const FastIntegerRotation r{angle};

    const IntPoint2D p0{-x_radius,                  -major_half_w};
    const IntPoint2D p1{-x_radius + tick_length_major, -major_half_w};
    const IntPoint2D p2{-x_radius + tick_length_major,  major_half_w};
    const IntPoint2D p3{-x_radius,                   major_half_w};

    const BulkPixelPoint rect[4] = {
      TransformRotatedPoint(r.Rotate(p0), geometry.offset),
      TransformRotatedPoint(r.Rotate(p1), geometry.offset),
      TransformRotatedPoint(r.Rotate(p2), geometry.offset),
      TransformRotatedPoint(r.Rotate(p3), geometry.offset),
    };
    canvas.DrawPolygon(rect, 4);

    char label[16];
    StringFormatUnsafe(label, "%d", std::abs(i * major_step));

    const auto label_size = canvas.CalcTextSize(label);
    const auto label_position = TransformRotatedPoint(r.Rotate(label_center),
                                                      geometry.offset)
      - label_size / 2U;

    canvas.DrawText(label_position, label);
  }
}

void
GaugeVario::OnPaintBuffer(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  auto vval = Basic().VarioOutputFilterActive()
    ? (Basic().brutto_vario_available
        ? Basic().FilteredBruttoVario()
        : 0.)
    : Basic().brutto_vario;

  if (!needle_initialised) {
    MakeAllPolygons();
    needle_initialised = true;
  }

  const int ival = ValueToNeedlePos(vval);

  int ival_av = 0;
  int ival_av_thermal = 0;
  if (Settings().show_thermal_average_needle)
    ival_av_thermal = ValueToNeedlePos(Calculated().current_thermal.lift_rate);
  if (Settings().show_average_needle) {
    ival_av = ValueToNeedlePos(!Calculated().circling
                               ? Calculated().netto_average
                               : Calculated().average);
  }

  canvas.Clear(look.background_color);

  // draw colored vario bar in the background (behind ticks)
  RenderVarioBar(canvas, rc, ival);

  // draw scale (ticks + labels) on top of bar
  RenderBackground(canvas, rc);

  // 4. draw UI overlays
  dirty = true; // force redraw of all value fields

  if (Settings().show_average) {
    // JMW averager now displays netto average if not circling
    RenderValue(canvas, geometry.average, average_di,
                Units::ToUserVSpeed(Calculated().circling ? Calculated().average : Calculated().netto_average),
                Calculated().circling ? "Avg" : "NetAvg");
  }

  if (Settings().show_mc) {
    auto mc = Units::ToUserVSpeed(GetGlidePolar().GetMC());
    RenderValue(canvas, geometry.mc, mc_di,
                mc,
                GetComputerSettings().task.auto_mc ? "Auto MC" : "MC");
  }

  if (Settings().show_speed_to_fly)
    RenderSpeedToFly(canvas, rc.right - 11, (rc.top + rc.bottom) / 2);
  else
    RenderClimb(canvas);

  if (Settings().show_ballast)
    RenderBallast(canvas);

  if (Settings().show_bugs)
    RenderBugs(canvas);

  if (Settings().show_gross) {
    auto vvaldisplay = std::clamp(Units::ToUserVSpeed(vval), -99.9, 99.9);
    RenderValue(canvas, geometry.gross, gross_di, vvaldisplay, "Gross");
  }

  // draw needle on top of everything
  if (Settings().show_average_needle)
    RenderNeedle(canvas, ival_av, true, false);

  RenderNeedle(canvas,
               Settings().show_thermal_average_needle ? ival_av_thermal : ival,
               false, false);

  dirty = false;
}

void
GaugeVario::MakePolygon(const int i) noexcept
{
  auto *bit = getPolygon(i);
  auto *bline = &lines[i + gmax];

  const FastIntegerRotation r(Angle::Degrees(i));

  bit[0] = TransformRotatedPoint(r.Rotate({-geometry.offset.x + geometry.nlength0, geometry.nwidth}),
                                 geometry.offset);
  bit[1] = TransformRotatedPoint(r.Rotate({-geometry.offset.x + geometry.nlength1, 0}),
                                 geometry.offset);
  bit[2] = TransformRotatedPoint(r.Rotate({-geometry.offset.x + geometry.nlength0, -geometry.nwidth}),
                                 geometry.offset);

  *bline = TransformRotatedPoint(r.Rotate({-geometry.offset.x + geometry.nline, 0}),
                                 geometry.offset);
}

inline BulkPixelPoint *
GaugeVario::getPolygon(int i) noexcept
{
  return polys + (i + gmax) * 3;
}

inline void
GaugeVario::MakeAllPolygons() noexcept
{
  for (int i = gmin; i <= gmax; i++)
    MakePolygon(i);
}

void
GaugeVario::RenderClimb(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();
  int x = rc.right - Layout::Scale(14);
  int y = rc.bottom - Layout::Scale(24);

  if (!dirty)
    return;

  const PixelSize dest_size{Layout::VptScale(9)};

  if (Basic().switch_state.flight_mode == SwitchState::FlightMode::CIRCLING)
    canvas.Stretch({x, y}, dest_size, look.climb_bitmap, {12, 0}, {12, 12});
  else if (IsPersistent())
    canvas.DrawFilledRectangle(PixelRect{{x, y}, dest_size},
                               look.background_color);
}

inline void
GaugeVario::RenderZero(Canvas &canvas) noexcept
{
  if (look.inverse)
    canvas.SelectWhitePen();
  else
    canvas.SelectBlackPen();

  canvas.DrawLine({0, geometry.offset.y},
                  {Layout::Scale(17), geometry.offset.y});
  canvas.DrawLine({0, geometry.offset.y + 1},
                  {Layout::Scale(17), geometry.offset.y + 1});
}

int
GaugeVario::ValueToNeedlePos(double Value) noexcept
{
  constexpr double degrees_per_unit =
    double(GAUGEVARIOSWEEP) / GAUGEVARIORANGE;

  int i;

  if (!needle_initialised){
    MakeAllPolygons();
    needle_initialised = true;
  }


  i = iround(Value * degrees_per_unit);
  i = std::clamp(i, int(gmin), int(gmax));
  return i;
}

void
GaugeVario::RenderVarioLine(Canvas &canvas, int i, int sink,
                            bool clear) noexcept
{
  dirty = true;
  if (i == sink)
    return;

  canvas.Select(clear
                ? look.thick_background_pen
                : (i > sink ? look.thick_lift_pen : look.thick_sink_pen));

  if (i > sink)
    canvas.DrawPolyline(lines + gmax + sink, i - sink);
  else
    canvas.DrawPolyline(lines + gmax + i, sink - i);

  if (!clear) {
    canvas.SelectNullPen();

    // clear up naked (sink) edge of polygon, this gives it a nice
    // taper look
    if (look.inverse) {
      canvas.SelectBlackBrush();
    } else {
      canvas.SelectWhiteBrush();
    }
    canvas.DrawTriangleFan(getPolygon(sink), 3);
  }
}

void
GaugeVario::RenderNeedle(Canvas &canvas, int i, bool average,
                         bool clear) noexcept
{
  dirty = true;

  canvas.SelectNullPen();

  // legacy behaviour
  if (clear ^ look.inverse) {
    canvas.SelectWhiteBrush();
    canvas.SelectWhitePen();
  } else {
    canvas.SelectBlackBrush();
    canvas.SelectBlackPen();
  }

  if (average)
    canvas.DrawPolyline(getPolygon(i), 3);
  else
    canvas.DrawTriangleFan(getPolygon(i), 3);
}

// TODO code: Optimise vario rendering, this is slow
void
GaugeVario::RenderValue(Canvas &canvas, const LabelValueGeometry &g,
                        LabelValueDrawInfo &di,
                        double value, const char *label) noexcept
{
  value = (double)iround(value * 10) / 10; // prevent the -0.0 case

  canvas.SetBackgroundTransparent();

  if (!IsPersistent() || (dirty && !StringIsEqual(di.label.last_text, label))) {
    canvas.SetTextColor(look.dimmed_text_color);
    canvas.Select(look.label_font);
    const unsigned width = canvas.CalcTextSize(label).width;

    const PixelPoint text_position{g.label_right - (int)width, g.label_y};

    if (IsPersistent()) {
      PixelRect rc;
      rc.left = text_position.x;
      rc.top = g.label_top;
      rc.right = g.label_right;
      rc.bottom = g.label_bottom;

      canvas.SetBackgroundColor(look.background_color);
      canvas.DrawOpaqueText(text_position, rc, label);
      di.label.last_width = width;
      strcpy(di.label.last_text, label);
    } else {
      canvas.DrawText(text_position, label);
    }
  }

  if (!IsPersistent() || (dirty && di.value.last_value != value)) {
    const auto buffer = FmtBuffer<18>("{:.1f}", value);
    canvas.SetBackgroundColor(look.background_color);
    canvas.SetTextColor(look.text_color);
    canvas.Select(look.value_font);
    const unsigned width = canvas.CalcTextSize(buffer.c_str()).width;

    const PixelPoint text_position{g.value_right - (int)width, g.value_y};

    if (IsPersistent()) {
      PixelRect rc;
      rc.left = text_position.x;
      rc.top = g.value_top;
      rc.right = g.value_right;
      rc.bottom = g.value_bottom;

      canvas.DrawOpaqueText(text_position, rc, buffer.c_str());

      di.value.last_width = width;
      di.value.last_value = value;
    } else {
      canvas.DrawText(text_position, buffer.c_str());
    }
  }

  if (!IsPersistent() ||
      di.value.last_unit != Units::current.vertical_speed_unit) {
    auto unit = di.value.last_unit = Units::current.vertical_speed_unit;

    const int ascent_height = look.value_font.GetAscentHeight();
    const int unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.unit_font, unit);

    canvas.Select(look.unit_font);
    canvas.SetTextColor(COLOR_GRAY);
    UnitSymbolRenderer::Draw(canvas,
                             PixelPoint(g.value_right,
                                        g.value_y + ascent_height - unit_height),
                             unit, look.unit_fraction_pen);
  }
}

inline void
GaugeVario::RenderSpeedToFly(Canvas &canvas, int x, int y) noexcept
{
  const auto stf = GetSTFSpeed(Basic(), Calculated());
  if (!stf || !Basic().airspeed_available ||
      !Basic().total_energy_vario_available)
    return;

  double v_diff;

  const unsigned arrow_y_size = Layout::Scale(3);
  const unsigned arrow_x_size = Layout::Scale(7);

  const PixelRect rc = GetClientRect();

  int nary = NARROWS * arrow_y_size;
  int ytop = rc.top + YOFFSET + nary; // JMW
  int ybottom = rc.bottom - YOFFSET - nary - Layout::FastScale(1);

  ytop += Layout::Scale(14);
  ybottom -= Layout::Scale(14);

  x = rc.right - 2 * arrow_x_size;

  // only draw speed command if flying and vario is not circling
  if ((Calculated().flight.flying)
      && (!Basic().gps.simulator || !Calculated().circling)) {
    /* V_stf is TAS (density-compensated); compare to actual TAS */
    v_diff = *stf - Basic().true_airspeed;
    v_diff = std::clamp(v_diff, -DELTA_V_LIMIT, DELTA_V_LIMIT); // limit it
    v_diff = iround(v_diff/DELTA_V_STEP) * DELTA_V_STEP;
  } else
    v_diff = 0;

  if (!IsPersistent() || last_v_diff != v_diff || dirty) {
    last_v_diff = v_diff;

    if (IsPersistent()) {
      const unsigned height = nary + arrow_y_size + Layout::FastScale(2);

      const PixelSize size{arrow_x_size * 2 + 1, height};

      // bottom (too slow)
      canvas.DrawFilledRectangle({{x, ybottom + YOFFSET}, size},
                                 look.background_color);

      // top (too fast)
      canvas.DrawFilledRectangle({{x, ytop - YOFFSET + 1 - (int)height}, size},
                                 look.background_color);
    }

    RenderClimb(canvas);

    canvas.SelectNullPen();

    if (look.colors) {
      if (v_diff > 0) {
        // too slow
        canvas.Select(look.sink_brush);
      } else {
        canvas.Select(look.lift_brush);
      }
    } else {
      if (look.inverse)
        canvas.SelectWhiteBrush();
      else
        canvas.SelectBlackBrush();
    }

    if (v_diff > 0) {
      // too slow
      y = ybottom;
      y += YOFFSET;

      const PixelSize size{arrow_x_size * 2 + 1, arrow_y_size - 1};
      while (v_diff > 0) {
        if (v_diff > DELTA_V_STEP) {
          canvas.DrawRectangle({{x, y}, size});
        } else {
          BulkPixelPoint arrow[3];
          arrow[0].x = x;
          arrow[0].y = y;
          arrow[1].x = x + arrow_x_size;
          arrow[1].y = y + arrow_y_size - 1;
          arrow[2].x = x + 2 * arrow_x_size;
          arrow[2].y = y;
          canvas.DrawTriangleFan(arrow, 3);
        }
        v_diff -= DELTA_V_STEP;
        y += arrow_y_size;
      }
    } else if (v_diff < 0) {
      // too fast
      y = ytop;
      y -= YOFFSET;

      const PixelSize size{arrow_x_size * 2 + 1, y - arrow_y_size + 1};
      while (v_diff < 0) {
        if (v_diff < -DELTA_V_STEP) {
          canvas.DrawRectangle({{x, y + 1}, size});
        } else {
          BulkPixelPoint arrow[3];
          arrow[0].x = x;
          arrow[0].y = y;
          arrow[1].x = x + arrow_x_size;
          arrow[1].y = y - arrow_y_size + 1;
          arrow[2].x = x + 2 * arrow_x_size;
          arrow[2].y = y;
          canvas.DrawTriangleFan(arrow, 3);
        }
        v_diff += DELTA_V_STEP;
        y -= arrow_y_size;
      }
    }
  }
}

inline void
GaugeVario::RenderBallast(Canvas &canvas) noexcept
{
  const GlidePolar &polar = GetGlidePolar();
  const double ballast_fraction = polar.GetBallastFraction();
  int ballast = iround(ballast_fraction * 100);

  if (!IsPersistent() || ballast != last_ballast) {
    // ballast hase been changed

    canvas.Select(look.label_font);

    if (IsPersistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    const auto &g = geometry.ballast;

    if (IsPersistent() || last_ballast < 1 || ballast < 1) {
      // new ballast is 0, hide label
      if (ballast > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        // ols ballast was 0, show label
        if (IsPersistent())
          canvas.DrawOpaqueText(g.label_pos, g.label_rect, TEXT_BALLAST);
        else
          canvas.DrawText(g.label_pos, TEXT_BALLAST);
      } else if (IsPersistent())
        canvas.DrawFilledRectangle(g.label_rect, look.background_color);
    }

    // new ballast 0, hide value
    if (ballast > 0) {
      const auto buffer = FmtBuffer<18>("{}%", ballast);
      canvas.SetTextColor(look.text_color);

      if (IsPersistent())
        canvas.DrawOpaqueText(g.value_pos, g.value_rect, buffer.c_str());
      else
        canvas.DrawText(g.value_pos, buffer.c_str());
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(g.value_rect, look.background_color);

    if (IsPersistent())
      last_ballast = ballast;
  }
}

inline void
GaugeVario::RenderBugs(Canvas &canvas) noexcept
{
  int bugs = iround((1 - GetComputerSettings().polar.bugs) * 100);
  if (!IsPersistent() || bugs != last_bugs) {

    canvas.Select(look.label_font);

    if (IsPersistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    const auto &g = geometry.bugs;

    if (IsPersistent() || last_bugs < 1 || bugs < 1) {
      if (bugs > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        if (IsPersistent())
          canvas.DrawOpaqueText(g.label_pos, g.label_rect, TEXT_BUG);
        else
          canvas.DrawText(g.label_pos, TEXT_BUG);
      } else if (IsPersistent())
        canvas.DrawFilledRectangle(g.label_rect, look.background_color);
    }

    if (bugs > 0) {
      const auto buffer = FmtBuffer<18>("{}%", bugs);
      canvas.SetTextColor(look.text_color);
      if (IsPersistent())
        canvas.DrawOpaqueText(g.value_pos, g.value_rect, buffer.c_str());
      else
        canvas.DrawText(g.value_pos, buffer.c_str());
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(g.value_rect, look.background_color);

    if (IsPersistent())
      last_bugs = bugs;
  }
}

void
GaugeVario::OnResize(PixelSize new_size) noexcept
{
  AntiFlickerWindow::OnResize(new_size);

  geometry = {look, GetClientRect()};

  /* trigger reinitialisation */
  background_dirty = true;
  needle_initialised = false;

  average_di.Reset();
  mc_di.Reset();
  gross_di.Reset();
}
