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

#include "ThermalAssistantWindow.hpp"
#include "Util/Macros.hpp"
#include "NMEA/Derived.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"

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
   direction(Angle::Zero())
{
  for (unsigned i = 0; i <= 36; i++) {
    lift_points[i].x = 0;
    lift_points[i].y = 0;
  }
}

void
ThermalAssistantWindow::OnCreate()
{
  BufferWindow::OnCreate();

  hbBackground.Set(hcBackground);

#ifdef ENABLE_OPENGL
  hbPolygon.Set(hcPolygonBrush.WithAlpha(128));
#else /* !OPENGL */
  hbPolygon.Set(hcPolygonBrush);
#endif /* !OPENGL */

  UPixelScalar width = Layout::FastScale(small ? 1 : 2);
#ifdef ENABLE_OPENGL
  hpPolygon.Set(width, hcPolygonPen.WithAlpha(128));
#else /* !OPENGL */
  hpPolygon.Set(width, hcPolygonPen);
#endif /* !OPENGL */
  hpInnerCircle.Set(1, hcCircles);
  hpOuterCircle.Set(Pen::DASH, 1, hcCircles);
  hpPlane.Set(width, COLOR_BLACK);

  hfNoTraffic.Set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  hfLabels.Set(Fonts::GetStandardFontFace(), Layout::FastScale(12));
}

void
ThermalAssistantWindow::OnDestroy()
{
  hfNoTraffic.Reset();
  hfLabels.Reset();

  BufferWindow::OnDestroy();
}

void
ThermalAssistantWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  BufferWindow::OnResize(width, height);

  // Calculate Radar size
  radius = min(height, width) / 2 - padding;
  mid.x = width / 2;
  mid.y = height / 2;
}

bool
ThermalAssistantWindow::LeftTurn() const
{
  return derived.TurningLeft();
}

void
ThermalAssistantWindow::Update(const Angle &_direction,
                               const DerivedInfo &_derived)
{
  direction = _direction;
  derived = _derived;

  UpdateLiftMax();
  UpdateLiftPoints();

  Invalidate();
}

void
ThermalAssistantWindow::UpdateLiftMax()
{
  max_lift = fixed_one;

  for (unsigned i = 0; i < 36; i++)
    max_lift = std::max(max_lift, fabs(derived.lift_database[i]));

  max_lift = ceil(max_lift);
}

void
ThermalAssistantWindow::UpdateLiftPoints()
{
  lift_point_avg.x = 0;
  lift_point_avg.y = 0;

  for (unsigned i = 0; i < 36; i++) {
    Angle d = Angle::Degrees(fixed(i * 10));

    lift_points[i].x = (int)((d - direction).cos() *
                       RangeScale(derived.lift_database[i]));
    lift_points[i].y = (int)((d - direction).sin() *
                       RangeScale(derived.lift_database[i]));

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
  lift = (lift + max_lift) / Double(max_lift);
  return std::min(fixed_one, std::max(fixed_zero, lift)) * fixed(radius);
}

void
ThermalAssistantWindow::PaintRadarPlane(Canvas &canvas) const
{
  canvas.Select(hpPlane);

  PixelScalar x = mid.x + (LeftTurn() ? radius : -radius);

  canvas.DrawLine(x + Layout::FastScale(small ? 5 : 10),
              mid.y - Layout::FastScale(small ? 1 : 2),
              x - Layout::FastScale(small ? 5 : 10),
              mid.y - Layout::FastScale(small ? 1 : 2));
  canvas.DrawLine(x,
              mid.y - Layout::FastScale(small ? 3 : 6),
              x,
              mid.y + Layout::FastScale(small ? 3 : 6));
  canvas.DrawLine(x + Layout::FastScale(small ? 2 : 4),
              mid.y + Layout::FastScale(small ? 2 : 4),
              x - Layout::FastScale(small ? 2 : 4),
              mid.y + Layout::FastScale(small ? 2 : 4));
}

void
ThermalAssistantWindow::PaintRadarBackground(Canvas &canvas) const
{
  canvas.Clear(hbBackground);
  canvas.SelectHollowBrush();

  canvas.Select(hpInnerCircle);
  canvas.DrawCircle(mid.x, mid.y, radius / 2);
  canvas.Select(hpOuterCircle);
  canvas.DrawCircle(mid.x, mid.y, radius);

  if (small)
    return;

  canvas.SetTextColor(COLOR_BLACK);
  canvas.Select(hfLabels);
  canvas.SetBackgroundColor(hcBackground);
  canvas.SetBackgroundOpaque();

  TCHAR lift_string[10];
  FormatUserVerticalSpeed(max_lift, lift_string, ARRAY_SIZE(lift_string));
  PixelSize s = canvas.CalcTextSize(lift_string);
  canvas.text(mid.x - s.cx / 2,
              mid.y + radius - s.cy * 0.75, lift_string);

  FormatUserVerticalSpeed(fixed_zero, lift_string, ARRAY_SIZE(lift_string));
  s = canvas.CalcTextSize(lift_string);
  canvas.text(mid.x - s.cx / 2,
              mid.y + radius / 2 - s.cy * 0.75, lift_string);

  canvas.SetBackgroundTransparent();
}

void
ThermalAssistantWindow::PaintPoints(Canvas &canvas) const
{
#ifdef ENABLE_OPENGL
  GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#elif defined(USE_GDI)
  canvas.SetMixMask();
#endif /* GDI */

  canvas.Select(hbPolygon);
  canvas.Select(hpPolygon);
  canvas.DrawPolygon(lift_points, 36);
}

void
ThermalAssistantWindow::PaintAdvisor(Canvas &canvas) const
{
  canvas.DrawLine(mid.x, mid.y, lift_point_avg.x, lift_point_avg.y);
}

void
ThermalAssistantWindow::PaintNotCircling(Canvas &canvas) const
{
  if (small)
    return;

  const TCHAR* str = _("Not Circling");
  canvas.Select(hfNoTraffic);
  PixelSize ts = canvas.CalcTextSize(str);
  canvas.SetTextColor(hcStandard);
  canvas.text(mid.x - (ts.cx / 2), mid.y - (radius / 2), str);
}

void
ThermalAssistantWindow::OnPaintBuffer(Canvas &canvas)
{
  PaintRadarBackground(canvas);
  if (!derived.circling) {
    PaintNotCircling(canvas);
    return;
  }

  PaintRadarPlane(canvas);
  PaintPoints(canvas);
  PaintAdvisor(canvas);
}
