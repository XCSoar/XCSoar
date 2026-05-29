// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gauge/GaugeVario.hpp"
#include "Look/VarioLook.hpp"
#include "Look/VarioBarLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "NMEA/SwitchState.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "util/StringUtil.hpp"
#include "util/Macros.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#define TEXT_BUG "Bug"
#define TEXT_BALLAST "Bal"

/** Infobox accent indices — match #InfoBoxLook::colors. */
static constexpr unsigned VARIO_ACCENT_NEUTRAL = 0;
static constexpr unsigned VARIO_ACCENT_RED = 1;
static constexpr unsigned VARIO_ACCENT_BLUE = 2;
static constexpr unsigned VARIO_ACCENT_GREEN = 3;

/* Larus sw_frontend_rs vario.rs / create_wallpaper_vario.py */
static constexpr double LARUS_SCALE_DEGREES = 25.0;
static constexpr double LARUS_SCALE_MAX_VALUE = 5.0;
static constexpr double LARUS_SCALE_MAX_ANGLE_DEG =
  LARUS_SCALE_MAX_VALUE * LARUS_SCALE_DEGREES;
/** Annulus arc extends this % past the ±5 labels (then panel background shows). */
static constexpr double LARUS_ANNULUS_OVERSHOOT_PERCENT = 2.0;
static constexpr double LARUS_INNER_RADIUS_RATIO = 0.80;
static constexpr double LARUS_NINE_O_CLOCK = 1.5 * M_PI;

struct LarusScaleParams {
  double max_value;
  double deg_per_unit;
  int tick_step;
  int n_ticks;
};

struct LarusNeedleCoord {
  double len;
  double alpha;
};

static constexpr LarusNeedleCoord classic_needle[] = {
  {1.0, 0.0},
  {0.934, 0.0405},
  {0.703, 0.0538},
  {0.703, -0.0538},
  {0.934, -0.0405},
};

/** Original Larus gross-needle inner extent in SVG coordinates. */
static constexpr double LARUS_CLASSIC_NEEDLE_INNER_ORIG = 0.703;

static constexpr LarusNeedleCoord simple_needle[] = {
  {1.223, 0.0},
  {1.012, -0.107},
  {1.012, 0.107},
};

/** Furthest `len_frac` on #simple_needle — tip at outer scale when remapped. */
static constexpr double LARUS_SIMPLE_NEEDLE_MAX_LEN = 1.223;

/** Original Larus net/average-needle inner extent in SVG coordinates. */
static constexpr double LARUS_SIMPLE_NEEDLE_INNER_ORIG = 1.012;

struct LarusNeedleLengthMap {
  double inner_orig;
  double outer_orig;
};

static constexpr LarusNeedleLengthMap larus_classic_needle_map{
  LARUS_CLASSIC_NEEDLE_INNER_ORIG, 1.0,
};

static constexpr LarusNeedleLengthMap larus_simple_needle_map{
  LARUS_SIMPLE_NEEDLE_INNER_ORIG, LARUS_SIMPLE_NEEDLE_MAX_LEN,
};

/** Map SVG needle length so inner base sits on the annulus inner circle. */
[[gnu::const]]
static double
RemapNeedleLength(double len_frac,
                  const LarusNeedleLengthMap &map) noexcept
{
  if (len_frac >= map.outer_orig)
    return 1.0;
  if (len_frac <= map.inner_orig)
    return LARUS_INNER_RADIUS_RATIO;
  return LARUS_INNER_RADIUS_RATIO +
    (len_frac - map.inner_orig) * (1.0 - LARUS_INNER_RADIUS_RATIO) /
    (map.outer_orig - map.inner_orig);
}

static constexpr LarusNeedleCoord scale_marker[] = {
  {1.014, -0.058},
  {1.014, 0.058},
  {0.878, 0.0},
};

static_assert(ARRAY_SIZE(classic_needle) == 5,
              "Larus classic needle coordinate count");
static_assert(ARRAY_SIZE(simple_needle) == 3,
              "Larus simple needle coordinate count");
static_assert(ARRAY_SIZE(scale_marker) == 3,
              "Larus scale marker coordinate count");

struct AverageDisplay {
  const char *label;
  double raw;
};

[[gnu::pure]]
static AverageDisplay
GetAverageDisplay(const DerivedInfo &calculated) noexcept
{
  if (calculated.circling)
    return {"Avg.", calculated.average};

  return {"Net.", calculated.netto_average};
}

