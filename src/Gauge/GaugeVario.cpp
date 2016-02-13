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

#include "Gauge/GaugeVario.hpp"
#include "Look/VarioLook.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Math/FastRotation.hpp"
#include "Units/Units.hpp"
#include "Util/Clamp.hpp"

#define DELTA_V_STEP 4.
#define DELTA_V_LIMIT 16.
#define TEXT_BUG _T("Bug")
#define TEXT_BALLAST _T("Bal")

GaugeVario::GaugeVario(const FullBlackboard &_blackboard,
                       ContainerWindow &parent, const VarioLook &_look,
                       PixelRect rc, const WindowStyle style)
  :blackboard(_blackboard), look(_look),
   nlength0(Layout::Scale(15)),
   nlength1(Layout::Scale(6)),
   nwidth(Layout::Scale(4)),
   nline(Layout::Scale(8)),
   dirty(true), layout_initialised(false), needle_initialised(false),
   ballast_initialised(false), bugs_initialised(false)
{
  value_top.initialised = false;
  value_middle.initialised = false;
  value_bottom.initialised = false;
  label_top.initialised = false;
  label_middle.initialised = false;
  label_bottom.initialised = false;

  Create(parent, rc, style);
}

void
GaugeVario::OnPaintBuffer(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();
  const unsigned width = rc.GetWidth(), height = rc.GetHeight();

  if (!IsPersistent() || !layout_initialised) {
    unsigned value_height = 4 + look.value_font.GetCapitalHeight()
      + look.text_font->GetCapitalHeight();

    middle_position.y = offset.y - value_height / 2;
    middle_position.x = rc.right;
    top_position.y = middle_position.y - value_height;
    top_position.x = rc.right;
    bottom_position.y = middle_position.y + value_height;
    bottom_position.x = rc.right;

    canvas.Stretch(rc.left, rc.top, width, height,
                   look.background_bitmap,
                   look.background_x, 0, 58, 120);

    layout_initialised = true;
  }

  if (Settings().show_average) {
    // JMW averager now displays netto average if not circling
    if (!Calculated().circling) {
      RenderValue(canvas, top_position.x, top_position.y, &value_top, &label_top,
                  Units::ToUserVSpeed(Calculated().netto_average),
                  _T("NetAvg"));
    } else {
      RenderValue(canvas, top_position.x, top_position.y,
                  &value_top, &label_top,
                  Units::ToUserVSpeed(Calculated().average), _T("Avg"));
    }
  }

  if (Settings().show_mc) {
    auto mc = Units::ToUserVSpeed(GetGlidePolar().GetMC());
    RenderValue(canvas, bottom_position.x, bottom_position.y,
                &value_bottom, &label_bottom,
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
  static int vval_last = 0;
  static int sval_last = 0;
  static int ival_last = 0;

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

  if (!IsPersistent() || ival != vval_last)
    RenderNeedle(canvas, vval_last, false, true);

  vval_last = ival;

  // now draw items
  RenderVarioLine(canvas, ival, sval, false);
  if (Settings().show_average_needle)
    RenderNeedle(canvas, ival_av, true, false);

  RenderNeedle(canvas, ival, false, false);

  if (Settings().show_gross) {
    auto vvaldisplay = Clamp(Units::ToUserVSpeed(vval),
                              -99.9, 99.9);

    RenderValue(canvas, middle_position.x, middle_position.y,
                &value_middle, &label_middle,
                vvaldisplay,
                _T("Gross"));
  }

  RenderZero(canvas);
}

gcc_const
static PixelPoint
TransformRotatedPoint(IntPoint2D pt, IntPoint2D offset)
{
  return { pt.x + offset.x, (pt.y * 112 / 100) + offset.y + 1 };
}

void
GaugeVario::MakePolygon(const int i)
{
  auto *bit = getPolygon(i);
  auto *bline = &lines[i + gmax];

  const FastIntegerRotation r(Angle::Degrees(i));

  bit[0] = TransformRotatedPoint(r.Rotate(-offset.x + nlength0, nwidth),
                                 offset);
  bit[1] = TransformRotatedPoint(r.Rotate(-offset.x + nlength1, 0),
                                 offset);
  bit[2] = TransformRotatedPoint(r.Rotate(-offset.x + nlength0, -nwidth),
                                 offset);

  *bline = TransformRotatedPoint(r.Rotate(-offset.x + nline, 0),
                                 offset);
}

BulkPixelPoint *
GaugeVario::getPolygon(int i)
{
  return polys + (i + gmax) * 3;
}

void
GaugeVario::MakeAllPolygons()
{
  for (int i = gmin; i <= gmax; i++)
    MakePolygon(i);
}

void
GaugeVario::RenderClimb(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();
  int x = rc.right - Layout::Scale(14);
  int y = rc.bottom - Layout::Scale(24);

  if (!dirty)
    return;

  if (Basic().switch_state.flight_mode == SwitchState::FlightMode::CIRCLING)
    canvas.ScaleCopy(x, y, look.climb_bitmap, 12, 0, 12, 12);
  else if (IsPersistent())
    canvas.DrawFilledRectangle(x, y, x + Layout::Scale(12), y + Layout::Scale(12),
                          look.background_color);
}

void
GaugeVario::RenderZero(Canvas &canvas)
{
  if (look.inverse)
    canvas.SelectWhitePen();
  else
    canvas.SelectBlackPen();

  canvas.DrawLine(0, offset.y, Layout::Scale(17), offset.y);
  canvas.DrawLine(0, offset.y + 1, Layout::Scale(17), offset.y + 1);
}

int
GaugeVario::ValueToNeedlePos(double Value)
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
GaugeVario::RenderVarioLine(Canvas &canvas, int i, int sink, bool clear)
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
GaugeVario::RenderNeedle(Canvas &canvas, int i, bool average, bool clear)
{
  dirty = true;

  canvas.SelectNullPen();

  // legacy behaviour
  if (clear ^ look.inverse) {
    canvas.SelectWhiteBrush();
  } else {
    canvas.SelectBlackBrush();
  }

  if (average)
    canvas.DrawPolyline(getPolygon(i), 3);
  else
    canvas.DrawTriangleFan(getPolygon(i), 3);
}

// TODO code: Optimise vario rendering, this is slow
void
GaugeVario::RenderValue(Canvas &canvas, int x, int y,
                        DrawInfo *value_info, DrawInfo *label_info,
                        double value, const TCHAR *label)
{
  PixelSize tsize;

  value = (double)iround(value * 10) / 10; // prevent the -0.0 case

  if (!value_info->initialised) {

    value_info->rc.right = x - Layout::Scale(5);
    value_info->rc.top = y + Layout::Scale(3)
      + look.text_font->GetCapitalHeight();

    value_info->rc.left = value_info->rc.right;
    // update back rect with max label size
    value_info->rc.bottom = value_info->rc.top + look.value_font.GetCapitalHeight();

    value_info->text_position.x = value_info->rc.left;
    value_info->text_position.y = value_info->rc.top
                         + look.value_font.GetCapitalHeight()
                         - look.value_font.GetAscentHeight();

    value_info->last_value = -9999;
    value_info->last_text[0] = '\0';
    value_info->last_unit = Unit::UNDEFINED;
    value_info->initialised = true;
  }

  if (!label_info->initialised) {

    label_info->rc.right = x;
    label_info->rc.top = y + Layout::Scale(1);

    label_info->rc.left = label_info->rc.right;
    // update back rect with max label size
    label_info->rc.bottom = label_info->rc.top
      + look.text_font->GetCapitalHeight();

    label_info->text_position.x = label_info->rc.left;
    label_info->text_position.y = label_info->rc.top
      + look.text_font->GetCapitalHeight()
      - look.text_font->GetAscentHeight();

    label_info->last_value = -9999;
    label_info->last_text[0] = '\0';
    label_info->initialised = true;
  }

  canvas.SetBackgroundTransparent();

  if (!IsPersistent() || (dirty && !StringIsEqual(label_info->last_text, label))) {
    canvas.SetTextColor(look.dimmed_text_color);
    canvas.Select(*look.text_font);
    tsize = canvas.CalcTextSize(label);
    label_info->text_position.x = label_info->rc.right - tsize.cx;

    if (IsPersistent()) {
      canvas.SetBackgroundColor(look.background_color);
      canvas.DrawOpaqueText(label_info->text_position.x, label_info->text_position.y,
                            label_info->rc, label);
      label_info->rc.left = label_info->text_position.x;
      _tcscpy(label_info->last_text, label);
    } else {
      canvas.DrawText(label_info->text_position.x, label_info->text_position.y,
                      label);
    }
  }

  if (!IsPersistent() || (dirty && value_info->last_value != value)) {
    TCHAR buffer[18];
    canvas.SetBackgroundColor(look.background_color);
    canvas.SetTextColor(look.text_color);
    _stprintf(buffer, _T("%.1f"), (double)value);
    canvas.Select(look.value_font);
    tsize = canvas.CalcTextSize(buffer);
    value_info->text_position.x = value_info->rc.right - tsize.cx;

    if (IsPersistent()) {
      canvas.DrawOpaqueText(value_info->text_position.x,
                            value_info->text_position.y,
                            value_info->rc, buffer);

      value_info->rc.left = value_info->text_position.x;
      value_info->last_value = value;
    } else {
      canvas.DrawText(value_info->text_position.x, value_info->text_position.y,
                      buffer);
    }
  }

  if (!IsPersistent() ||
      value_info->last_unit != Units::current.vertical_speed_unit) {
    auto unit = value_info->last_unit = Units::current.vertical_speed_unit;

    const int ascent_height = look.value_font.GetAscentHeight();
    const int unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.unit_font, unit);

    canvas.Select(look.unit_font);
    canvas.SetTextColor(COLOR_GRAY);
    UnitSymbolRenderer::Draw(canvas,
                             PixelPoint(x - Layout::Scale(5),
                                        value_info->text_position.y + ascent_height - unit_height),
                             unit, look.unit_fraction_pen);
  }
}

void
GaugeVario::RenderSpeedToFly(Canvas &canvas, int x, int y)
{
  if (!Basic().airspeed_available ||
      !Basic().total_energy_vario_available)
    return;

  static double last_v_diff;
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
      // bottom (too slow)
      canvas.DrawFilledRectangle(x, ybottom + YOFFSET,
                                 x + arrow_x_size * 2 + 1,
                                 ybottom + YOFFSET + nary + arrow_y_size +
                                 Layout::FastScale(2),
                                 look.background_color);

      // top (too fast)
      canvas.DrawFilledRectangle(x, ytop - YOFFSET + 1,
                                 x + arrow_x_size * 2  +1,
                                 ytop - YOFFSET - nary + 1 - arrow_y_size -
                                 Layout::FastScale(2),
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
          canvas.Rectangle(x, y,
                           x + arrow_x_size * 2 + 1, y + arrow_y_size - 1);
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
          canvas.Rectangle(x, y + 1,
                           x + arrow_x_size * 2 + 1, y - arrow_y_size + 2);
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

void
GaugeVario::RenderBallast(Canvas &canvas)
{
  static int last_ballast = -1;
  static PixelRect label_rect = {-1,-1,-1,-1};
  static PixelRect value_rect = {-1,-1,-1,-1};
  static PixelPoint label_pos = {-1,-1};
  static PixelPoint value_pos = {-1,-1};

  if (!ballast_initialised) { // ontime init, origin and background rect
    const PixelRect rc = GetClientRect();

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
    canvas.Select(*look.text_font);
    tSize = canvas.CalcTextSize(TEXT_BALLAST);

    // update back rect with max label size
    label_rect.right = label_rect.left + tSize.cx;
    label_rect.bottom = label_rect.top +
      look.text_font->GetCapitalHeight();

    // get max value size
    tSize = canvas.CalcTextSize(_T("100%"));

    value_rect.right = value_rect.left + tSize.cx;
    // update back rect with max label size
    value_rect.bottom = value_rect.top +
      look.text_font->GetCapitalHeight();

    ballast_initialised = true;
  }

  int ballast = iround(GetGlidePolar().GetBallast() * 100);

  if (!IsPersistent() || ballast != last_ballast) {
    // ballast hase been changed

    canvas.Select(*look.text_font);

    if (IsPersistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    if (IsPersistent() || last_ballast < 1 || ballast < 1) {
      // new ballast is 0, hide label
      if (ballast > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        // ols ballast was 0, show label
        if (IsPersistent())
          canvas.DrawOpaqueText(label_pos.x, label_pos.y, label_rect, TEXT_BALLAST);
        else
          canvas.DrawText(label_pos.x, label_pos.y, TEXT_BALLAST);
      } else if (IsPersistent())
        canvas.DrawFilledRectangle(label_rect, look.background_color);
    }

    // new ballast 0, hide value
    if (ballast > 0) {
      TCHAR buffer[18];
      _stprintf(buffer, _T("%u%%"), ballast);
      canvas.SetTextColor(look.text_color);

      if (IsPersistent())
        canvas.DrawOpaqueText(value_pos.x, value_pos.y, value_rect, buffer);
      else
        canvas.DrawText(value_pos.x, value_pos.y, buffer);
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(value_rect, look.background_color);

    if (IsPersistent())
      last_ballast = ballast;
  }
}

void
GaugeVario::RenderBugs(Canvas &canvas)
{
  static int last_bugs = -1;
  static PixelRect label_rect = {-1,-1,-1,-1};
  static PixelRect value_rect = {-1,-1,-1,-1};
  static PixelPoint label_pos = {-1,-1};
  static PixelPoint value_pos = {-1,-1};

  if (!bugs_initialised) {
    const PixelRect rc = GetClientRect();
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

    canvas.Select(*look.text_font);
    tSize = canvas.CalcTextSize(TEXT_BUG);

    label_rect.right = label_rect.left + tSize.cx;
    label_rect.bottom = label_rect.top
      + look.text_font->GetCapitalHeight()
      + look.text_font->GetHeight()
      - look.text_font->GetAscentHeight();

    tSize = canvas.CalcTextSize(_T("100%"));

    value_rect.right = value_rect.left + tSize.cx;
    value_rect.bottom = value_rect.top +
      look.text_font->GetCapitalHeight();

    bugs_initialised = true;
  }

  int bugs = iround((1 - GetComputerSettings().polar.bugs) * 100);
  if (!IsPersistent() || bugs != last_bugs) {

    canvas.Select(*look.text_font);

    if (IsPersistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    if (IsPersistent() || last_bugs < 1 || bugs < 1) {
      if (bugs > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        if (IsPersistent())
          canvas.DrawOpaqueText(label_pos.x, label_pos.y, label_rect, TEXT_BUG);
        else
          canvas.DrawText(label_pos.x, label_pos.y, TEXT_BUG);
      } else if (IsPersistent())
        canvas.DrawFilledRectangle(label_rect, look.background_color);
    }

    if (bugs > 0) {
      TCHAR buffer[18];
      _stprintf(buffer, _T("%d%%"), bugs);
      canvas.SetTextColor(look.text_color);
      if (IsPersistent())
        canvas.DrawOpaqueText(value_pos.x, value_pos.y, value_rect, buffer);
      else 
        canvas.DrawText(value_pos.x, value_pos.y, buffer);
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(value_rect, look.background_color);

    if (IsPersistent())
      last_bugs = bugs;
  }
}

void
GaugeVario::OnResize(PixelSize new_size)
{
  AntiFlickerWindow::OnResize(new_size);

  /* trigger reinitialisation */
  offset.x = new_size.cx;
  offset.y = new_size.cy / 2;
  layout_initialised = false;
  needle_initialised = false;
  ballast_initialised = false;
  bugs_initialised = false;

  value_top.initialised = false;
  value_middle.initialised = false;
  value_bottom.initialised = false;
  label_top.initialised = false;
  label_middle.initialised = false;
  label_bottom.initialised = false;
}
