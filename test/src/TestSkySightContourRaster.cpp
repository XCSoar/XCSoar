// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/FieldImage.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <map>

static SkySight::ScalarField
MakeRow(std::initializer_list<float> values)
{
  SkySight::ScalarField field;
  field.values = values;
  field.width = unsigned(values.size());
  field.height = 1;
  return field;
}

static void
TestBipolarDeadZone()
{
  const SkySight::ContourPalette palette{{
    {-0.2f, {81, 201, 11}},
    {0.2f, {168, 228, 5}},
    {0.6f, {255, 255, 0}},
    {1.0f, {255, 0, 0}},
  }};

  /* Below first stop → transparent. */
  ok1(palette.Find(-0.3f) == nullptr);

  /* Last non-positive band [-0.2, 0.2) → transparent (ridge wash fix). */
  ok1(palette.Find(-0.2f) == nullptr);
  ok1(palette.Find(-0.01f) == nullptr);
  ok1(palette.Find(0.f) == nullptr);
  ok1(palette.Find(0.19f) == nullptr);

  /* Positive lift uses the positive stops. */
  const auto *lift = palette.Find(0.2f);
  ok1(lift != nullptr);
  ok1(lift->red == 168 && lift->green == 228 && lift->blue == 5);

  const auto *strong = palette.Find(0.9f);
  ok1(strong != nullptr);
  ok1(strong->red == 255 && strong->green == 255 && strong->blue == 0);

  const auto *max = palette.Find(2.f);
  ok1(max != nullptr);
  ok1(max->red == 255 && max->green == 0 && max->blue == 0);
}

static void
TestNegativeStopsStillPaint()
{
  const SkySight::ContourPalette palette{{
    {-0.6f, {0, 39, 255}},
    {-0.2f, {81, 201, 11}},
    {0.2f, {168, 228, 5}},
  }};

  const auto *sink = palette.Find(-0.5f);
  ok1(sink != nullptr);
  ok1(sink->red == 0 && sink->green == 39 && sink->blue == 255);

  /* -0.2 band remains the dead zone. */
  ok1(palette.Find(-0.2f) == nullptr);
}

static void
TestAllPositiveLegend()
{
  const SkySight::ContourPalette palette{{
    {0.5f, {10, 20, 30}},
    {1.5f, {40, 50, 60}},
  }};

  ok1(palette.Find(0.4f) == nullptr);

  const auto *mid = palette.Find(0.5f);
  ok1(mid != nullptr);
  ok1(mid->red == 10);

  const auto *hi = palette.Find(2.f);
  ok1(hi != nullptr);
  ok1(hi->red == 40);
}

static void
TestPaletteRejectsMissingSamples()
{
  const SkySight::ContourPalette palette{{{0.5f, {10, 20, 30}}}};

  ok1(!palette.empty());
  ok1(palette.Find(1.f) != nullptr);

  /* Missing and infinite samples never select a band. */
  ok1(palette.Find(std::nanf("")) == nullptr);
  ok1(palette.Find(std::numeric_limits<float>::infinity()) == nullptr);

  const std::map<float, SkySight::LegendColor> no_stops;
  const SkySight::ContourPalette empty_palette{no_stops};
  ok1(empty_palette.empty());
}

