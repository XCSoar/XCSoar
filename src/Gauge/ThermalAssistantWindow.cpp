/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "ThermalAssistantWindow.hpp"

#include "NMEA/Derived.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Units.hpp"
#include "Language.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

const Color ThermalAssistantWindow::hcBackground(0xFF, 0xFF, 0xFF);
const Color ThermalAssistantWindow::hcCircles(0xB0, 0xB0, 0xB0);
const Color ThermalAssistantWindow::hcStandard(0x00, 0x00, 0x00);
const Color ThermalAssistantWindow::hcPolygonBrush(0xCC, 0xCC, 0xFF);
const Color ThermalAssistantWindow::hcPolygonPen(0x00, 0x00, 0xFF);

ThermalAssistantWindow::ThermalAssistantWindow(unsigned _padding, bool _small)
  :max_lift(fixed_one),
   padding(_padding),
   small(_small),
   direction(Angle::radians(fixed_zero))
{
  for (unsigned i = 0; i <= 36; i++) {
    lift_points[i].x = 0;
    lift_points[i].y = 0;
  }
}

bool
ThermalAssistantWindow::on_create()
{
  BufferWindow::on_create();

  hbBackground.set(hcBackground);

#ifdef ENABLE_OPENGL
  hbPolygon.set(hcPolygonBrush.with_alpha(128));
#else /* !OPENGL */
  hbPolygon.set(hcPolygonBrush);
#endif /* !OPENGL */

  int width = Layout::FastScale(small ? 1 : 2);
#ifdef ENABLE_OPENGL
  hpPolygon.set(width, hcPolygonPen.with_alpha(128));
#else /* !OPENGL */
  hpPolygon.set(width, hcPolygonPen);
#endif /* !OPENGL */
  hpInnerCircle.set(1, hcCircles);
  hpOuterCircle.set(Pen::DASH, 1, hcCircles);
  hpPlane.set(width, hcCircles);

  hfNoTraffic.set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  hfLabels.set(Fonts::GetStandardFontFace(), Layout::FastScale(12));

  return true;
}

bool
ThermalAssistantWindow::on_destroy()
{
  hfNoTraffic.reset();
  hfLabels.reset();

  BufferWindow::on_destroy();
  return true;
}

bool
ThermalAssistantWindow::on_resize(unsigned width, unsigned height)
{
  BufferWindow::on_resize(width, height);

  // Calculate Radar size
  radius = min(height, width) / 2 - padding;
  mid.x = width / 2;
  mid.y = height / 2;

  return true;
}

bool
ThermalAssistantWindow::LeftTurn() const
{
  return negative(derived.SmoothedTurnRate);
}

void
ThermalAssistantWindow::Update(const Angle &_direction,
                               const DERIVED_INFO &_derived)
{
  direction = _direction;
  derived = _derived;

  UpdateLiftMax();
  UpdateLiftPoints();

  invalidate();
}

void
ThermalAssistantWindow::UpdateLiftMax()
{
  max_lift = fixed_one;

  for (unsigned i = 0; i < 36; i++)
    max_lift = std::max(max_lift, fabs(derived.LiftDatabase[i]));

  max_lift = ceil(max_lift);
}

void
ThermalAssistantWindow::UpdateLiftPoints()
{
  lift_point_avg.x = 0;
  lift_point_avg.y = 0;

  for (unsigned i = 0; i < 36; i++) {
    Angle d = Angle::degrees(fixed(i * 10));

    lift_points[i].x = (int)((d - direction).cos() *
                       RangeScale(derived.LiftDatabase[i]));
    lift_points[i].y = (int)((d - direction).sin() *
                       RangeScale(derived.LiftDatabase[i]));

    if (!LeftTurn()) {
      lift_points[i].x *= -1;
      lift_points[i].y *= -1;
    }

    lift_points[i].x += mid.x;
    lift_points[i].y += mid.y;

    lift_point_avg.x += lift_points[i].x;
    lift_point_avg.y += lift_points[i].y;
  }
  lift_points[36] = lift_points[0];

  lift_point_avg.x /= 36;
  lift_point_avg.y /= 36;
}