[[gnu::const]]
static LarusScaleParams
GetLarusScaleParams() noexcept
{
  switch (Units::current.vertical_speed_unit) {
  case Unit::FEET_PER_MINUTE:
    return {1020.0, LARUS_SCALE_DEGREES / 200.0, 200, 5};

  case Unit::KNOTS:
    return {10.2, LARUS_SCALE_DEGREES / 2.0, 2, 5};

  default:
    return {5.1, LARUS_SCALE_DEGREES, 1, 5};
  }
}

/** Scale limit for the annulus arc (±5 plus overshoot; needles use max_value). */
[[gnu::const]]
static double
LarusAnnulusArcEnd(const LarusScaleParams &params) noexcept
{
  const double tick_limit = double(params.n_ticks) * params.tick_step;
  return tick_limit * (1.0 + LARUS_ANNULUS_OVERSHOOT_PERCENT / 100.0);
}

[[gnu::const]]
static Angle
ValueToNeedleAngle(double value) noexcept
{
  const LarusScaleParams params = GetLarusScaleParams();
  value = std::clamp(value, -params.max_value, params.max_value);
  return Angle::Degrees(value * params.deg_per_unit);
}

[[gnu::const]]
static double
LarusScaleValueRadians(double scale_value,
                       const LarusScaleParams &params) noexcept
{
  return scale_value * params.deg_per_unit * M_PI / 180.0;
}

[[gnu::const]]
static PixelPoint
LarusScalePoint(PixelPoint center, unsigned radius,
                double scale_value, const LarusScaleParams &params) noexcept
{
  const double rad = LarusScaleValueRadians(scale_value, params);
  return PixelPoint{
    center.x - int(std::lround(std::cos(rad) * radius)),
    center.y - int(std::lround(std::sin(rad) * radius)),
  };
}

/** Canvas arc angle for a Larus scale value (matches CirclePoint convention). */
[[gnu::const]]
static Angle
LarusScaleValueToCanvasAngle(double scale_value,
                             const LarusScaleParams &params) noexcept
{
  const double rad = LarusScaleValueRadians(scale_value, params);
  return Angle::Radians(std::atan2(-std::cos(rad), std::sin(rad)));
}

/** Horizontal extent of ±max scale value past the dial center (toward right). */
[[gnu::const]]
static double
LarusScaleRightExtent() noexcept
{
  const double rad = LARUS_SCALE_MAX_ANGLE_DEG * M_PI / 180.0;
  return -std::cos(rad);
}

/** Vertical extent of ±max scale value from the dial center. */
[[gnu::const]]
static double
LarusScaleVerticalExtent() noexcept
{
  const double rad = LARUS_SCALE_MAX_ANGLE_DEG * M_PI / 180.0;
  return std::sin(rad);
}

[[gnu::const]]
static PixelPoint
LarusNeedlePoint(PixelPoint center, int length, Angle rotation,
                 double len_frac, double alpha) noexcept
{
  const double rad = LARUS_NINE_O_CLOCK + rotation.Radians() + alpha;
  const double dist = len_frac * length;
  return PixelPoint{
    center.x + int(std::lround(std::sin(rad) * dist)),
    center.y + int(std::lround(-std::cos(rad) * dist)),
  };
}

static void
DrawNeedlePolygon(Canvas &canvas, PixelPoint center, int length,
                  Angle rotation, Color color,
                  const LarusNeedleCoord *coords, unsigned count,
                  const LarusNeedleLengthMap *length_map) noexcept
{
  std::array<BulkPixelPoint, 5> points{};
  assert(count <= points.size());

  for (unsigned i = 0; i < count; ++i) {
    const double len_frac = length_map != nullptr
      ? RemapNeedleLength(coords[i].len, *length_map)
      : coords[i].len;
    points[i] = LarusNeedlePoint(center, length, rotation, len_frac,
                                 coords[i].alpha);
  }

  canvas.Select(Brush(color));
  canvas.SelectNullPen();
  canvas.DrawTriangleFan(points.data(), count);
}

[[gnu::const]]
static Color
VarioAccentColor(const VarioLook &look, unsigned index) noexcept
{
  assert(index < ARRAY_SIZE(look.accent));
  if (!look.colors)
    return look.inverse ? look.text_color : look.dimmed_text_color;
  return look.accent[index];
}