static void
TestRasterBudget()
{
  constexpr unsigned MAX_AXIS = 4096;
  constexpr std::size_t MAX_CELLS = 4 * 1024 * 1024;

  /* A small patch gets the full factor. */
  const auto small = SkySight::FitContourRaster({0, 0, 10, 10}, 8, MAX_AXIS,
                                                MAX_CELLS);
  ok1(small.raster_width == 80 && small.raster_height == 80);

  /* A patch that would overrun the axis is shrunk to fit it. */
  const auto wide = SkySight::FitContourRaster({0, 0, 1000, 10}, 8, MAX_AXIS,
                                               MAX_CELLS);
  ok1(wide.raster_width <= MAX_AXIS && wide.raster_height >= 1);

  /* The cell budget binds even when both axes fit. */
  const auto big = SkySight::FitContourRaster({0, 0, 1000, 1000}, 8, MAX_AXIS,
                                              MAX_CELLS);
  ok1(big.raster_width <= MAX_AXIS && big.raster_height <= MAX_AXIS);
  ok1(std::size_t(big.raster_width) * big.raster_height <= MAX_CELLS);

  /* A zoomed-out patch larger than the budget is decimated, not cropped:
     it keeps every sample column it covers. */
  const auto region = SkySight::FitContourRaster({0, 0, 3002, 1504}, 1,
                                                 MAX_AXIS, MAX_CELLS);
  ok1(region.width == 3002 && region.height == 1504);
  ok1(region.raster_width < 3002);
  ok1(std::size_t(region.raster_width) * region.raster_height <= MAX_CELLS);

  /* A device with a 2048 texture limit must not be handed more. */
  const auto vc4 = SkySight::FitContourRaster({0, 0, 3002, 1504}, 1, 2048,
                                              MAX_CELLS);
  ok1(vc4.raster_width <= 2048 && vc4.raster_height <= 2048);
  ok1(vc4.raster_height >= 1);

  /* Aspect is preserved, so the patch is not squashed. */
  const double source_aspect = 3002. / 1504;
  const double raster_aspect = double(vc4.raster_width) / vc4.raster_height;
  ok1(std::fabs(source_aspect - raster_aspect) < 0.01);

  /* Degenerate input never divides by zero. */
  ok1(SkySight::FitContourRaster({0, 0, 0, 10}, 4, MAX_AXIS,
                                 MAX_CELLS).empty());
  ok1(SkySight::FitContourRaster({0, 0, 10, 10}, 4, 0, MAX_CELLS).empty());
  ok1(SkySight::FitContourRaster({0, 0, 10, 10}, 4, MAX_AXIS, 0).empty());

  /* An upsample of zero behaves like one rather than collapsing. */
  const auto zero = SkySight::FitContourRaster({0, 0, 10, 10}, 0, MAX_AXIS,
                                               MAX_CELLS);
  ok1(zero.raster_width == 10 && zero.raster_height == 10);
}

static void
TestRasterizerDecimates()
{
  /* Eight samples into a four-pixel raster: each output pixel covers two
     samples, and the whole patch is still represented.  The centres sit
     clear of the edges, where sample repetition would bend the ramp. */
  const auto field = MakeRow({0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f});
  const SkySight::ContourPalette palette{{
    {0.5f, {10, 0, 0}}, {4.5f, {20, 0, 0}},
  }};
  const SkySight::ContourRasterizer rasterizer{field, palette,
                                               {0, 0, 8, 1, 4, 1}};

  ok1(rasterizer.GetWidth() == 4);
  ok1(rasterizer.GetHeight() == 1);

  /* Pixel i covers samples [2i, 2i+2), so its centre is at index
     2i + 0.5.  Only the interior pixels are checked: at the rim the
     repeated edge sample bends the ramp. */
  ok1(equals(rasterizer.SampleAt(1, 0), 2.5));
  ok1(equals(rasterizer.SampleAt(2, 0), 4.5));

  uint8_t row[4 * 4];
  rasterizer.RenderRow(0, row);
  ok1(row[1 * 4] == 10 && row[1 * 4 + 3] == 255);
  ok1(row[2 * 4] == 20 && row[2 * 4 + 3] == 255);
}

static void
TestSampleFieldInterpolates()
{
  const auto field = MakeRow({0.f, 1.f, 2.f, 3.f});

  /* Grid samples are reproduced exactly. */
  ok1(equals(SkySight::SampleField(field, 0.f, 0.f), 0.));
  ok1(equals(SkySight::SampleField(field, 1.f, 0.f), 1.));
  ok1(equals(SkySight::SampleField(field, 3.f, 0.f), 3.));

  /* Catmull-Rom is exact for a linear ramp. */
  ok1(equals(SkySight::SampleField(field, 1.5f, 0.f), 1.5));
  ok1(equals(SkySight::SampleField(field, 1.25f, 0.f), 1.25));

  /* Coordinates outside the grid clamp to the edge samples. */
  ok1(equals(SkySight::SampleField(field, -2.f, 0.f), 0.));
  ok1(equals(SkySight::SampleField(field, 5.f, 0.f), 3.));

  const SkySight::ScalarField empty;
  ok1(std::isnan(SkySight::SampleField(empty, 0.f, 0.f)));
}

