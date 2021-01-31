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

#include "ThermalAssistantRenderer.hpp"
#include "util/Macros.hpp"
#include "util/Clamp.hpp"
#include "NMEA/Attitude.hpp"
#include "NMEA/Derived.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"
#include "Look/ThermalAssistantLook.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

PixelPoint
ThermalAssistantRenderer::LiftPoints::GetAverage() const
{
  PixelPoint avg(0, 0);

  for (auto it = begin(), it_end = end(); it != it_end; ++it) {
    avg.x += it->x;
    avg.y += it->y;
  }

  avg.x /= size();
  avg.y /= size();

  return avg;
}

ThermalAssistantRenderer::ThermalAssistantRenderer(const ThermalAssistantLook &_look,
                                                   unsigned _padding, bool _small)
  :look(_look),
   padding(_padding),
   small(_small),
   direction(Angle::Zero()) {}

void
ThermalAssistantRenderer::Update(const AttitudeState &attitude,
                               const DerivedInfo &derived)
{
  direction = attitude.heading;
  circling = (CirclingInfo)derived;
  vario = (VarioInfo)derived;
}

double
ThermalAssistantRenderer::CalculateMaxLift() const
{
  return std::max(1.,
                  *std::max_element(vario.lift_database.begin(),
                                    vario.lift_database.end()));
}

void
ThermalAssistantRenderer::CalculateLiftPoints(LiftPoints &lift_points,
                                              double max_lift) const
{
  Angle angle = -direction;
  constexpr Angle delta = Angle::FullCircle() / unsigned(std::tuple_size<LiftDatabase>());

  for (unsigned i = 0; i < lift_points.size(); i++, angle += delta) {
    auto sincos = angle.SinCos();
    double scale = NormalizeLift(vario.lift_database[i], max_lift) * radius;

    lift_points[i].x = (int)(sincos.second * scale);
    lift_points[i].y = (int)(sincos.first * scale);

    if (!circling.TurningLeft()) {
      lift_points[i].x *= -1;
      lift_points[i].y *= -1;
    }

    lift_points[i].x += mid.x;
    lift_points[i].y += mid.y;
  }
}

double
ThermalAssistantRenderer::NormalizeLift(double lift, double max_lift)
{
  lift = (lift + max_lift) / (2 * max_lift);
  return Clamp(lift, 0., 1.);
}

void
ThermalAssistantRenderer::PaintRadarPlane(Canvas &canvas) const
{
  canvas.Select(look.plane_pen);

  PixelPoint p = mid.At(circling.TurningLeft() ? (int)radius : (int)-radius,
                        0);

  canvas.DrawLine(p.At(+Layout::FastScale(small ? 5 : 10),
                       -Layout::FastScale(small ? 1 : 2)),
                  p.At(-Layout::FastScale(small ? 5 : 10),
                       -Layout::FastScale(small ? 1 : 2)));
  canvas.DrawLine(p.At(0, -Layout::FastScale(small ? 3 : 6)),
                  p.At(0, +Layout::FastScale(small ? 3 : 6)));
  canvas.DrawLine(p.At(+Layout::FastScale(small ? 2 : 4),
                       +Layout::FastScale(small ? 2 : 4)),
                  p.At(-Layout::FastScale(small ? 2 : 4),
                       +Layout::FastScale(small ? 2 : 4)));
}

static void
DrawCircleLabel(Canvas &canvas, PixelPoint p,
                BasicStringView<TCHAR> text) noexcept
{
  const auto size = canvas.CalcTextSize(text);
  p.x -= size.width / 2;
  p.y -= size.height * 3 / 4;

  canvas.DrawText(p, text);
}

static void
DrawCircleLabelVSpeed(Canvas &canvas, PixelPoint p, double value) noexcept
{
  TCHAR buffer[10];
  FormatUserVerticalSpeed(value, buffer, ARRAY_SIZE(buffer));
  DrawCircleLabel(canvas, p, buffer);
}

void
ThermalAssistantRenderer::PaintRadarBackground(Canvas &canvas, double max_lift) const
{
  canvas.SelectHollowBrush();

  canvas.Select(look.inner_circle_pen);
  canvas.DrawCircle(mid, radius / 2);
  canvas.Select(look.outer_circle_pen);
  canvas.DrawCircle(mid, radius);

  if (small)
    return;

  canvas.SetTextColor(COLOR_BLACK);
  canvas.Select(look.circle_label_font);
  canvas.SetBackgroundColor(look.background_color);
  canvas.SetBackgroundOpaque();

  DrawCircleLabelVSpeed(canvas, mid + PixelSize{0u, radius}, max_lift);
  DrawCircleLabelVSpeed(canvas, mid + PixelSize{0u, radius / 2}, 0);

  canvas.SetBackgroundTransparent();
}

void
ThermalAssistantRenderer::PaintPoints(Canvas &canvas,
                                    const LiftPoints &lift_points) const
{
#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#elif defined(USE_GDI)
  canvas.SetMixMask();
#endif /* GDI */

  canvas.Select(look.polygon_brush);
  canvas.Select(look.polygon_pen);
  canvas.DrawPolygon(lift_points.data(), lift_points.size());
}

void
ThermalAssistantRenderer::PaintAdvisor(Canvas &canvas,
                                     const LiftPoints &lift_points) const
{
  canvas.DrawLine(mid, lift_points.GetAverage());
}

void
ThermalAssistantRenderer::PaintNotCircling(Canvas &canvas) const
{
  if (small)
    return;

  const TCHAR* str = _("Not Circling");
  canvas.Select(look.overlay_font);
  canvas.SetTextColor(look.text_color);

  DrawCircleLabel(canvas, mid - PixelSize{0u, radius / 2}, str);
}

void
ThermalAssistantRenderer::UpdateLayout(const PixelRect &rc)
{
  radius = std::min(rc.GetWidth(), rc.GetHeight()) / 2 - padding;
  mid = rc.GetCenter();
}

void
ThermalAssistantRenderer::Paint(Canvas &canvas)
{
  double max_lift = ceil(CalculateMaxLift());

  PaintRadarBackground(canvas, max_lift);
  if (!circling.circling) {
    PaintNotCircling(canvas);
    return;
  }

  LiftPoints lift_points;
  CalculateLiftPoints(lift_points, max_lift);
  PaintPoints(canvas, lift_points);
  PaintAdvisor(canvas, lift_points);

  PaintRadarPlane(canvas);
}