/** MC accent — matches #InfoBoxContentMacCready::Update value color indices. */
[[gnu::pure]]
static Color
McAccentColor(const VarioLook &look, bool auto_mc) noexcept
{
  return VarioAccentColor(look,
                          auto_mc ? VARIO_ACCENT_BLUE : VARIO_ACCENT_GREEN);
}

/** Side length of the largest square inscribed in the inner hole. */
[[gnu::const]]
static unsigned
InnerHoleInscribedSquareSide(unsigned inner_radius) noexcept
{
  return std::max(1u, unsigned(std::lround(double(inner_radius) * M_SQRT2)));
}

[[gnu::const]]
static unsigned
InnerTextWidth(unsigned square_side) noexcept
{
  return std::max(1u, square_side * VarioLook::INNER_TEXT_WIDTH_PERCENT / 100u);
}

/** Baseline that vertically centers capital letters in a row. */
[[gnu::const]]
static int
RowTextBaselineY(const Font &font, int row_top, int row_bottom) noexcept
{
  const int cap_top =
    (row_top + row_bottom - int(font.GetCapitalHeight())) / 2;
  return cap_top - int(font.GetAscentHeight()) + int(font.GetCapitalHeight());
}

/** Text area and speed-arrow strip inside the inscribed inner hole. */
struct InnerHoleTextLayout {
  PixelRect text_area;
  PixelRect speed_arrows;
};

[[gnu::pure]]
static InnerHoleTextLayout
ComputeInnerHoleTextLayout(PixelPoint dial_center,
                           unsigned inner_square_side,
                           const PixelRect &content_rect,
                           int pad) noexcept
{
  const int cx = dial_center.x;
  const int cy = dial_center.y;
  const int half = int(inner_square_side) / 2;
  const int square_left = cx - half;
  const int square_top = cy - half;
  const int square_bottom = square_top + int(inner_square_side);
  const int text_width = int(InnerTextWidth(inner_square_side));
  const int square_right = square_left + int(inner_square_side);
  const int top = square_top + pad;
  const int bottom = square_bottom - pad;
  const int strip_gap = Layout::GetTextPadding();

  return {
    {
      std::max(content_rect.left + pad, square_left + pad),
      top,
      std::min(content_rect.right - pad,
               square_left + text_width - pad - strip_gap),
      bottom,
    },
    {
      square_left + text_width + strip_gap,
      top,
      std::min(content_rect.right - pad, square_right - pad),
      bottom,
    },
  };
}

struct DialMetrics {
  PixelPoint dial_center;
  unsigned outer_radius;
  unsigned inner_radius;
};

[[gnu::pure]]
static DialMetrics
ComputeDialMetrics(const PixelRect &rc) noexcept
{
  const unsigned panel_width = rc.GetWidth();
  const unsigned panel_height = rc.GetHeight();

  const double scale_right = LarusScaleRightExtent();
  const double scale_vertical = LarusScaleVerticalExtent();

  const unsigned max_radius_x =
    unsigned(double(panel_width) / (1.0 + scale_right));
  const unsigned max_radius_y =
    unsigned(double(panel_height) / 2.0 / scale_vertical);
  const unsigned outer_radius =
    std::max(1u, std::min(max_radius_x, max_radius_y));

  unsigned inner_radius = std::max(1u, unsigned(double(outer_radius)
                                                  * LARUS_INNER_RADIUS_RATIO));
  if (inner_radius >= outer_radius)
    inner_radius = outer_radius - 1;

  return {
    {
      rc.left + int(outer_radius),
      rc.top + int(panel_height) / 2,
    },
    outer_radius,
    inner_radius,
  };
}

inline
GaugeVario::BallastGeometry::BallastGeometry(const VarioLook &look,
                                             const PixelRect &rc) noexcept
{
  PixelSize tSize;

  label_pos.x = 1;
  label_pos.y = rc.top + 2
    + look.label_font.GetCapitalHeight() * 2
    - look.label_font.GetAscentHeight();

  value_pos.x = 1;
  value_pos.y = rc.top + 1
    + look.label_font.GetCapitalHeight()
    - look.label_font.GetAscentHeight();

  label_rect.left = label_pos.x;
  label_rect.top = label_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();

  value_rect.left = value_pos.x;
  value_rect.top = value_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();

  tSize = look.label_font.TextSize(TEXT_BALLAST);

  label_rect.right = label_rect.left + tSize.width;
  label_rect.bottom = label_rect.top +
    look.label_font.GetCapitalHeight();

  tSize = look.label_font.TextSize("100%");

  value_rect.right = value_rect.left + tSize.width;
  value_rect.bottom = value_rect.top +
    look.label_font.GetCapitalHeight();
}