static void
TestSampleFieldDoesNotOvershoot()
{
  /* Catmull-Rom undershoots below zero between the two zero samples;
     clamping to the surrounding samples must suppress that, or the
     overlay would grow bands the forecast never reaches. */
  const auto field = MakeRow({10.f, 0.f, 0.f, 10.f});

  ok1(equals(SkySight::SampleField(field, 1.25f, 0.f), 0.));
  ok1(equals(SkySight::SampleField(field, 1.5f, 0.f), 0.));
  ok1(equals(SkySight::SampleField(field, 1.75f, 0.f), 0.));
}

static void
TestSampleFieldSkipsMissingData()
{
  const auto field = MakeRow({0.f, std::nanf(""), 2.f, 3.f});

  /* Missing samples do not poison their neighbours. */
  ok1(equals(SkySight::SampleField(field, 2.f, 0.f), 2.));

  /* Mostly outside the model domain → missing. */
  ok1(std::isnan(SkySight::SampleField(field, 1.f, 0.f)));
  ok1(std::isnan(SkySight::SampleField(field, 1.2f, 0.f)));

  /* Mostly inside → the valid neighbour carries the sample, so a data
     edge does not grow a transparent fringe. */
  ok1(equals(SkySight::SampleField(field, 1.8f, 0.f), 2.));
}

static void
TestRasterizerPlacesBandBoundary()
{
  constexpr unsigned UPSAMPLE = 4;

  /* A ramp from 0 to 3 across four samples; the 1.25 stop therefore
     crosses at grid coordinate 1.25, which is output pixel 6.5. */
  const auto field = MakeRow({0.f, 1.f, 2.f, 3.f});
  const SkySight::ContourPalette palette{{{1.25f, {12, 34, 56}}}};
  const SkySight::ContourRasterizer rasterizer{
    field, palette, {0, 0, 4, 1, 4 * UPSAMPLE, UPSAMPLE}};

  ok1(rasterizer.GetWidth() == 16);
  ok1(rasterizer.GetHeight() == UPSAMPLE);

  uint8_t row[16 * 4];
  rasterizer.RenderRow(0, row);

  /* Below the stop: fully transparent, all channels cleared. */
  ok1(row[6 * 4] == 0 && row[6 * 4 + 1] == 0 && row[6 * 4 + 2] == 0 &&
      row[6 * 4 + 3] == 0);

  /* The band starts at the iso-line rather than at the edge of grid cell
     2, where a flat one-pixel-per-sample image would switch (pixel 8). */
  ok1(row[7 * 4] == 12 && row[7 * 4 + 1] == 34 && row[7 * 4 + 2] == 56 &&
      row[7 * 4 + 3] == 255);
  ok1(row[15 * 4 + 3] == 255);
}

static void
TestRasterizerWindowMatchesWholeField()
{
  /* Rendering a window must give the same pixels as the corresponding
     part of the whole field: samples outside the window still feed the
     interpolation, so a patch boundary is not a contour boundary. */
  const auto field = MakeRow({0.f, 1.f, 2.f, 3.f, 4.f, 5.f});
  const SkySight::ContourPalette palette{{
    {1.f, {10, 0, 0}}, {2.f, {20, 0, 0}}, {3.f, {30, 0, 0}},
    {4.f, {40, 0, 0}},
  }};

  const SkySight::ContourRasterizer whole{field, palette,
                                         {0, 0, 6, 1, 24, 4}};
  const SkySight::ContourRasterizer patch{field, palette,
                                         {2, 0, 2, 1, 8, 4}};

  uint8_t whole_row[6 * 4 * 4], patch_row[2 * 4 * 4];
  whole.RenderRow(0, whole_row);
  patch.RenderRow(0, patch_row);

  ok1(patch.GetWidth() == 8);
  ok1(std::equal(std::begin(patch_row), std::end(patch_row),
                 whole_row + 2 * 4 * 4));
}

static void
TestRasterizerKeepsMissingDataTransparent()
{
  const auto field = MakeRow({std::nanf(""), std::nanf(""), 2.f, 3.f});
  const SkySight::ContourPalette palette{{{0.5f, {12, 34, 56}}}};
  const SkySight::ContourRasterizer rasterizer{
    field, palette, {0, 0, 4, 1, 8, 2}};

  uint8_t row[8 * 4];
  rasterizer.RenderRow(0, row);

  ok1(row[3] == 0);
  ok1(row[7] == 0);
  ok1(row[7 * 4 + 3] == 255);
}

