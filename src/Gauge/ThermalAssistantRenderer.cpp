// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalAssistantRenderer.hpp"
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

#include <algorithm> // for std::clamp()
#include <numeric> // for std::accumulate()

inline PixelPoint
ThermalAssistantRenderer::LiftPoints::GetAverage() const noexcept
{
  return std::accumulate(begin(), end(), PixelPoint{}) / size();
}

ThermalAssistantRenderer::ThermalAssistantRenderer(const ThermalAssistantLook &_look,
                                                   unsigned _padding, bool _small)
  :look(_look),
   radar_renderer(_padding),
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
  constexpr Angle delta = Angle::FullCircle() / std::tuple_size<LiftDatabase>();

  const BulkPixelPoint mid{radar_renderer.GetCenter()};
  for (unsigned i = 0; i < lift_points.size(); i++, angle += delta) {
    auto sincos = angle.SinCos();
    double scale = NormalizeLift(vario.lift_database[i], max_lift) * radar_renderer.GetRadius();

    lift_points[i] = {
      (int)(sincos.second * scale),
      (int)(sincos.first * scale),
    };

    if (!circling.TurningLeft())
      lift_points[i] = -lift_points[i];

    lift_points[i] += mid;
  }
}

inline constexpr double
ThermalAssistantRenderer::NormalizeLift(double lift, double max_lift) noexcept
{
  lift = (lift + max_lift) / (2 * max_lift);
  return std::clamp(lift, 0., 1.);
}

static void
DrawCircleLabel(Canvas &canvas, PixelPoint p,
                tstring_view text) noexcept
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
  FormatUserVerticalSpeed(value, buffer);
  DrawCircleLabel(canvas, p, buffer);
}

void
ThermalAssistantRenderer::PaintRadarPlane(Canvas &canvas, double max_lift) const
{
  int normalised_average = NormalizeLift(vario.average, max_lift) * radar_renderer.GetRadius();

  canvas.Select(look.plane_pen);

  PixelPoint p = radar_renderer.GetCenter()
    .At(circling.TurningLeft() ? normalised_average : -normalised_average, 0);

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

void
ThermalAssistantRenderer::PaintRadarBackground(Canvas &canvas, double max_lift) const
{
  const unsigned radius = radar_renderer.GetRadius();

  canvas.SelectHollowBrush();

  canvas.Select(look.inner_circle_pen);
  radar_renderer.DrawCircle(canvas, radius / 2);
  canvas.Select(look.outer_circle_pen);
  radar_renderer.DrawCircle(canvas, radius);

  if (small)
    return;

  canvas.SetTextColor(look.text_color);
  canvas.Select(look.circle_label_font);
  canvas.SetBackgroundColor(look.background_color);
  canvas.SetBackgroundOpaque();

  DrawCircleLabelVSpeed(canvas,
                        radar_renderer.GetCenter().At(0, radius),
                        max_lift);
  DrawCircleLabelVSpeed(canvas,
                        radar_renderer.GetCenter().At(0, radius / 2),
                        0);

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
  canvas.DrawLine(radar_renderer.GetCenter(), lift_points.GetAverage());
}

void
ThermalAssistantRenderer::PaintNotCircling(Canvas &canvas) const
{
  if (small)
    return;

  const TCHAR* str = _("Not Circling");
  canvas.Select(look.overlay_font);
  canvas.SetTextColor(look.text_color);

  DrawCircleLabel(canvas,
                  radar_renderer.GetCenter().At(0u, radar_renderer.GetRadius() / 2),
                  str);
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

  PaintRadarPlane(canvas,max_lift);
}
