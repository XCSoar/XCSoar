// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gauge/GaugeVario.hpp"
#include "Audio/VarioAudioValue.hpp"
#include "Computer/STF.hpp"
#include "Look/VarioLook.hpp"
#include "Look/Look.hpp"
#include "UIGlobals.hpp"
#include "InfoBoxes/Data.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Input/InputEvents.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Math/FastRotation.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "lib/fmt/ToBuffer.hxx"

#include <algorithm> // for std::clamp()

static constexpr double DELTA_V_STEP = 4.0;
static constexpr double DELTA_V_LIMIT = 16.0;

/** Climb/sink band and the speed bar, in pixels. */
static int
BandWidth() noexcept
{
  return std::max(1, (int)Layout::Scale(5));
}
static constexpr const char *TEXT_BUG = "Bug";
static constexpr const char *TEXT_BALLAST = "Bal";

struct DialFit {
  int radius;
  /** Pivot moves left by this when the height cap shortens the radius. */
  int left_shift;
};

static DialFit
FitDial(const VarioLook &look, PixelSize window) noexcept
{
  const int arc_padding = look.arc_label_font.GetHeight();
  const int tick_length = look.Scale(Layout::GetTextPadding() * 4);
  const int label_width = look.arc_label_font.TextSize("-5").width;
  const int right = int(std::min(window.width, look.geometry_width))
    - int(look.Scale(Layout::GetTextPadding()));

  int radius = right - tick_length - arc_padding / 2 - label_width / 2;
  int left_shift = 0;

  /* Keep the circle inside the gauge. A two-row portrait gauge is
     shorter than the column is wide, and the spare width shifts
     the arc left. */
  const int limit = int(window.height / 2);
  if (limit > 0 && radius > limit) {
    left_shift = radius - limit;
    radius = limit;
  }

  /* A tall gauge has no spare width, so the figures sit on the right
     edge and cover the speed bar. Leave one bar's width. The two-row
     gauge already has at least that. */
  const int bar = BandWidth();
  const int pad = int(look.Scale(Layout::GetTextPadding()));
  const int gap = left_shift + pad;
  if (gap < bar && radius > 1) {
    const int extra = std::min(bar - gap, radius - 1);
    left_shift += extra;
    radius -= extra;
  }

  return {std::max(1, radius), left_shift};
}

inline
GaugeVario::PercentGeometry::PercentGeometry(const VarioLook &look,
                                             const PixelRect &rc,
                                             const bool top,
                                             const char *caption) noexcept
{
  const int capital = look.label_font.GetCapitalHeight();
  const int ascent = look.label_font.GetAscentHeight();

  label_pos.x = 1;
  value_pos.x = 1;
  if (top) {
    label_pos.y = rc.top + 2 + capital * 2 - ascent;
    value_pos.y = rc.top + 1 + capital - ascent;
  } else {
    label_pos.y = rc.bottom - 2 - capital - ascent;
    value_pos.y = rc.bottom - 1 - ascent;
  }

  label_rect.left = label_pos.x;
  value_rect.left = value_pos.x;
  label_rect.top = label_pos.y + ascent - capital;
  value_rect.top = value_pos.y + ascent - capital;

  label_rect.right = label_rect.left
    + look.label_font.TextSize(caption).width;
  label_rect.bottom = label_rect.top + capital;
  /* The bottom caption's erase rect covers the line below. */
  if (!top)
    label_rect.bottom += look.label_font.GetHeight() - ascent;

  value_rect.right = value_rect.left
    + look.label_font.TextSize("100%").width;
  value_rect.bottom = value_rect.top + capital;
}

inline
GaugeVario::LabelValueGeometry::LabelValueGeometry(const VarioLook &look,
                                                   PixelPoint position) noexcept
  :label_right(position.x),
   label_top(position.y + look.Scale(Layout::Scale(1))),
   label_bottom(label_top + look.label_font.GetCapitalHeight()),
   label_y(label_top + look.label_font.GetCapitalHeight()
           - look.label_font.GetAscentHeight()),
   // TODO: update after units got reconfigured?
   value_right(position.x - UnitSymbolRenderer::GetSize(look.unit_font,
                                                        Units::current.vertical_speed_unit).width),
   value_top(label_bottom + look.Scale(Layout::Scale(2))),
   value_bottom(value_top + look.value_font.GetCapitalHeight()),
   value_y(value_top + look.value_font.GetCapitalHeight()
           - look.value_font.GetAscentHeight())
{
}

