// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/LegendMapping.hpp"
#include "TestUtil.hpp"

static void
TestBipolarDeadZone()
{
  std::map<float, SkySight::LegendColor> legend{
    {-0.2f, {81, 201, 11}},
    {0.2f, {168, 228, 5}},
    {0.6f, {255, 255, 0}},
    {1.0f, {255, 0, 0}},
  };

  /* Below first stop → transparent. */
  ok1(SkySight::FindLegendColor(legend, -0.3f) == nullptr);

  /* Last non-positive band [-0.2, 0.2) → transparent (ridge wash fix). */
  ok1(SkySight::FindLegendColor(legend, -0.2f) == nullptr);
  ok1(SkySight::FindLegendColor(legend, -0.01f) == nullptr);
  ok1(SkySight::FindLegendColor(legend, 0.f) == nullptr);
  ok1(SkySight::FindLegendColor(legend, 0.19f) == nullptr);

  /* Positive lift uses the positive stops. */
  const auto *lift = SkySight::FindLegendColor(legend, 0.2f);
  ok1(lift != nullptr);
  ok1(lift->red == 168 && lift->green == 228 && lift->blue == 5);

  const auto *strong = SkySight::FindLegendColor(legend, 0.9f);
  ok1(strong != nullptr);
  ok1(strong->red == 255 && strong->green == 255 && strong->blue == 0);

  const auto *max = SkySight::FindLegendColor(legend, 2.f);
  ok1(max != nullptr);
  ok1(max->red == 255 && max->green == 0 && max->blue == 0);
}

static void
TestNegativeStopsStillPaint()
{
  std::map<float, SkySight::LegendColor> legend{
    {-0.6f, {0, 39, 255}},
    {-0.2f, {81, 201, 11}},
    {0.2f, {168, 228, 5}},
  };

  const auto *sink = SkySight::FindLegendColor(legend, -0.5f);
  ok1(sink != nullptr);
  ok1(sink->red == 0 && sink->green == 39 && sink->blue == 255);

  /* -0.2 band remains the dead zone. */
  ok1(SkySight::FindLegendColor(legend, -0.2f) == nullptr);
}

static void
TestAllPositiveLegend()
{
  std::map<float, SkySight::LegendColor> legend{
    {0.5f, {10, 20, 30}},
    {1.5f, {40, 50, 60}},
  };

  ok1(SkySight::FindLegendColor(legend, 0.4f) == nullptr);

  const auto *mid = SkySight::FindLegendColor(legend, 0.5f);
  ok1(mid != nullptr);
  ok1(mid->red == 10);

  const auto *hi = SkySight::FindLegendColor(legend, 2.f);
  ok1(hi != nullptr);
  ok1(hi->red == 40);
}

int
main()
{
  plan_tests(11 + 3 + 5);
  TestBipolarDeadZone();
  TestNegativeStopsStillPaint();
  TestAllPositiveLegend();
  return exit_status();
}