inline
GaugeVario::BugsGeometry::BugsGeometry(const VarioLook &look,
                                       const PixelRect &rc) noexcept
{
  PixelSize tSize;

  label_pos.x = 1;
  label_pos.y = rc.bottom - 2
    - look.label_font.GetCapitalHeight()
    - look.label_font.GetAscentHeight();

  value_pos.x = 1;
  value_pos.y = rc.bottom - 1
    - look.label_font.GetAscentHeight();

  label_rect.left = label_pos.x;
  label_rect.top = label_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();
  value_rect.left = value_pos.x;
  value_rect.top = value_pos.y
    + look.label_font.GetAscentHeight()
    - look.label_font.GetCapitalHeight();

  tSize = look.label_font.TextSize(TEXT_BUG);

  label_rect.right = label_rect.left + tSize.width;
  label_rect.bottom = label_rect.top +
    look.label_font.GetCapitalHeight();

  tSize = look.label_font.TextSize("100%");

  value_rect.right = value_rect.left + tSize.width;
  value_rect.bottom = value_rect.top +
    look.label_font.GetCapitalHeight();
}

inline
GaugeVario::Geometry::Geometry(const VarioLook &look,
                               const PixelRect &rc) noexcept
  :ballast(look, rc), bugs(look, rc)
{
  content_rect = rc;
  row_gap = Layout::Scale(3);
  speed_arrows_rect = {};

  const DialMetrics dial = ComputeDialMetrics(rc);
  dial_center = dial.dial_center;
  outer_radius = dial.outer_radius;
  inner_radius = dial.inner_radius;
  inner_square_side = InnerHoleInscribedSquareSide(inner_radius);
  inner_row_slot = std::max(1u, inner_square_side / 3u);
}

GaugeVario::GaugeVario(const FullBlackboard &_blackboard,
                       ContainerWindow &parent, VarioLook &_look,
                       const VarioBarLook &vario_bar_look,
                       PixelRect rc, const WindowStyle style) noexcept
  :blackboard(_blackboard), look(_look),
   speed_bar_renderer(vario_bar_look)
{
  Create(parent, rc, style);
}

void
GaugeVario::RecalculateGeometry() noexcept
{
  const PixelRect rc = GetClientRect();
  const unsigned panel_width = rc.GetWidth();
  const unsigned scale_title =
    blackboard.GetUISettings().info_boxes.scale_title_font;

  const DialMetrics dial = ComputeDialMetrics(rc);
  const unsigned square = InnerHoleInscribedSquareSide(dial.inner_radius);
  const unsigned row_slot = std::max(1u, square / 3u);
  const unsigned band = dial.outer_radius - dial.inner_radius;
  const unsigned arc_w = std::max(4u, band / 2u);
  const int pad = Layout::GetTextPadding();
  const InnerHoleTextLayout layout = ComputeInnerHoleTextLayout(
    dial.dial_center, square, rc, pad);
  const unsigned layout_text_width =
    std::max(1u, unsigned(layout.text_area.GetWidth()));

  look.ReinitialiseLayout(panel_width, scale_title, layout_text_width,
                          row_slot, arc_w);
  geometry = {look, rc};

  cached_scale_title_font = scale_title;

  LayoutValueColumn();

  background_dirty = true;
  dirty = true;
  cached_look_generation = look.layout_generation;

  hero_di.Reset();
  gross_di.Reset();
  mc_di.Reset();

  last_ballast = -1;
  last_bugs = -1;
}