static void
TestQuantisationRoundTrip()
{
  const auto field = MakeRow({-5.f, 0.f, 12.5f, 20.f});
  const auto quantisation = SkySight::MeasureField(field);

  ok1(quantisation.IsValid());
  ok1(quantisation.minimum == -5.f);
  ok1(quantisation.maximum == 20.f);

  /* Sample 0 is reserved, so the extremes survive exactly. */
  ok1(quantisation.Decode(quantisation.Encode(-5.f)) == -5.f);
  ok1(quantisation.Decode(quantisation.Encode(20.f)) == 20.f);

  /* A step is a small fraction of any legend band. */
  const float step = (20.f + 5.f) / 254;
  ok1(std::fabs(quantisation.Decode(quantisation.Encode(12.5f)) - 12.5f) <=
      step);

  /* Missing data survives as missing. */
  ok1(quantisation.Encode(std::nanf("")) == 0);
  ok1(std::isnan(quantisation.Decode(0)));

  /* A field with no usable sample yields no packing range. */
  ok1(!SkySight::MeasureField(MakeRow({std::nanf("")})).IsValid());

  /* A constant field must not divide by zero. */
  const auto flat = SkySight::MeasureField(MakeRow({7.f, 7.f}));
  ok1(flat.IsValid());
  ok1(std::fabs(flat.Decode(flat.Encode(7.f)) - 7.f) < 0.001f);
}

static SkySight::GeoScalarField
MakeGeoField()
{
  SkySight::GeoScalarField result;
  result.field = MakeRow({0.f, 1.f, 2.f, 3.f});
  result.north_west = GeoPoint{Angle::Degrees(10), Angle::Degrees(50)};
  result.longitude_step = 0.5;
  result.latitude_step = 0.25;
  return result;
}

static void
TestGeoFieldGrid()
{
  const auto geo = MakeGeoField();
  ok1(geo.IsValid());

  /* The raster covers whole sample areas, so its bounds run from the
     north-west corner by one step per sample. */
  const auto bounds = geo.GetBounds();
  ok1(equals(bounds.GetWest().Degrees(), 10.));
  ok1(equals(bounds.GetEast().Degrees(), 12.));
  ok1(equals(bounds.GetNorth().Degrees(), 50.));
  ok1(equals(bounds.GetSouth().Degrees(), 49.75));

  /* A point inside sample 2 maps to column 2. */
  ok1(geo.ProjectClamped({Angle::Degrees(11.2), Angle::Degrees(49.9)}).x == 2);

  /* Points outside clamp to the grid instead of indexing past it. */
  ok1(geo.ProjectClamped({Angle::Degrees(-40), Angle::Degrees(80)}).x == 0);
  ok1(geo.ProjectClamped({Angle::Degrees(40), Angle::Degrees(0)}).x == 3);
  ok1(geo.ProjectClamped({Angle::Degrees(40), Angle::Degrees(0)}).y == 0);

  /* Sample corners line up with the bounds. */
  ok1(equals(geo.GetSampleCorner(0, 0).longitude.Degrees(), 10.));
  ok1(equals(geo.GetSampleCorner(4, 1).longitude.Degrees(), 12.));
  ok1(equals(geo.GetSampleCorner(4, 1).latitude.Degrees(), 49.75));
}

int
main()
{
  plan_tests(11 + 3 + 5 + 5 + 14 + 8 + 3 + 4 + 5 + 2 + 6 + 3 + 11 + 12);
  TestBipolarDeadZone();
  TestNegativeStopsStillPaint();
  TestAllPositiveLegend();
  TestPaletteRejectsMissingSamples();
  TestRasterBudget();
  TestSampleFieldInterpolates();
  TestSampleFieldDoesNotOvershoot();
  TestSampleFieldSkipsMissingData();
  TestRasterizerPlacesBandBoundary();
  TestRasterizerWindowMatchesWholeField();
  TestRasterizerDecimates();
  TestRasterizerKeepsMissingDataTransparent();
  TestQuantisationRoundTrip();
  TestGeoFieldGrid();
  return exit_status();
}
