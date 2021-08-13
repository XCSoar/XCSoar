/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Gauge/GaugeVario.hpp"
#include "Look/VarioLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Math/FastRotation.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "util/Clamp.hpp"

#define DELTA_V_STEP 4.
#define DELTA_V_LIMIT 16.
#define TEXT_BUG _T("Bug")
#define TEXT_BALLAST _T("Bal")

inline
GaugeVario::BallastGeometry::BallastGeometry(const VarioLook &look,
                                             const PixelRect &rc) noexcept
{
  PixelSize tSize;

  // position of ballast label
  label_pos.x = 1;
  label_pos.y = rc.top + 2
    + look.text_font->GetCapitalHeight() * 2
    - look.text_font->GetAscentHeight();

  // position of ballast value
  value_pos.x = 1;
  value_pos.y = rc.top + 1
    + look.text_font->GetCapitalHeight()
    - look.text_font->GetAscentHeight();

  // set upper left corner
  label_rect.left = label_pos.x;
  label_rect.top = label_pos.y
    + look.text_font->GetAscentHeight()
    - look.text_font->GetCapitalHeight();

  // set upper left corner
  value_rect.left = value_pos.x;
  value_rect.top = value_pos.y
    + look.text_font->GetAscentHeight()
    - look.text_font->GetCapitalHeight();

  // get max label size
  tSize = look.text_font->TextSize(TEXT_BALLAST);

  // update back rect with max label size
  label_rect.right = label_rect.left + tSize.width;
  label_rect.bottom = label_rect.top +
    look.text_font->GetCapitalHeight();

  // get max value size
  tSize = look.text_font->TextSize(_T("100%"));

  value_rect.right = value_rect.left + tSize.width;
  // update back rect with max label size
  value_rect.bottom = value_rect.top +
    look.text_font->GetCapitalHeight();
}

inline
GaugeVario::BugsGeometry::BugsGeometry(const VarioLook &look,
                                       const PixelRect &rc) noexcept
{
  PixelSize tSize;

  label_pos.x = 1;
  label_pos.y = rc.bottom - 2
    - look.text_font->GetCapitalHeight()
    - look.text_font->GetAscentHeight();

  value_pos.x = 1;
  value_pos.y = rc.bottom - 1
    - look.text_font->GetAscentHeight();

  label_rect.left = label_pos.x;
  label_rect.top = label_pos.y
    + look.text_font->GetAscentHeight()
    - look.text_font->GetCapitalHeight();
  value_rect.left = value_pos.x;
  value_rect.top = value_pos.y
    + look.text_font->GetAscentHeight()
    - look.text_font->GetCapitalHeight();

  tSize = look.text_font->TextSize(TEXT_BUG);

  label_rect.right = label_rect.left + tSize.width;
  label_rect.bottom = label_rect.top
    + look.text_font->GetCapitalHeight()
    + look.text_font->GetHeight()
    - look.text_font->GetAscentHeight();

  tSize = look.text_font->TextSize(_T("100%"));

  value_rect.right = value_rect.left + tSize.width;
  value_rect.bottom = value_rect.top +
    look.text_font->GetCapitalHeight();
}

inline
GaugeVario::LabelValueGeometry::LabelValueGeometry(const VarioLook &look,
                                                   PixelPoint position) noexcept
  :label_right(position.x),
   label_top(position.y + Layout::Scale(1)),
   label_bottom(label_top + look.text_font->GetCapitalHeight()),
   label_y(label_top + look.text_font->GetCapitalHeight()
           - look.text_font->GetAscentHeight()),
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
    + look.text_font->GetCapitalHeight();
}