fixed
ThermalAssistantWindow::RangeScale(fixed lift) const
{
  lift = (lift + max_lift) / (max_lift * fixed_two);
  return std::min(fixed_one, std::max(fixed_zero, lift)) * fixed(radius);
}

void
ThermalAssistantWindow::PaintRadarPlane(Canvas &canvas) const
{
  canvas.select(hpPlane);

  int x = mid.x + (LeftTurn() ? radius : -radius);

  canvas.line(x + Layout::FastScale(small ? 5 : 10),
              mid.y - Layout::FastScale(small ? 1 : 2),
              x - Layout::FastScale(small ? 5 : 10),
              mid.y - Layout::FastScale(small ? 1 : 2));
  canvas.line(x,
              mid.y - Layout::FastScale(small ? 3 : 6),
              x,
              mid.y + Layout::FastScale(small ? 3 : 6));
  canvas.line(x + Layout::FastScale(small ? 2 : 4),
              mid.y + Layout::FastScale(small ? 2 : 4),
              x - Layout::FastScale(small ? 2 : 4),
              mid.y + Layout::FastScale(small ? 2 : 4));
}

void
ThermalAssistantWindow::PaintRadarBackground(Canvas &canvas) const
{
  canvas.clear(hbBackground);
  canvas.hollow_brush();

  canvas.select(hpInnerCircle);
  canvas.circle(mid.x, mid.y, radius / 2);
  canvas.select(hpOuterCircle);
  canvas.circle(mid.x, mid.y, radius);

  if (small)
    return;

  canvas.set_text_color(hcCircles);
  canvas.select(hfLabels);
  canvas.set_background_color(hcBackground);
  canvas.background_opaque();

  TCHAR lift_string[10];
  Units::FormatUserVSpeed(max_lift, lift_string,
                            sizeof(lift_string) / sizeof(lift_string[0]));
  SIZE s = canvas.text_size(lift_string);
  canvas.text(mid.x - s.cx / 2,
              mid.y + radius - s.cy * 0.75, lift_string);

  Units::FormatUserVSpeed(fixed_zero, lift_string,
                            sizeof(lift_string) / sizeof(lift_string[0]));
  s = canvas.text_size(lift_string);
  canvas.text(mid.x - s.cx / 2,
              mid.y + radius / 2 - s.cy * 0.75, lift_string);

  canvas.background_transparent();
}

void
ThermalAssistantWindow::PaintPoints(Canvas &canvas) const
{
#ifdef ENABLE_OPENGL
  GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#elif !defined(ENABLE_SDL)
  canvas.mix_mask();
#endif /* GDI */

  canvas.select(hbPolygon);
  canvas.select(hpPolygon);
  canvas.polygon(lift_points, 36);
}

void
ThermalAssistantWindow::PaintAdvisor(Canvas &canvas) const
{
  canvas.line(mid.x, mid.y, lift_point_avg.x, lift_point_avg.y);
}

void
ThermalAssistantWindow::PaintNotCircling(Canvas &canvas) const
{
  if (small)
    return;

  const TCHAR* str = _("Not Circling");
  canvas.select(hfNoTraffic);
  SIZE ts = canvas.text_size(str);
  canvas.set_text_color(hcStandard);
  canvas.text(mid.x - (ts.cx / 2), mid.y - (radius / 2), str);
}

void
ThermalAssistantWindow::on_paint_buffer(Canvas &canvas)
{
  PaintRadarBackground(canvas);
  if (!derived.Circling) {
    PaintNotCircling(canvas);
    return;
  }

  PaintRadarPlane(canvas);
  PaintPoints(canvas);
  PaintAdvisor(canvas);
}