inline unsigned
GaugeVario::LabelValueGeometry::GetHeight(const VarioLook &look) noexcept
{
  return look.Scale(Layout::Scale(4)) + look.value_font.GetCapitalHeight()
    + look.label_font.GetCapitalHeight();
}

inline
GaugeVario::Geometry::Geometry(const VarioLook &look, const PixelRect &rc) noexcept
  :ballast(look, rc, true, TEXT_BALLAST),
   bugs(look, rc, false, TEXT_BUG)
{
  nlength0 = look.Scale(Layout::Scale(10));
  nlength1 = look.Scale(Layout::Scale(2));
  nwidth = look.Scale(Layout::Scale(4));

  const DialFit dial = FitDial(look, rc.GetSize());
  offset = rc.GetMiddleRight();
  offset.x -= look.Scale(Layout::GetTextPadding()) + dial.left_shift;
  dial_radius = dial.radius;

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

static constexpr PixelPoint
TransformRotatedPoint(IntPoint2D pt, IntPoint2D offset) noexcept
{
  return { pt.x + offset.x, pt.y + offset.y };
}

inline void
GaugeVario::RenderBackground(Canvas &canvas) noexcept
{
  canvas.Clear(look.background_color);

  canvas.Select(look.arc_pen);

  const int arc_padding = look.arc_label_font.GetHeight();
  const int x_radius = geometry.dial_radius;

  /* Left semicircle: bottom, through the left, to the top. */
  canvas.DrawArc(geometry.offset, std::max(1, x_radius),
                 Angle::HalfCircle(), Angle::FullCircle());

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

  const int tick_length = look.Scale(Layout::GetTextPadding() * 4);

  const IntPoint2D tick_start{1 - x_radius, 0};
  const IntPoint2D tick_end{-tick_length - x_radius, 0};
  const IntPoint2D label_center{-x_radius - arc_padding / 2 - tick_length, 0};

  /* The end labels sit beyond the fitted dial on compact layouts. */
  for (int i = 1 - n_ticks; i < n_ticks; ++i) {
    Angle angle = tick_angle_step * i;
    const FastIntegerRotation r{angle};

    canvas.DrawLine(TransformRotatedPoint(r.Rotate(tick_start),
                                          geometry.offset),
                    TransformRotatedPoint(r.Rotate(tick_end),
                                          geometry.offset));

    char label[16];
    StringFormatUnsafe(label, "%d", i * tick_value_step);

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
  if (!IsPersistent() || background_dirty) {
    RenderBackground(canvas);
    background_dirty = false;
  }

  if (Settings().show_average) {
    // JMW averager now displays netto average if not circling
    const bool circling = Calculated().circling;
    const double avg = circling ? Calculated().average
                                : Calculated().netto_average;
    /* Same red as TC 30s: the 30 s climb is below half the risk MC.
       NetAvg is a different figure, so it stays the plain text colour. */
    const int color_index =
      circling && 2 * avg < Calculated().common_stats.current_risk_mc
        ? 1 : 0;
    if (color_index != average_color_index) {
      average_color_index = color_index;
      average_di.value.last_value = -9999;
      dirty = true;
    }

    RenderValue(canvas, geometry.average, average_di,
                Units::ToUserVSpeed(avg),
                circling ? "Avg" : "NetAvg",
                color_index == 0
                  ? look.text_color
                  : UIGlobals::GetLook().info_box.GetValueColor(color_index));
  }

  bool show_mc_mark = false;
  int mc_pos = 0;
  Color mc_color = COLOR_BLACK;
  if (Settings().show_mc) {
    const bool abort = Calculated().common_stats.task_type == TaskType::ABORT;
    const bool auto_mc = GetComputerSettings().task.auto_mc;
    const int color_index = abort ? (int)InfoBoxData::COLOR_ORANGE
                                  : auto_mc ? 2 : 3;
    if (color_index != mc_color_index) {
      mc_color_index = color_index;
      mc_di.value.last_value = -9999;
      dirty = true;
    }

    double mc = GetGlidePolar().GetMC();
    const char *label = auto_mc ? "Auto MC" : "MC";
    if (abort) {
      const GlidePolar &safety = Calculated().glide_polar_safety;
      mc = safety.IsValid()
        ? safety.GetMC()
        : GetComputerSettings().task.safety_mc;
      label = _("Safety MC");
    }

    mc_color = UIGlobals::GetLook().info_box.GetValueColor(color_index);
    mc_pos = ValueToNeedlePos(mc);
    show_mc_mark = true;

    RenderValue(canvas, geometry.mc, mc_di,
                Units::ToUserVSpeed(mc),
                label,
                mc_color);
  }

  if (Settings().show_speed_to_fly)
    RenderSpeedToFly(canvas);
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

  const bool vario_available = Basic().brutto_vario_available;
  const auto vval = Basic().VarioOutputFilterActive()
    ? (vario_available ? Basic().FilteredBruttoVario() : 0.)
    : (vario_available ? Basic().brutto_vario : 0.);
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

  if (vario_line_drawn &&
      (!vario_available || !IsPersistent() ||
       (sval != sval_last) || (ival != vval_last))) {
    RenderVarioLine(canvas, vval_last, sval_last, true);
    vario_line_drawn = false;
  }

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
  if (vario_available) {
    RenderVarioLine(canvas, ival, sval, false);
    vario_line_drawn = true;
  }
  if (Settings().show_average_needle)
    RenderNeedle(canvas, ival_av, true, false);

  RenderNeedle(canvas,
               Settings().show_thermal_average_needle ? ival_av_thermal : ival,
               false, false);

  if (show_mc_mark) {
    if (!IsPersistent() || mc_mark_pos != mc_pos
        || mc_mark_color != mc_color_index) {
      if (IsPersistent() && mc_mark_pos >= gmin)
        RenderMcMark(canvas, mc_mark_pos, look.background_color);
      RenderMcMark(canvas, mc_pos, mc_color);
      mc_mark_pos = mc_pos;
      mc_mark_color = mc_color_index;
    }
  } else if (IsPersistent() && mc_mark_pos >= gmin) {
    RenderMcMark(canvas, mc_mark_pos, look.background_color);
    mc_mark_pos = gmin - 1;
    mc_mark_color = -1;
  }

  if (Settings().show_gross) {
    auto vvaldisplay = std::clamp(Units::ToUserVSpeed(vval),
                                  -99.9, 99.9);

    RenderValue(canvas, geometry.gross, gross_di,
                vvaldisplay,
                "Gross",
                look.text_color);
  }

  RenderMode(canvas);
}

void
GaugeVario::RenderMode(Canvas &canvas) noexcept
{
  const auto &sound = blackboard.GetUISettings().sound.vario;
  const bool stf = AudioVarioGlue::UsesSTF(
    Calculated().circling, Basic().V_stf_available,
    sound.switching_mode, sound.manual_mode);
  const int mode = stf ? 1 : 0;
  if (IsPersistent() && mode_stf == mode)
    return;

  const char *label = stf ? "STF" : "Var";
  canvas.Select(look.label_font);
  canvas.SetTextColor(look.dimmed_text_color);
  canvas.SetBackgroundColor(look.background_color);

  const int width = (int)std::max(canvas.CalcTextWidth("STF"),
                                  canvas.CalcTextWidth("Var"));
  const int height = look.label_font.GetCapitalHeight();
  const int right = geometry.mc.label_right;
  const int top = geometry.mc.value_bottom
    + look.Scale(Layout::Scale(1));
  canvas.DrawOpaqueText({right - (int)canvas.CalcTextWidth(label),
                         top + height
                           - (int)look.label_font.GetAscentHeight()},
                        {right - width, top, right, top + height},
                        label);
  mode_stf = mode;
}

void
GaugeVario::RenderClimb(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();
  int x = rc.right - int(look.Scale(Layout::Scale(14)));
  int y = rc.bottom - int(look.Scale(Layout::Scale(24)));

  if (!dirty)
    return;

  const unsigned dest = look.Scale(Layout::VptScale(9));
  const PixelSize dest_size{dest};

  if (Basic().switch_state.flight_mode == SwitchState::FlightMode::CIRCLING) {
    canvas.SetTextColor(look.text_color);
    look.climb_icon.Draw(canvas,
                         {x + int(dest / 2), y + int(dest / 2)},
                         dest);
  } else if (IsPersistent())
    canvas.DrawFilledRectangle(PixelRect{{x, y}, dest_size},
                               look.background_color);
}

int
GaugeVario::ValueToNeedlePos(double Value) noexcept
{
  constexpr double degrees_per_unit =
    double(GAUGEVARIOSWEEP) / GAUGEVARIORANGE;

  return std::clamp(iround(Value * degrees_per_unit),
                    int(gmin), int(gmax));
}

void
GaugeVario::RenderVarioLine(Canvas &canvas, int i, int sink,
                            bool clear) noexcept
{
  dirty = true;
  if (i == sink)
    return;

  const int lo = std::min(i, sink);
  const int hi = std::max(i, sink);
  const unsigned outer = std::max(1, geometry.dial_radius);
  const unsigned thick = BandWidth();
  const unsigned inner = outer > thick ? outer - thick : 1u;

  canvas.SelectNullPen();
  if (clear) {
    if (look.inverse)
      canvas.SelectBlackBrush();
    else
      canvas.SelectWhiteBrush();
  } else
    canvas.Select(i > sink ? look.lift_brush : look.sink_brush);

  /* 0° on this dial is the left of the circle, climb is towards
     the top. */
  canvas.DrawAnnulus(geometry.offset, inner, outer,
                     Angle::Degrees(270 + lo),
                     Angle::Degrees(270 + hi));
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

  const FastIntegerRotation r(Angle::Degrees(i));
  const int radius = geometry.dial_radius;
  const BulkPixelPoint needle[3] = {
    TransformRotatedPoint(r.Rotate({-radius + geometry.nlength0,
                                    geometry.nwidth}),
                          geometry.offset),
    TransformRotatedPoint(r.Rotate({-radius + geometry.nlength1, 0}),
                          geometry.offset),
    TransformRotatedPoint(r.Rotate({-radius + geometry.nlength0,
                                    -geometry.nwidth}),
                          geometry.offset),
  };

  if (average)
    canvas.DrawPolyline(needle, 3);
  else
    canvas.DrawTriangleFan(needle, 3);
}

void
GaugeVario::RenderMcMark(Canvas &canvas, int i, Color color) noexcept
{
  /* Outside the scale, pointing in at the MacCready value. */
  const int radius = geometry.dial_radius;
  const int thick = BandWidth();
  const int outer = radius + (int)look.arc_pen.GetWidth() / 2;
  const int length = std::max(3, thick);
  const int half = std::max(2, length / 2);
  const FastIntegerRotation r(Angle::Degrees(i));
  const BulkPixelPoint arrow[3] = {
    TransformRotatedPoint(r.Rotate({-outer, 0}), geometry.offset),
    TransformRotatedPoint(r.Rotate({-outer - length, half}),
                          geometry.offset),
    TransformRotatedPoint(r.Rotate({-outer - length, -half}),
                          geometry.offset),
  };

  Brush brush;
  brush.Create(color);
  canvas.SelectNullPen();
  canvas.Select(brush);
  canvas.DrawTriangleFan(arrow, 3);
}

// TODO code: Optimise vario rendering, this is slow
void
GaugeVario::RenderValue(Canvas &canvas, const LabelValueGeometry &g,
                        LabelValueDrawInfo &di,
                        double value, const char *label,
                        Color value_color) noexcept
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
      strcpy(di.label.last_text, label);
    } else {
      canvas.DrawText(text_position, label);
    }
  }

  if (!IsPersistent() || (dirty && di.value.last_value != value)) {
    const auto buffer = FmtBuffer<18>("{:.1f}", value);
    canvas.SetBackgroundColor(look.background_color);
    canvas.SetTextColor(value_color);
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
GaugeVario::RenderSpeedToFly(Canvas &canvas) noexcept
{
  /* Cruise, or no switch and the computer is not circling. A climb
     switch or a detected circle clears the bar. */
  const bool stf =
    Basic().switch_state.SpeedCommandPage()
      != SwitchState::FlightMode::CIRCLING
    && !Calculated().circling;

  const auto stf_speed = stf && Basic().airspeed_available
    ? GetSTFSpeed(Basic(), Calculated())
    : std::optional<double>{};

  const PixelRect rc = GetClientRect();

  /* V_stf is the target TAS. Positive means too slow. */
  double v_diff = 0;
  if (stf_speed) {
    v_diff = *stf_speed - Basic().true_airspeed;
    v_diff = std::clamp(v_diff, -DELTA_V_LIMIT, DELTA_V_LIMIT);
    v_diff = iround(v_diff / DELTA_V_STEP) * DELTA_V_STEP;
  }

  /* Same thickness and colours as the climb/sink band, drawn
     vertically. Up commands slow down. Full deflection meets the
     top or bottom of the scale. */
  const int thick = BandWidth();
  const int x = rc.right - thick;
  const int mid = geometry.offset.y;
  const int reach = std::max(1, geometry.dial_radius);
  const double speed_error = v_diff < 0 ? -v_diff : v_diff;
  const int span = std::min(reach,
                            int(speed_error / DELTA_V_LIMIT * reach));

  if (!IsPersistent() || last_v_diff != v_diff || dirty) {
    last_v_diff = v_diff;

    if (IsPersistent())
      canvas.DrawFilledRectangle({x, mid - reach, rc.right, mid + reach},
                                 look.background_color);

    RenderClimb(canvas);

    if (span > 0) {
      const bool down = v_diff > 0;
      const int y1 = down ? mid + span : mid - span;
      const Color color = down ? look.sink_color : look.lift_color;
      canvas.DrawFilledRectangle({x, std::min(mid, y1),
                                  x + thick, std::max(mid, y1)},
                                 color);
    }
  }
}

void
GaugeVario::RenderPercent(Canvas &canvas, const PercentGeometry &corner,
                          const char *caption,
                          int percent, int &last) noexcept
{
  const bool persistent = IsPersistent();
  if (persistent && percent == last)
    return;

  canvas.Select(look.label_font);
  if (persistent)
    canvas.SetBackgroundColor(look.background_color);
  else
    canvas.SetBackgroundTransparent();

  if (persistent || last < 1 || percent < 1) {
    if (percent > 0) {
      canvas.SetTextColor(look.dimmed_text_color);
      if (persistent)
        canvas.DrawOpaqueText(corner.label_pos, corner.label_rect, caption);
      else
        canvas.DrawText(corner.label_pos, caption);
    } else if (persistent)
      canvas.DrawFilledRectangle(corner.label_rect, look.background_color);
  }

  if (percent > 0) {
    const auto buffer = FmtBuffer<18>("{}%", percent);
    canvas.SetTextColor(look.text_color);
    if (persistent)
      canvas.DrawOpaqueText(corner.value_pos, corner.value_rect,
                            buffer.c_str());
    else
      canvas.DrawText(corner.value_pos, buffer.c_str());
  } else if (persistent)
    canvas.DrawFilledRectangle(corner.value_rect, look.background_color);

  if (persistent)
    last = percent;
}

inline void
GaugeVario::RenderBallast(Canvas &canvas) noexcept
{
  const int ballast = iround(GetGlidePolar().GetBallastFraction() * 100);
  RenderPercent(canvas, geometry.ballast, TEXT_BALLAST, ballast, last_ballast);
}

inline void
GaugeVario::RenderBugs(Canvas &canvas) noexcept
{
  const int bugs = iround((1 - GetComputerSettings().polar.bugs) * 100);
  RenderPercent(canvas, geometry.bugs, TEXT_BUG, bugs, last_bugs);
}

void
GaugeVario::OnCancelMode() noexcept
{
  if (dragging) {
    dragging = false;
    ReleaseCapture();
  }

  AntiFlickerWindow::OnCancelMode();
}

bool
GaugeVario::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  if (!dragging) {
    dragging = true;
    SetCapture();
  }

  return true;
}

bool
GaugeVario::OnMouseUp(PixelPoint p) noexcept
{
  if (!dragging)
    return false;

  dragging = false;
  ReleaseCapture();

  /* Same dialog as the Ballast InfoBox.  A slide off cancels. */
  if (IsInside(p))
    InputEvents::eventSetup("Basic");

  return true;
}

bool
GaugeVario::OnMouseMove([[maybe_unused]] PixelPoint p,
                        [[maybe_unused]] unsigned keys) noexcept
{
  return dragging;
}

void
GaugeVario::OnResize(PixelSize new_size) noexcept
{
  AntiFlickerWindow::OnResize(new_size);

  ReinitialiseLook();
}

void
GaugeVario::ReinitialiseLook() noexcept
{

  geometry = {look, GetClientRect()};

  /* trigger reinitialisation */
  dirty = true;
  background_dirty = true;
  vario_line_drawn = false;
  mc_mark_pos = gmin - 1;
  mc_mark_color = -1;

  average_di.Reset();
  mc_di.Reset();
  gross_di.Reset();

  last_ballast = -1;
  last_bugs = -1;
  mode_stf = -1;

  Invalidate();
}