inline
GaugeVario::Geometry::Geometry(const VarioLook &look, const PixelRect &rc) noexcept
  :ballast(look, rc), bugs(look, rc)
{
  nlength0 = Layout::Scale(15);
  nlength1 = Layout::Scale(6);
  nwidth = Layout::Scale(4);
  nline = Layout::Scale(8);

  offset = rc.GetMiddleRight();

  const PixelSize value_offset{0u, LabelValueGeometry::GetHeight(look)};

  const PixelPoint gross_position = offset + value_offset / 2u;
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

inline void
GaugeVario::RenderBackground(Canvas &canvas, const PixelRect &rc) noexcept
{
  canvas.Clear(look.background_color);

  canvas.Select(look.arc_pen);

  std::array<BulkPixelPoint, 21> arc;

  const int arc_padding = look.arc_label_font.GetHeight();

  const int x_radius = rc.GetWidth() - arc_padding;
  const int y_radius = WidthToHeight(x_radius);

  for (std::size_t i = 0; i < arc.size(); ++i) {
    const unsigned angle = INT_ANGLE_RANGE / 2
      + i * INT_ANGLE_RANGE / 2 / (arc.size() - 1);
    const PixelPoint delta(ISINETABLE[NormalizeIntAngle(angle)] * x_radius / 1024,
                           -ISINETABLE[NormalizeIntAngle(angle + INT_QUARTER_CIRCLE)] * y_radius / 1024);

    arc[i] = geometry.offset + delta;
  }

  canvas.DrawPolyline(arc.data(), arc.size());

  canvas.Select(look.tick_pen);
  canvas.Select(look.arc_label_font);
  canvas.SetTextColor(look.dimmed_text_color);
  canvas.SetBackgroundTransparent();

  int tick_value_step = 1;
  const auto &unit_descriptor =
    Units::unit_descriptors[(std::size_t)Units::current.vertical_speed_unit];
  const double unit_factor = unit_descriptor.factor_to_user;

  switch (Units::current.vertical_speed_unit) {
  case Unit::FEET_PER_MINUTE:
    tick_value_step = 200;
    break;

  default:
    break;
  }

  const Angle tick_angle_step = Angle::QuarterCircle() * tick_value_step
    / unit_factor / GAUGEVARIORANGE;

  const int n_ticks = GAUGEVARIORANGE / (tick_value_step / unit_factor);

  const int tick_length = Layout::GetTextPadding() * 4;

  const IntPoint2D tick_start{1 - x_radius, 0};
  const IntPoint2D tick_end{-tick_length - x_radius, 0};
  const IntPoint2D label_center{-x_radius - arc_padding / 2 - tick_length, 0};

  for (int i = -n_ticks; i < n_ticks; ++i) {
    Angle angle = tick_angle_step * i;
    const FastIntegerRotation r{angle};

    canvas.DrawLine(TransformRotatedPoint(r.Rotate(tick_start),
                                          geometry.offset),
                    TransformRotatedPoint(r.Rotate(tick_end),
                                          geometry.offset));

    TCHAR label[16];
    StringFormatUnsafe(label, _T("%d"), i * tick_value_step);

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

  if (!IsPersistent() || background_dirty) {
    RenderBackground(canvas, rc);
    background_dirty = false;
  }

  if (Settings().show_average) {
    // JMW averager now displays netto average if not circling
    RenderValue(canvas, geometry.average, average_di,
                Units::ToUserVSpeed(Calculated().circling ? Calculated().average : Calculated().netto_average),
                Calculated().circling ? _T("Avg") : _T("NetAvg"));
  }

  if (Settings().show_mc) {
    auto mc = Units::ToUserVSpeed(GetGlidePolar().GetMC());
    RenderValue(canvas, geometry.mc, mc_di,
                mc,
                GetComputerSettings().task.auto_mc ? _T("Auto MC") : _T("MC"));
  }

  if (Settings().show_speed_to_fly)
    RenderSpeedToFly(canvas, rc.right - 11, (rc.top + rc.bottom) / 2);
  else
    RenderClimb(canvas);

  if (Settings().show_ballast)
    RenderBallast(canvas);

  if (Settings().show_bugs)
    RenderBugs(canvas);

  dirty = false;
  int ival, sval, ival_av = 0;
  int ival_av_thermal = 0;
  if (Settings().show_thermal_average_needle) {
      ival_av_thermal = ValueToNeedlePos(Calculated().current_thermal.lift_rate);
  }

  auto vval = Basic().brutto_vario;
  ival = ValueToNeedlePos(vval);
  sval = ValueToNeedlePos(Calculated().sink_rate);
  if (Settings().show_average_needle) {
    if (!Calculated().circling)
      ival_av = ValueToNeedlePos(Calculated().netto_average);
    else
      ival_av = ValueToNeedlePos(Calculated().average);
  }

  // clear items first

  if (Settings().show_average_needle) {
    if (!IsPersistent() || ival_av != ival_last)
      RenderNeedle(canvas, ival_last, true, true);

    ival_last = ival_av;
  }

  if (!IsPersistent() || (sval != sval_last) || (ival != vval_last))
    RenderVarioLine(canvas, vval_last, sval_last, true);

  sval_last = sval;
  if (Settings().show_thermal_average_needle) {
      if (!IsPersistent() || ival_av_thermal != ival_av_last)
          RenderNeedle(canvas, ival_av_last, false, true);

      ival_av_last = ival_av_thermal;
  } else {
      if (!IsPersistent() || ival != vval_last)
        RenderNeedle(canvas, vval_last, false, true);

      vval_last = ival;
  }

  // now draw items
  RenderVarioLine(canvas, ival, sval, false);
  if (Settings().show_average_needle)
    RenderNeedle(canvas, ival_av, true, false);

  RenderNeedle(canvas,
               Settings().show_thermal_average_needle ? ival_av_thermal : ival,
               false, false);

  if (Settings().show_gross) {
    auto vvaldisplay = Clamp(Units::ToUserVSpeed(vval),
                              -99.9, 99.9);

    RenderValue(canvas, geometry.gross, gross_di,
                vvaldisplay,
                _T("Gross"));
  }

  RenderZero(canvas);
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

  if (Basic().switch_state.flight_mode == SwitchState::FlightMode::CIRCLING)
    canvas.ScaleCopy({x, y}, look.climb_bitmap, {12, 0}, {12, 12});
  else if (IsPersistent())
    canvas.DrawFilledRectangle(PixelRect{{x, y}, PixelSize{Layout::Scale(12u)}},
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
  i = Clamp(i, int(gmin), int(gmax));
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
                        double value, const TCHAR *label) noexcept
{
  value = (double)iround(value * 10) / 10; // prevent the -0.0 case

  canvas.SetBackgroundTransparent();

  if (!IsPersistent() || (dirty && !StringIsEqual(di.label.last_text, label))) {
    canvas.SetTextColor(look.dimmed_text_color);
    canvas.Select(*look.text_font);
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
      _tcscpy(di.label.last_text, label);
    } else {
      canvas.DrawText(text_position, label);
    }
  }

  if (!IsPersistent() || (dirty && di.value.last_value != value)) {
    TCHAR buffer[18];
    canvas.SetBackgroundColor(look.background_color);
    canvas.SetTextColor(look.text_color);
    _stprintf(buffer, _T("%.1f"), (double)value);
    canvas.Select(look.value_font);
    const unsigned width = canvas.CalcTextSize(buffer).width;

    const PixelPoint text_position{g.value_right - (int)width, g.value_y};

    if (IsPersistent()) {
      PixelRect rc;
      rc.left = text_position.x;
      rc.top = g.value_top;
      rc.right = g.value_right;
      rc.bottom = g.value_bottom;

      canvas.DrawOpaqueText(text_position, rc, buffer);

      di.value.last_width = width;
      di.value.last_value = value;
    } else {
      canvas.DrawText(text_position, buffer);
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
  if (!Basic().airspeed_available ||
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
    v_diff = Calculated().V_stf - Basic().indicated_airspeed;
    v_diff = Clamp(v_diff, -DELTA_V_LIMIT, DELTA_V_LIMIT); // limit it
    v_diff = iround(v_diff/DELTA_V_STEP) * DELTA_V_STEP;
  } else
    v_diff = 0;

  if (!IsPersistent() || last_v_diff != v_diff || dirty) {
    last_v_diff = v_diff;

    if (IsPersistent()) {
      const unsigned height = nary + arrow_y_size + Layout::FastScale(2);

      // bottom (too slow)
      canvas.DrawFilledRectangle({{x, ybottom + YOFFSET},
                                  {arrow_x_size * 2 + 1, height}},
                                 look.background_color);

      // top (too fast)
      canvas.DrawFilledRectangle({{x, ytop - YOFFSET + 1 - (int)height},
                                  {arrow_x_size * 2 + 1, height}},
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

      while (v_diff > 0) {
        if (v_diff > DELTA_V_STEP) {
          canvas.DrawRectangle({{x, y}, {arrow_x_size * 2 + 1, arrow_y_size - 1}});
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

      while (v_diff < 0) {
        if (v_diff < -DELTA_V_STEP) {
          canvas.DrawRectangle({{x, y + 1},
                                {arrow_x_size * 2 + 1, y - arrow_y_size + 1}});
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
  int ballast = iround(GetGlidePolar().GetBallast() * 100);

  if (!IsPersistent() || ballast != last_ballast) {
    // ballast hase been changed

    canvas.Select(*look.text_font);

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
      TCHAR buffer[18];
      _stprintf(buffer, _T("%u%%"), ballast);
      canvas.SetTextColor(look.text_color);

      if (IsPersistent())
        canvas.DrawOpaqueText(g.value_pos, g.value_rect, buffer);
      else
        canvas.DrawText(g.value_pos, buffer);
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

    canvas.Select(*look.text_font);

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
      TCHAR buffer[18];
      _stprintf(buffer, _T("%d%%"), bugs);
      canvas.SetTextColor(look.text_color);
      if (IsPersistent())
        canvas.DrawOpaqueText(g.value_pos, g.value_rect, buffer);
      else 
        canvas.DrawText(g.value_pos, buffer);
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(g.value_rect, look.background_color);

    if (IsPersistent())
      last_bugs = bugs;
  }
}

void
GaugeVario::OnResize(PixelSize new_size)
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