void
GaugeVario::LayoutValueColumn() noexcept
{
  const VarioSettings &settings = Settings();

  struct RowSpec {
    RowGeometry *row;
    bool visible;
  };

  const RowSpec rows[] = {
    {&geometry.hero_row, settings.show_average},
    {&geometry.gross_row, settings.show_gross},
    {&geometry.mc_row, settings.show_mc},
  };

  unsigned visible_count = 0;
  for (const RowSpec &spec : rows) {
    if (spec.visible)
      ++visible_count;
  }

  const int cy = geometry.dial_center.y;
  const int pad = Layout::GetTextPadding();
  const InnerHoleTextLayout layout = ComputeInnerHoleTextLayout(
    geometry.dial_center, geometry.inner_square_side,
    geometry.content_rect, pad);
  geometry.speed_arrows_rect = layout.speed_arrows;

  if (visible_count == 0)
    return;

  static constexpr unsigned text_slots = 3;
  const unsigned row_height = std::max(look.label_font.GetHeight(),
                                       look.value_font.GetHeight());

  const int square_top = cy - int(geometry.inner_square_side) / 2;
  const int row_left = layout.text_area.left;
  const int row_right = layout.text_area.right;
  const int text_width = row_right - row_left;
  const int split_x = row_left
    + text_width * int(VarioLook::INNER_ROW_LABEL_WIDTH_PERCENT) / 100;

  const unsigned first_slot = (text_slots - visible_count) / 2;
  unsigned slot_index = 0;

  for (const RowSpec &spec : rows) {
    if (!spec.visible)
      continue;

    const int slot_y = square_top
      + int((first_slot + slot_index) * geometry.inner_row_slot);
    const int y = slot_y + (int(geometry.inner_row_slot) - int(row_height)) / 2;
    const int row_bottom = y + int(row_height);

    spec.row->label_background = {row_left, y, split_x, row_bottom};
    spec.row->value_background = {split_x, y, row_right, row_bottom};

    ++slot_index;
  }
}

void
GaugeVario::RenderBackground(Canvas &canvas, const PixelRect &rc) noexcept
{
  canvas.DrawFilledRectangle(rc, look.background_color);
}

void
GaugeVario::RenderSpeedArrows(Canvas &canvas) noexcept
{
  if (!Settings().show_speed_to_fly)
    return;

  const PixelRect &rc = geometry.speed_arrows_rect;
  if (rc.GetWidth() == 0 || rc.GetHeight() == 0)
    return;

  if (IsPersistent())
    canvas.DrawFilledRectangle(rc, look.background_color);

  const int pad = Layout::GetTextPadding();
  const unsigned max_half = std::max(1u,
    (geometry.inner_square_side - 2u * unsigned(pad)) / 2u);

  speed_bar_renderer.DrawSpeedToFly(canvas, rc, Basic(), Calculated(),
                                    max_half,
                                    VarioAccentColor(look, VARIO_ACCENT_GREEN),
                                    VarioAccentColor(look, VARIO_ACCENT_BLUE));
}

