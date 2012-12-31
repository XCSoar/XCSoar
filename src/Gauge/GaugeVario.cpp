/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Look/UnitsLook.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Layout.hpp"
#include "Screen/FastPixelRotation.hpp"
#include "LogFile.hpp"
#include "Units/Units.hpp"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

using std::min;
using std::max;

#define DeltaVstep fixed_four
#define DeltaVlimit fixed(16)
#define TextBug _T("Bug")
#define TextBal _T("Bal")

GaugeVario::GaugeVario(const FullBlackboard &_blackboard,
                       ContainerWindow &parent, const VarioLook &_look,
                       const UnitsLook &_units_look,
                       PixelScalar left, PixelScalar top,
                       UPixelScalar width, UPixelScalar height,
                       const WindowStyle style)
  :blackboard(_blackboard), look(_look), units_look(_units_look),
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

  set(parent, left, top, width, height, style);
}

void
GaugeVario::OnPaintBuffer(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();
  const UPixelScalar width = rc.right - rc.left;
  const UPixelScalar height = rc.bottom - rc.top;

  if (!is_persistent() || !layout_initialised) {
    UPixelScalar value_height = 4 + look.value_font->GetCapitalHeight()
      + look.text_font->GetCapitalHeight();

    middle_position.y = yoffset - value_height / 2;
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
    fixed mc = Units::ToUserVSpeed(GetGlidePolar().GetMC());
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

  fixed vval = Basic().brutto_vario;
  ival = ValueToNeedlePos(fixed(vval));
  sval = ValueToNeedlePos(Calculated().sink_rate);
  if (Settings().show_average_needle) {
    if (!Calculated().circling)
      ival_av = ValueToNeedlePos(Calculated().netto_average);
    else
      ival_av = ValueToNeedlePos(Calculated().average);
  }

  // clear items first

  if (Settings().show_average_needle) {
    if (!is_persistent() || ival_av != ival_last)
      RenderNeedle(canvas, ival_last, true, true);

    ival_last = ival_av;
  }

  if (!is_persistent() || (sval != sval_last) || (ival != vval_last))
    RenderVarioLine(canvas, vval_last, sval_last, true);

  sval_last = sval;

  if (!is_persistent() || ival != vval_last)
    RenderNeedle(canvas, vval_last, false, true);

  vval_last = ival;

  // now draw items
  RenderVarioLine(canvas, ival, sval, false);
  if (Settings().show_average_needle)
    RenderNeedle(canvas, ival_av, true, false);

  RenderNeedle(canvas, ival, false, false);

  if (Settings().show_gross) {
    fixed vvaldisplay = min(fixed(99.9), max(fixed(-99.9), Units::ToUserVSpeed(vval)));

    RenderValue(canvas, middle_position.x, middle_position.y,
                &value_middle, &label_middle,
                vvaldisplay,
                _T("Gross"));
  }

  RenderZero(canvas);
}

gcc_const
static RasterPoint
TransformRotatedPoint(RasterPoint pt, PixelScalar xoffset, PixelScalar yoffset)
{
  return RasterPoint{ PixelScalar(pt.x + xoffset),
      PixelScalar((pt.y * 112 / 100) + yoffset + 1) };
}

void
GaugeVario::MakePolygon(const int i)
{
  RasterPoint *bit = getPolygon(i);
  RasterPoint *bline = &lines[i + gmax];

  const FastPixelRotation r(Angle::Degrees(fixed(i)));

  bit[0] = TransformRotatedPoint(r.Rotate(-xoffset + nlength0, nwidth),
                                 xoffset, yoffset);
  bit[1] = TransformRotatedPoint(r.Rotate(-xoffset + nlength1, 0),
                                 xoffset, yoffset);
  bit[2] = TransformRotatedPoint(r.Rotate(-xoffset + nlength0, -nwidth),
                                 xoffset, yoffset);

  *bline = TransformRotatedPoint(r.Rotate(-xoffset + nline, 0),
                                 xoffset, yoffset);
}

RasterPoint *
GaugeVario::getPolygon(int i)
{
  return polys + (i + gmax) * 3;
}

void
GaugeVario::MakeAllPolygons()
{
  if (polys && lines)
    for (int i = -gmax; i <= gmax; i++)
      MakePolygon(i);
}

void
GaugeVario::RenderClimb(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();
  PixelScalar x = rc.right - Layout::Scale(14);
  PixelScalar y = rc.bottom - Layout::Scale(24);

  if (!dirty)
    return;

  if (Basic().switch_state.flight_mode == SwitchInfo::FlightMode::CIRCLING)
    canvas.ScaleCopy(x, y, look.climb_bitmap, 12, 0, 12, 12);
  else if (is_persistent())
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

  canvas.DrawLine(0, yoffset, Layout::Scale(17), yoffset);
  canvas.DrawLine(0, yoffset + 1, Layout::Scale(17), yoffset + 1);
}

int
GaugeVario::ValueToNeedlePos(fixed Value)
{
  static fixed degrees_per_unit = fixed(GAUGEVARIOSWEEP) / GAUGEVARIORANGE;
  int i;

  if (!needle_initialised){
    MakeAllPolygons();
    needle_initialised = true;
  }
  i = iround(Value * degrees_per_unit);
  i = min((int)gmax, max(-gmax, i));
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
GaugeVario::RenderValue(Canvas &canvas, PixelScalar x, PixelScalar y,
                        DrawInfo *value_info,
                        DrawInfo *diLabel, fixed Value, const TCHAR *Label)
{
  PixelSize tsize;

#ifndef FIXED_MATH
  Value = (double)iround(Value * 10) / 10; // prevent the -0.0 case
#endif

  if (!value_info->initialised) {

    value_info->rc.right = x - Layout::Scale(5);
    value_info->rc.top = y + Layout::Scale(3)
      + look.text_font->GetCapitalHeight();

    value_info->rc.left = value_info->rc.right;
    // update back rect with max label size
    value_info->rc.bottom = value_info->rc.top + look.value_font->GetCapitalHeight();

    value_info->text_position.x = value_info->rc.left;
    value_info->text_position.y = value_info->rc.top
                         + look.value_font->GetCapitalHeight()
                         - look.value_font->GetAscentHeight();

    value_info->last_value = fixed(-9999);
    value_info->last_text[0] = '\0';
    value_info->last_unit = Unit::UNDEFINED;
    value_info->initialised = true;
  }

  if (!diLabel->initialised) {

    diLabel->rc.right = x;
    diLabel->rc.top = y + Layout::Scale(1);

    diLabel->rc.left = diLabel->rc.right;
    // update back rect with max label size
    diLabel->rc.bottom = diLabel->rc.top
      + look.text_font->GetCapitalHeight();

    diLabel->text_position.x = diLabel->rc.left;
    diLabel->text_position.y = diLabel->rc.top
      + look.text_font->GetCapitalHeight()
      - look.text_font->GetAscentHeight();

    diLabel->last_value = fixed(-9999);
    diLabel->last_text[0] = '\0';
    diLabel->initialised = true;
  }

  canvas.SetBackgroundTransparent();

  if (!is_persistent() || (dirty && _tcscmp(diLabel->last_text, Label) != 0)) {
    canvas.SetTextColor(look.dimmed_text_color);
    canvas.Select(*look.text_font);
    tsize = canvas.CalcTextSize(Label);
    diLabel->text_position.x = diLabel->rc.right - tsize.cx;

    if (is_persistent()) {
      canvas.SetBackgroundColor(look.background_color);
      canvas.text_opaque(diLabel->text_position.x, diLabel->text_position.y,
                         diLabel->rc, Label);
      diLabel->rc.left = diLabel->text_position.x;
      _tcscpy(diLabel->last_text, Label);
    } else {
      canvas.text(diLabel->text_position.x, diLabel->text_position.y, Label);
    }
  }

  if (!is_persistent() || (dirty && value_info->last_value != Value)) {
    TCHAR buffer[18];
    canvas.SetBackgroundColor(look.background_color);
    canvas.SetTextColor(look.text_color);
    _stprintf(buffer, _T("%.1f"), (double)Value);
    canvas.Select(*look.value_font);
    tsize = canvas.CalcTextSize(buffer);
    value_info->text_position.x = value_info->rc.right - tsize.cx;

    if (is_persistent()) {
      canvas.text_opaque(value_info->text_position.x,
                         value_info->text_position.y,
                         value_info->rc, buffer);

      value_info->rc.left = value_info->text_position.x;
      value_info->last_value = Value;
    } else {
      canvas.text(value_info->text_position.x, value_info->text_position.y,
                  buffer);
    }
  }

  if (!is_persistent() ||
      value_info->last_unit != Units::current.vertical_speed_unit) {
    value_info->last_unit = Units::current.vertical_speed_unit;
    const UnitSymbol *unit_symbol = units_look.GetSymbol(value_info->last_unit);
    unit_symbol->Draw(canvas, x - Layout::Scale(5), value_info->rc.top,
                      look.inverse
                      ? UnitSymbol::INVERSE_GRAY
                      : UnitSymbol::GRAY);
  }
}

void
GaugeVario::RenderSpeedToFly(Canvas &canvas, PixelScalar x, PixelScalar y)
{
  if (!Basic().airspeed_available ||
      !Basic().total_energy_vario_available)
    return;

  static fixed lastVdiff;
  fixed vdiff;

  const UPixelScalar arrow_y_size = Layout::Scale(3);
  const UPixelScalar arrow_x_size = Layout::Scale(7);

  const PixelRect rc = GetClientRect();

  PixelScalar nary = NARROWS * arrow_y_size;
  PixelScalar ytop = rc.top + YOFFSET + nary; // JMW
  PixelScalar ybottom = rc.bottom - YOFFSET - nary - Layout::FastScale(1);

  ytop += Layout::Scale(14);
  ybottom -= Layout::Scale(14);

  x = rc.right - 2 * arrow_x_size;

  // only draw speed command if flying and vario is not circling
  if ((Calculated().flight.flying)
      && (!Basic().gps.simulator || !Calculated().circling)) {
    vdiff = Calculated().V_stf - Basic().indicated_airspeed;
    vdiff = max(-DeltaVlimit, min(DeltaVlimit, vdiff)); // limit it
    vdiff = iround(vdiff/DeltaVstep) * DeltaVstep;
  } else
    vdiff = fixed_zero;

  if (!is_persistent() || lastVdiff != vdiff || dirty) {
    lastVdiff = vdiff;

    if (is_persistent()) {
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
      if (positive(vdiff)) {
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

    if (positive(vdiff)) {
      // too slow
      y = ybottom;
      y += YOFFSET;

      while (positive(vdiff)) {
        if (vdiff > DeltaVstep) {
          canvas.Rectangle(x, y,
                           x + arrow_x_size * 2 + 1, y + arrow_y_size - 1);
        } else {
          RasterPoint arrow[3];
          arrow[0].x = x;
          arrow[0].y = y;
          arrow[1].x = x + arrow_x_size;
          arrow[1].y = y + arrow_y_size - 1;
          arrow[2].x = x + 2 * arrow_x_size;
          arrow[2].y = y;
          canvas.DrawTriangleFan(arrow, 3);
        }
        vdiff -= DeltaVstep;
        y += arrow_y_size;
      }
    } else if (negative(vdiff)) {
      // too fast
      y = ytop;
      y -= YOFFSET;

      while (negative(vdiff)) {
        if (vdiff < -DeltaVstep) {
          canvas.Rectangle(x, y + 1,
                           x + arrow_x_size * 2 + 1, y - arrow_y_size + 2);
        } else {
          RasterPoint arrow[3];
          arrow[0].x = x;
          arrow[0].y = y;
          arrow[1].x = x + arrow_x_size;
          arrow[1].y = y - arrow_y_size + 1;
          arrow[2].x = x + 2 * arrow_x_size;
          arrow[2].y = y;
          canvas.DrawTriangleFan(arrow, 3);
        }
        vdiff += DeltaVstep;
        y -= arrow_y_size;
      }
    }
  }
}

void
GaugeVario::RenderBallast(Canvas &canvas)
{
  static unsigned last_ballast = -1;
  static PixelRect recLabelBk = {-1,-1,-1,-1};
  static PixelRect recValueBk = {-1,-1,-1,-1};
  static RasterPoint orgLabel = {-1,-1};
  static RasterPoint orgValue = {-1,-1};

  if (!ballast_initialised) { // ontime init, origin and background rect
    const PixelRect rc = GetClientRect();

    PixelSize tSize;

    // position of ballast label
    orgLabel.x = 1;
    orgLabel.y = rc.top + 2
      + look.text_font->GetCapitalHeight() * 2
      - look.text_font->GetAscentHeight();

    // position of ballast value
    orgValue.x = 1;
    orgValue.y = rc.top + 1
      + look.text_font->GetCapitalHeight()
      - look.text_font->GetAscentHeight();

    // set upper left corner
    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y
      + look.text_font->GetAscentHeight()
      - look.text_font->GetCapitalHeight();

    // set upper left corner
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y
      + look.text_font->GetAscentHeight()
      - look.text_font->GetCapitalHeight();

    // get max label size
    canvas.Select(*look.text_font);
    tSize = canvas.CalcTextSize(TextBal);

    // update back rect with max label size
    recLabelBk.right = recLabelBk.left + tSize.cx;
    recLabelBk.bottom = recLabelBk.top +
      look.text_font->GetCapitalHeight();

    // get max value size
    tSize = canvas.CalcTextSize(_T("100%"));

    recValueBk.right = recValueBk.left + tSize.cx;
    // update back rect with max label size
    recValueBk.bottom = recValueBk.top +
      look.text_font->GetCapitalHeight();

    ballast_initialised = true;
  }

  unsigned ballast = uround(GetGlidePolar().GetBugs() * 100);

  if (!is_persistent() || ballast != last_ballast) {
    // ballast hase been changed

    canvas.Select(*look.text_font);

    if (is_persistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    if (is_persistent() || last_ballast == 0 || ballast == 0) {
      // new ballast is 0, hide label
      if (ballast > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        // ols ballast was 0, show label
        if (is_persistent())
          canvas.text_opaque(orgLabel.x, orgLabel.y, recLabelBk, TextBal);
        else
          canvas.text(orgLabel.x, orgLabel.y, TextBal);
      } else if (is_persistent())
        canvas.DrawFilledRectangle(recLabelBk, look.background_color);
    }

    // new ballast 0, hide value
    if (ballast > 0) {
      TCHAR buffer[18];
      _stprintf(buffer, _T("%u%%"), ballast);
      canvas.SetTextColor(look.text_color);

      if (is_persistent())
        canvas.text_opaque(orgValue.x, orgValue.y, recValueBk, buffer);
      else
        canvas.text(orgValue.x, orgValue.y, buffer);
    } else if (is_persistent())
      canvas.DrawFilledRectangle(recValueBk, look.background_color);

    if (is_persistent())
      last_ballast = ballast;
  }
}

void
GaugeVario::RenderBugs(Canvas &canvas)
{
  static int last_bugs = -1;
  static PixelRect recLabelBk = {-1,-1,-1,-1};
  static PixelRect recValueBk = {-1,-1,-1,-1};
  static RasterPoint orgLabel = {-1,-1};
  static RasterPoint orgValue = {-1,-1};

  if (!bugs_initialised) {
    const PixelRect rc = GetClientRect();
    PixelSize tSize;

    orgLabel.x = 1;
    orgLabel.y = rc.bottom - 2
      - look.text_font->GetCapitalHeight()
      - look.text_font->GetAscentHeight();

    orgValue.x = 1;
    orgValue.y = rc.bottom - 1
      - look.text_font->GetAscentHeight();

    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y
      + look.text_font->GetAscentHeight()
      - look.text_font->GetCapitalHeight();
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y
      + look.text_font->GetAscentHeight()
      - look.text_font->GetCapitalHeight();

    canvas.Select(*look.text_font);
    tSize = canvas.CalcTextSize(TextBug);

    recLabelBk.right = recLabelBk.left + tSize.cx;
    recLabelBk.bottom = recLabelBk.top
      + look.text_font->GetCapitalHeight()
      + look.text_font->GetHeight()
      - look.text_font->GetAscentHeight();

    tSize = canvas.CalcTextSize(_T("100%"));

    recValueBk.right = recValueBk.left + tSize.cx;
    recValueBk.bottom = recValueBk.top +
      look.text_font->GetCapitalHeight();

    bugs_initialised = true;
  }

  int bugs = iround((fixed_one - GetComputerSettings().polar.bugs) * 100);
  if (!is_persistent() || bugs != last_bugs) {

    canvas.Select(*look.text_font);

    if (is_persistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    if (is_persistent() || last_bugs < 1 || bugs < 1) {
      if (bugs > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        if (is_persistent())
          canvas.text_opaque(orgLabel.x, orgLabel.y, recLabelBk, TextBug);
        else
          canvas.text(orgLabel.x, orgLabel.y, TextBug);
      } else if (is_persistent())
        canvas.DrawFilledRectangle(recLabelBk, look.background_color);
    }

    if (bugs > 0) {
      TCHAR buffer[18];
      _stprintf(buffer, _T("%d%%"), bugs);
      canvas.SetTextColor(look.text_color);
      if (is_persistent())
        canvas.text_opaque(orgValue.x, orgValue.y, recValueBk, buffer);
      else 
        canvas.text(orgValue.x, orgValue.y, buffer);
    } else if (is_persistent())
      canvas.DrawFilledRectangle(recValueBk, look.background_color);

    if (is_persistent())
      last_bugs = bugs;
  }
}

void
GaugeVario::OnResize(UPixelScalar width, UPixelScalar height)
{
  BufferWindow::OnResize(width, height);

  /* trigger reinitialisation */
  xoffset = width;
  yoffset = height / 2;
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