void
GaugeVario::RenderScale(Canvas &canvas) noexcept
{
  const LarusScaleParams params = GetLarusScaleParams();
  const unsigned tick_outer = geometry.outer_radius;
  const unsigned tick_inner = geometry.inner_radius;
  const unsigned label_radius = tick_inner + (tick_outer - tick_inner) / 2;
  const double arc_limit = LarusAnnulusArcEnd(params);

  const Angle arc_start =
    LarusScaleValueToCanvasAngle(-arc_limit, params);
  const Angle arc_end =
    LarusScaleValueToCanvasAngle(arc_limit, params);

  canvas.Select(Brush(look.scale_face_color));
  canvas.SelectNullPen();
  canvas.DrawAnnulus(geometry.dial_center, tick_inner, tick_outer,
                     arc_start, arc_end);

  canvas.Select(Pen(Layout::ScaleFinePenWidth(1), look.scale_ink_color));
  for (int i = -params.n_ticks; i <= params.n_ticks; ++i) {
    const double value = i * params.tick_step;
    canvas.DrawLine(LarusScalePoint(geometry.dial_center, tick_outer,
                                    value, params),
                    LarusScalePoint(geometry.dial_center, tick_inner,
                                    value, params));
  }

  canvas.Select(look.arc_label_font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(look.scale_ink_color);
  for (int i = -params.n_ticks; i <= params.n_ticks; ++i) {
    const double value = i * params.tick_step;
    const PixelPoint anchor =
      LarusScalePoint(geometry.dial_center, label_radius, value, params);
    const auto label = FmtBuffer<8>("{}", std::abs(i * params.tick_step));
    const PixelSize text_size = canvas.CalcTextSize(label.c_str());
    const PixelRect text_rc = PixelRect::Centered(anchor, text_size);
    if (!geometry.content_rect.OverlapsWith(text_rc))
      continue;

    canvas.DrawText(text_rc.GetTopLeft(), label.c_str());
  }
}

void
GaugeVario::RenderNeedles(Canvas &canvas) noexcept
{
  const int full_len = int(geometry.outer_radius);

  double classic_value = Basic().brutto_vario;
  if (Settings().show_thermal_average_needle) {
    classic_value = Calculated().circling
      ? Calculated().netto_average
      : Calculated().last_thermal_average_smooth;
  }

  const double gross = Units::ToUserVSpeed(classic_value);
  DrawNeedlePolygon(canvas, geometry.dial_center, full_len,
                    ValueToNeedleAngle(gross),
                    VarioAccentColor(look, VARIO_ACCENT_RED),
                    classic_needle, ARRAY_SIZE(classic_needle),
                    &larus_classic_needle_map);

  if (Settings().show_average_needle) {
    const AverageDisplay average = GetAverageDisplay(Calculated());
    const double average_user = Units::ToUserVSpeed(average.raw);
    DrawNeedlePolygon(canvas, geometry.dial_center, full_len,
                      ValueToNeedleAngle(average_user),
                      VarioAccentColor(look, VARIO_ACCENT_BLUE),
                      simple_needle, ARRAY_SIZE(simple_needle),
                      &larus_simple_needle_map);
  }

  if (Settings().show_mc) {
    const double mc = Units::ToUserVSpeed(GetGlidePolar().GetMC());
    const bool auto_mc = GetComputerSettings().task.auto_mc;
    DrawNeedlePolygon(canvas, geometry.dial_center, full_len,
                      ValueToNeedleAngle(mc),
                      McAccentColor(look, auto_mc),
                      scale_marker, ARRAY_SIZE(scale_marker),
                      nullptr);
  }
}

void
GaugeVario::OnPaintBuffer(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();
  const unsigned scale_title =
    blackboard.GetUISettings().info_boxes.scale_title_font;
  if (rc.GetSize() != geometry.content_rect.GetSize() ||
      look.layout_generation != cached_look_generation ||
      cached_scale_title_font != scale_title)
    RecalculateGeometry();

  if (!IsPersistent() || background_dirty) {
    RenderBackground(canvas, rc);
    background_dirty = false;
  }

  RenderScale(canvas);
  RenderNeedles(canvas);

  RenderInnerText(canvas);
  RenderSpeedArrows(canvas);

  if (!Settings().show_speed_to_fly)
    RenderClimb(canvas);

  if (Settings().show_ballast)
    RenderBallast(canvas);

  if (Settings().show_bugs)
    RenderBugs(canvas);

  dirty = false;
}

void
GaugeVario::RenderInnerText(Canvas &canvas) noexcept
{
  if (Settings().show_average) {
    const AverageDisplay average = GetAverageDisplay(Calculated());
    const double value =
      (double)iround(Units::ToUserVSpeed(average.raw) * 10) / 10;
    const auto buffer = FmtBuffer<18>("{:<+4.1f}", value);
    RenderRow(canvas, geometry.hero_row, hero_di.label, hero_di.value,
              average.label, buffer.c_str(), look.text_color);
  }

  if (Settings().show_gross) {
    const auto vvaldisplay = std::clamp(Units::ToUserVSpeed(Basic().brutto_vario),
                                        -99.9, 99.9);
    const auto buffer = FmtBuffer<18>("{:<+4.1f}",
                                      (double)iround(vvaldisplay * 10) / 10);
    RenderRow(canvas, geometry.gross_row, gross_di.label, gross_di.value,
              "Gross", buffer.c_str(), look.text_color);
  }

  if (Settings().show_mc) {
    const auto mc = Units::ToUserVSpeed(GetGlidePolar().GetMC());
    const auto buffer = FmtBuffer<18>("{:<+4.1f}",
                                      (double)iround(mc * 10) / 10);
    RenderRow(canvas, geometry.mc_row, mc_di.label, mc_di.value,
              "MC", buffer.c_str(),
              McAccentColor(look, GetComputerSettings().task.auto_mc));
  }
}

void
GaugeVario::RenderRow(Canvas &canvas, const RowGeometry &row,
                      DrawInfo &label_di, DrawInfo &value_di,
                      const char *label, const char *value_text,
                      Color value_color) noexcept
{
  if (row.label_background.GetWidth() == 0)
    return;

  canvas.SetBackgroundColor(look.background_color);

  const bool value_changed = dirty &&
    !StringIsEqual(value_di.last_text, value_text);
  const bool label_changed = dirty &&
    !StringIsEqual(label_di.last_text, label);

  if (!IsPersistent() || label_changed || value_changed) {
    const int row_top = row.label_background.top;
    const int row_bottom = row.label_background.bottom;

    canvas.Select(look.label_font);
    const PixelPoint label_position{
      row.label_background.left,
      RowTextBaselineY(look.label_font, row_top, row_bottom),
    };

    canvas.SetTextColor(look.dimmed_text_color);
    canvas.Select(look.label_font);
    if (IsPersistent()) {
      canvas.DrawOpaqueText(label_position, row.label_background, label);
      strcpy(label_di.last_text, label);
    } else {
      canvas.DrawText(label_position, label);
    }

    canvas.Select(look.value_font);
    const unsigned value_width = canvas.CalcTextSize(value_text).width;
    const PixelPoint value_position{
      row.value_background.right - int(value_width),
      RowTextBaselineY(look.value_font, row_top, row_bottom),
    };

    canvas.SetTextColor(value_color);
    canvas.Select(look.value_font);
    if (IsPersistent()) {
      canvas.DrawOpaqueText(value_position, row.value_background, value_text);
      strcpy(value_di.last_text, value_text);
    } else {
      canvas.DrawText(value_position, value_text);
    }
  }
}

void
GaugeVario::RenderClimb(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();
  int x = rc.right - Layout::Scale(14);
  int y = rc.bottom - Layout::Scale(24);

  if (!dirty)
    return;

  const PixelSize dest_size{Layout::VptScale(9)};

  if (Basic().switch_state.flight_mode == SwitchState::FlightMode::CIRCLING)
    canvas.Stretch({x, y}, dest_size, look.climb_bitmap, {12, 0}, {12, 12});
  else if (IsPersistent())
    canvas.DrawFilledRectangle(PixelRect{{x, y}, dest_size},
                               look.background_color);
}

void
GaugeVario::RenderBallast(Canvas &canvas) noexcept
{
  const GlidePolar &polar = GetGlidePolar();
  const int ballast = iround(polar.GetBallastFraction() * 100);

  if (!IsPersistent() || ballast != last_ballast) {
    canvas.Select(look.label_font);

    if (IsPersistent())
      canvas.SetBackgroundColor(look.background_color);
    else
      canvas.SetBackgroundTransparent();

    const auto &g = geometry.ballast;

    if (IsPersistent() || last_ballast < 1 || ballast < 1) {
      if (ballast > 0) {
        canvas.SetTextColor(look.dimmed_text_color);
        if (IsPersistent())
          canvas.DrawOpaqueText(g.label_pos, g.label_rect, TEXT_BALLAST);
        else
          canvas.DrawText(g.label_pos, TEXT_BALLAST);
      } else if (IsPersistent())
        canvas.DrawFilledRectangle(g.label_rect, look.background_color);
    }

    if (ballast > 0) {
      const auto buffer = FmtBuffer<18>("{}%", ballast);
      canvas.SetTextColor(look.text_color);

      if (IsPersistent())
        canvas.DrawOpaqueText(g.value_pos, g.value_rect, buffer.c_str());
      else
        canvas.DrawText(g.value_pos, buffer.c_str());
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(g.value_rect, look.background_color);

    if (IsPersistent())
      last_ballast = ballast;
  }
}

inline void
GaugeVario::RenderBugs(Canvas &canvas) noexcept
{
  const int bugs = iround((1 - GetComputerSettings().polar.bugs) * 100);

  if (!IsPersistent() || bugs != last_bugs) {
    canvas.Select(look.label_font);

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
      const auto buffer = FmtBuffer<18>("{}%", bugs);
      canvas.SetTextColor(look.text_color);
      if (IsPersistent())
        canvas.DrawOpaqueText(g.value_pos, g.value_rect, buffer.c_str());
      else
        canvas.DrawText(g.value_pos, buffer.c_str());
    } else if (IsPersistent())
      canvas.DrawFilledRectangle(g.value_rect, look.background_color);

    if (IsPersistent())
      last_bugs = bugs;
  }
}

void
GaugeVario::OnResize(PixelSize new_size) noexcept
{
  AntiFlickerWindow::OnResize(new_size);
  RecalculateGeometry();
}
