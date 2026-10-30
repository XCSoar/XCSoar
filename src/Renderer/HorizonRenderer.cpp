// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonRenderer.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "Look/AutoFont.hpp"
#include "Look/FontDescription.hpp"
#include "Look/HorizonLook.hpp"
#include "Math/Util.hpp"
#include "NMEA/Attitude.hpp"
#include "RadarRenderer.hpp"
#include "Screen/Layout.hpp"
#include "WarningRibbonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "ui/canvas/Font.hpp"
#include "util/StringFormat.hpp"

#include <algorithm>
#include <cmath>

#ifdef HAVE_TEXT_CACHE
#include "ui/canvas/custom/Cache.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

/** Pitch that moves the horizon from the centre to the rect edge. */
static constexpr float MAX_PITCH_DEG = 40.f;

/** One design pixel, never a zero-width pen. */
static unsigned
PenPx() noexcept
{
  const unsigned width = Layout::Scale(1U);
  return width > 0 ? width : 1U;
}

static unsigned
Stroke(bool compact) noexcept
{
  return compact ? 1U : PenPx();
}

/**
 * The InfoBox dial is a small fraction of the screen.  A fixed 100px
 * cutoff treats a high-DPI InfoBox as the full horizon.
 */
static bool
IsCompact(int radius) noexcept
{
  return radius <= 100 ||
         radius * 5 <= static_cast<int>(Layout::min_screen_pixels);
}

/**
 * Dial fraction, raised to a Layout:: floor when that floor still
 * fits in @p cap.  The radius is already in screen pixels.
 */
static int
Fit(int proportional, int design_px, int cap) noexcept
{
  const int scaled = Layout::Scale(design_px);
  const int floor = scaled > 0 ? scaled : std::max(design_px, 1);
  int v = std::max(proportional, floor);
  if (v > cap)
    v = std::max(proportional, 1);
  return std::max(v, 1);
}

static void
Rotate(PixelPoint point, PixelPoint center, Angle a,
       PixelPoint &rotated) noexcept
{
  rotated.x = int(point.x * a.cos() - point.y * a.sin() + center.x);
  rotated.y = int(point.x * a.sin() + point.y * a.cos() + center.y);
}

/** InfoBox outlines stay one pixel; the full horizon uses the Look pen. */
static void
SelectFill(Canvas &canvas, const Pen &pen, const Brush &brush,
           Color color, bool hairline) noexcept
{
  if (hairline)
    canvas.Select(Pen(1, color));
  else
    canvas.Select(pen);
  canvas.Select(brush);
}

/**
 * Signed area of h0->h1->p.  Wings level, this is negative on the
 * sky side (smaller screen y).
 */
static int64_t
LineSide(PixelPoint h0, PixelPoint h1, PixelPoint p) noexcept
{
  return int64_t(h1.x - h0.x) * (p.y - h0.y)
       - int64_t(h1.y - h0.y) * (p.x - h0.x);
}

static int
DivRound(int64_t num, int64_t den) noexcept
{
  const int64_t offset = den < 0 ? -den / 2 : den / 2;
  return int((num < 0 ? num - offset : num + offset) / den);
}

/** Proper crossing of segment ab with the infinite horizon line. */
static bool
CutSegment(PixelPoint a, PixelPoint b,
           PixelPoint h0, PixelPoint h1,
           PixelPoint &hit) noexcept
{
  const int64_t da = LineSide(h0, h1, a);
  const int64_t db = LineSide(h0, h1, b);
  if (!((da > 0 && db < 0) || (da < 0 && db > 0)))
    return false;

  const int64_t den = da - db;
  hit.x = a.x + DivRound(int64_t(b.x - a.x) * da, den);
  hit.y = a.y + DivRound(int64_t(b.y - a.y) * da, den);
  return true;
}

struct ClipPoly {
  BulkPixelPoint v[8];
  unsigned n = 0;

  void Add(PixelPoint p) noexcept {
    if (n > 0 && v[n - 1].x == p.x && v[n - 1].y == p.y)
      return;
    if (n < std::size(v))
      v[n++] = p;
  }
};

/**
 * Split the rectangle by the horizon line.  Sky is the negative side.
 * Returns the two boundary crossings when the line cuts the rect.
 */
static bool
ClipHorizon(const PixelRect &rc, PixelPoint h0, PixelPoint h1,
            ClipPoly &sky, ClipPoly &ground,
            PixelPoint &cut_a, PixelPoint &cut_b) noexcept
{
  const PixelPoint corner[4] = {
    rc.GetTopLeft(),
    rc.GetTopRight(),
    rc.GetBottomRight(),
    rc.GetBottomLeft(),
  };

  PixelPoint cuts[4];
  unsigned n_cuts = 0;
  const auto add_cut = [&](PixelPoint p) noexcept {
    for (unsigned i = 0; i < n_cuts; ++i)
      if (cuts[i].x == p.x && cuts[i].y == p.y)
        return;
    if (n_cuts < std::size(cuts))
      cuts[n_cuts++] = p;
  };

  for (unsigned i = 0; i < 4; ++i) {
    const PixelPoint a = corner[i];
    const PixelPoint b = corner[(i + 1) % 4];
    const int64_t side = LineSide(h0, h1, a);
    if (side <= 0)
      sky.Add(a);
    if (side >= 0)
      ground.Add(a);
    if (side == 0)
      add_cut(a);

    PixelPoint hit;
    if (CutSegment(a, b, h0, h1, hit)) {
      sky.Add(hit);
      ground.Add(hit);
      add_cut(hit);
    }
  }

  if (n_cuts < 2)
    return false;

  cut_a = cuts[0];
  cut_b = cuts[1];
  int64_t best = 0;
  for (unsigned i = 0; i < n_cuts; ++i) {
    for (unsigned j = i + 1; j < n_cuts; ++j) {
      const int64_t dx = int64_t(cuts[i].x) - cuts[j].x;
      const int64_t dy = int64_t(cuts[i].y) - cuts[j].y;
      const int64_t d = dx * dx + dy * dy;
      if (d > best) {
        best = d;
        cut_a = cuts[i];
        cut_b = cuts[j];
      }
    }
  }
  return true;
}

static void
PaintHalf(Canvas &canvas, const HorizonLook &look, bool compact,
          bool sky, const ClipPoly &poly) noexcept
{
  if (poly.n < 3)
    return;

  if (sky)
    SelectFill(canvas, look.sky_pen, look.sky_brush,
               HorizonLook::sky_color, compact);
  else
    SelectFill(canvas, look.terrain_pen, look.terrain_brush,
               HorizonLook::terrain_color, compact);
  canvas.DrawPolygon(poly.v, poly.n);
}

static void
DrawSkyGround(Canvas &canvas, const PixelRect &rc,
              const HorizonLook &look, bool compact,
              PixelPoint center, double bank_deg,
              double pitch_deg) noexcept
{
  const int pitch_px =
    int(rc.GetHeight() / 2.f * pitch_deg / MAX_PITCH_DEG);
  const Angle roll = Angle::Degrees(-bank_deg);
  const int span = std::max(int(rc.GetWidth()), int(rc.GetHeight()));
  PixelPoint h0{-span, pitch_px};
  PixelPoint h1{span, pitch_px};
  Rotate(h0, center, roll, h0);
  Rotate(h1, center, roll, h1);

  ClipPoly sky, ground;
  PixelPoint cut_a, cut_b;
  if (!ClipHorizon(rc, h0, h1, sky, ground, cut_a, cut_b)) {
    /* Horizon misses the rect: the whole dial is one colour. */
    PaintHalf(canvas, look, compact, sky.n >= ground.n, sky.n >= 3
              ? sky : ground);
    return;
  }

  PaintHalf(canvas, look, compact, true, sky);
  PaintHalf(canvas, look, compact, false, ground);

  const Angle bank = Angle::Degrees(bank_deg);
  const unsigned width = compact
    ? 1U
    : std::max(1U, look.horizon_pen.GetWidth());
  const PixelPoint off{iround(bank.sin() * int(width)),
                       iround(bank.cos() * int(width))};
  canvas.SelectBlackPen(width);
  canvas.DrawLine(cut_a + off, cut_b + off);

  if (compact)
    canvas.Select(Pen(1, COLOR_WHITE));
  else
    canvas.Select(look.horizon_pen);
  canvas.DrawLine(cut_a, cut_b);
}

/**
 * Three equal dashes along the segment.  Pen::DASH repeats every
 * 32 pixels, which is longer than a five-degree mark, so the built-in
 * style left a single dash.
 */
static void
DrawThreeDashes(Canvas &canvas, PixelPoint a, PixelPoint b) noexcept
{
  const double dx = b.x - a.x;
  const double dy = b.y - a.y;
  const double len = std::hypot(dx, dy);
  if (len < 3)
    return;

  const double ux = dx / len;
  const double uy = dy / len;
  const double step = len / 5;

  for (int i = 0; i < 5; i += 2) {
    const double t0 = i * step;
    canvas.DrawLine({a.x + iround(ux * t0), a.y + iround(uy * t0)},
                    {a.x + iround(ux * (t0 + step)),
                     a.y + iround(uy * (t0 + step))});
  }
}

static int
PitchY(int height, double pitch, int deg) noexcept
{
  return int(height / 2.f * (pitch - deg) / MAX_PITCH_DEG);
}

template<unsigned N>
static void
DrawBankTicks(Canvas &canvas, PixelPoint center, double bank,
              PixelPoint inner, PixelPoint outer,
              const int (&angles)[N]) noexcept
{
  for (int angle : angles) {
    PixelPoint a, b;
    Rotate(inner, center, Angle::Degrees(angle - bank), a);
    Rotate(outer, center, Angle::Degrees(angle - bank), b);
    canvas.DrawLine(a, b);
  }
}

static void
DrawPitchMark(Canvas &canvas, PixelPoint center, int height,
              double pitch, double bank, int half, int deg,
              bool dashed) noexcept
{
  const int y = PitchY(height, pitch, deg);
  PixelPoint a, b;
  const Angle roll = Angle::Degrees(-bank);
  Rotate({-half, y}, center, roll, a);
  Rotate({half, y}, center, roll, b);
  if (dashed)
    DrawThreeDashes(canvas, a, b);
  else
    canvas.DrawLine(a, b);
}

/** Black cloth under a mark.  The blend stays active while @p paint runs. */
template<typename Paint>
static void
DrawShadow(Canvas &canvas, Paint &&paint) noexcept
{
#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend blend;
#endif
  const Color shadow = COLOR_BLACK.WithAlpha(140);
  canvas.Select(Pen(PenPx(), shadow));
  canvas.Select(Brush(shadow));
  paint();
}

/**
 * One thin stroke: wings level, rising to a low point in the middle.
 * @param dy shifts the stroke down, for the underside shadow
 */
static void
DrawChevron(Canvas &canvas, PixelPoint c,
            int span, int gap, int half, int peak, int dy) noexcept
{
  const PixelPoint line[5] = {
    {c.x - span, c.y + dy},
    {c.x - gap, c.y + dy},
    {c.x, c.y - peak + dy},
    {c.x + gap, c.y + dy},
    {c.x + span, c.y + dy},
  };

  float nx[4], ny[4];
  for (unsigned i = 0; i < 4; ++i) {
    const float dx = float(line[i + 1].x - line[i].x);
    const float ddy = float(line[i + 1].y - line[i].y);
    const float len = std::hypot(dx, ddy);
    nx[i] = len > 0 ? ddy / len : 0.f;
    ny[i] = len > 0 ? -dx / len : 1.f;
    if (ny[i] < 0) {
      nx[i] = -nx[i];
      ny[i] = -ny[i];
    }
  }

  BulkPixelPoint v[10];
  const auto put = [&](unsigned i, float ox, float oy) noexcept {
    v[i] = {int(std::lround(line[i].x - ox)),
            int(std::lround(line[i].y - oy))};
    v[9 - i] = {int(std::lround(line[i].x + ox)),
                int(std::lround(line[i].y + oy))};
  };

  put(0, nx[0] * half, ny[0] * half);
  put(4, nx[3] * half, ny[3] * half);

  for (unsigned i = 1; i <= 3; ++i) {
    float ax = nx[i - 1] + nx[i];
    float ay = ny[i - 1] + ny[i];
    const float alen = std::hypot(ax, ay);
    if (!(alen > 0)) {
      put(i, 0.f, float(half));
      continue;
    }

    ax /= alen;
    ay /= alen;
    const float along = ax * nx[i - 1] + ay * ny[i - 1];
    const float scale = along > 0.2f
      ? std::min(float(half) / along, float(half) * 2)
      : float(half);
    put(i, ax * scale, ay * scale);
  }

  canvas.DrawPolygon(v, 10);
}

/** Center reference: a low "-^-", longer than the side dashes. */
struct AircraftGlyph {
  int span, gap, half, peak, thick;
};

static AircraftGlyph
MakeCenterGlyph(int radius) noexcept
{
  AircraftGlyph g;
  /* The InfoBox follows the dial.  A Layout:: floor that still fits
     the circle stretches the mark out to the rim. */
  if (IsCompact(radius)) {
    g.span = std::max(3, radius / 6);
    g.thick = 2;
    g.half = 2;
    g.peak = std::max(g.half + 2, g.span / 6);
    g.gap = std::max(2, g.span / 3);
    return g;
  }

  g.span = Fit(radius / 3, 8, radius / 2);
  g.thick = Fit(g.span / 12, 2, std::max(2, g.span / 4));
  g.half = std::max(1, g.thick / 2);
  g.peak = Fit(g.span / 6, 2, std::max(g.half + 1, g.span / 3));
  g.peak = std::max(g.peak, g.half + 1);
  g.gap = Fit(g.span / 3, 1, std::max(g.half + 1, g.span * 2 / 3));
  g.gap = std::max(g.gap, g.half + 1);
  return g;
}

static AircraftGlyph
MakeDashGlyph(int radius) noexcept
{
  AircraftGlyph g;
  g.span = Fit(radius / 12, 4, radius / 6);
  g.thick = Fit(g.span / 4, 2, std::max(2, g.span / 2));
  g.half = std::max(1, g.thick / 2);
  g.peak = 0;
  g.gap = 0;
  return g;
}

static void
DrawAircraftSymbol(Canvas &canvas, const PixelPoint &center, int radius,
                   const HorizonLook &look) noexcept
{
  const AircraftGlyph g = MakeCenterGlyph(radius);
  const bool compact = IsCompact(radius);

  if (!compact)
    DrawShadow(canvas, [&]{
      DrawChevron(canvas, center, g.span, g.gap, g.half, g.peak,
                  Fit(g.thick / 3, 1, g.thick));
    });

  canvas.Select(Pen(Stroke(compact), look.aircraft_color));
  canvas.Select(look.aircraft_brush);
  DrawChevron(canvas, center, g.span, g.gap, g.half, g.peak, 0);
}

/** Screen-fixed dash.  The shadow sits under the lower edge. */
static void
DrawDash(Canvas &canvas, PixelPoint c, int span, int half, int drop,
         Color color, const Brush &brush) noexcept
{
  if (drop > 0) {
    const BulkPixelPoint lip[] = {
      {c.x - span, c.y + half},
      {c.x + span, c.y + half},
      {c.x + span, c.y + half + drop},
      {c.x - span, c.y + half + drop},
    };
    DrawShadow(canvas, [&]{ canvas.DrawPolygon(lip, 4); });
  }

  canvas.Select(Pen(PenPx(), color));
  canvas.Select(brush);
  const BulkPixelPoint bar[] = {
    {c.x - span, c.y - half},
    {c.x + span, c.y - half},
    {c.x + span, c.y + half},
    {c.x - span, c.y + half},
  };
  canvas.DrawPolygon(bar, 4);
}

/**
 * Bold face for the ribbon.  The full horizon uses a short tag,
 * three eighths of the width.  The InfoBox uses three quarters of
 * its box.  Separate faces: the text cache keys a Font by address.
 */
static const Font *
AhrsFont(const PixelRect &rc, const char *label, bool large) noexcept
{
  static Font fonts[2];
  static unsigned key_w[2], key_h[2];

  const unsigned slot = large ? 1u : 0u;
  Font &font = fonts[slot];

  const unsigned width = rc.GetWidth();
  const unsigned height = rc.GetHeight();
  if (width < 8 || height < 8)
    return nullptr;

  if (font.IsDefined() && key_w[slot] == width && key_h[slot] == height)
    return &font;

  const unsigned target = large ? width * 3 / 8 : width * 3 / 4;
  const unsigned max_h = large ? height / 2 : height;

  try {
    FontDescription d(10, true);
    AutoSizeFont(d, std::max(8u, target), label);
    font.Load(d);

    const PixelSize banner = WarningRibbonRenderer::Size(font, label);
    if (banner.height > max_h && banner.height > 0) {
      const unsigned shrunk =
        std::max(6u, d.GetHeight() * max_h / banner.height);
      if (shrunk < d.GetHeight()) {
        d.SetHeight(shrunk);
        font.Load(d);
      }
    }
  } catch (...) {
    return nullptr;
  }

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif

  key_w[slot] = width;
  key_h[slot] = height;
  return &font;
}

/** Level horizon is not an attitude source: cross it out. */
static void
DrawNoAhrs(Canvas &canvas, const PixelRect &rc, bool compact) noexcept
{
  const Color color = HasColors() ? COLOR_RED : COLOR_BLACK;
  const unsigned width = compact
    ? 2U
    : std::max(2U, Layout::Scale(2U));
  canvas.Select(Pen(width, color));
  canvas.DrawLine(rc.GetTopLeft(), rc.GetBottomRight());
  canvas.DrawLine(rc.GetTopRight(), rc.GetBottomLeft());

  const char *label = _("NO AHRS");
  const bool large = !compact;
  const Font *font = AhrsFont(rc, label, large);
  if (font == nullptr || !font->IsDefined())
    return;

  const PixelSize banner = WarningRibbonRenderer::Size(*font, label);
  if (banner.width == 0 || banner.width > rc.GetWidth()
      || banner.height > rc.GetHeight())
    return;

  const PixelPoint centered = rc.CenteredTopLeft(banner);
  int origin_y = centered.y;
  if (large) {
    const int mid = (rc.top + rc.bottom) / 2;
    const int upper = mid - rc.top;
    origin_y = rc.top;
    if (upper > int(banner.height))
      origin_y += (upper - int(banner.height)) / 2;
  }

  WarningRibbonRenderer::Draw(canvas, {centered.x, origin_y},
                              *font, label);
}

static void
DrawScales(Canvas &canvas, const HorizonLook &look, bool compact,
           PixelPoint center, int radius, int height,
           double bank_deg, double pitch_deg) noexcept
{
  if (compact)
    canvas.Select(Pen(std::max(2U, unsigned(radius) / 16U), COLOR_WHITE));
  else
    canvas.Select(look.mark_pen);
  canvas.Select(look.mark_brush);

  const int tick_inset = compact
    ? std::max(1, radius / 16)
    : Layout::Scale(2);
  const int tick_len = compact
    ? std::max(2, radius / 6)
    : Layout::Scale(10);
  const int minor_tick_inset = compact
    ? tick_inset + tick_len / 3
    : Layout::Scale(7);
  const int pitch_half_width = compact
    ? std::max(4, radius / 6)
    : std::min(radius / 3,
               std::max(Layout::Scale(12), radius / 5));

  const PixelPoint tick_inner{-radius + tick_inset, 0};
  const PixelPoint tick_end{-radius + tick_inset + tick_len, 0};
  static constexpr int major_ticks[] = {
    0, 30, 45, 60, 120, 135, 150, 180,
  };
  DrawBankTicks(canvas, center, bank_deg, tick_inner, tick_end, major_ticks);

  if (!compact) {
    static constexpr int minor_ticks[] = {70, 80, 100, 110};
    DrawBankTicks(canvas, center, bank_deg,
                  {-radius + minor_tick_inset, 0}, tick_end, minor_ticks);

    const int zero_half = std::max(Layout::Scale(5), 1);
    const Angle up = Angle::Degrees(90 - bank_deg);
    PixelPoint a, b, tip;
    Rotate({-radius + tick_inset, -zero_half}, center, up, a);
    Rotate({-radius + tick_inset, zero_half}, center, up, b);
    Rotate({-radius + tick_inset + tick_len, 0}, center, up, tip);
    const BulkPixelPoint zero_mark[3] = {a, b, tip};
    canvas.DrawPolygon(zero_mark, 3);
  }

  const Pen pitch_pen(Stroke(compact), COLOR_WHITE);
  canvas.Select(pitch_pen);

  const int pitch_10 = int(pitch_deg) / 10 * 10;
  const int pitch_span = compact ? 10 : 20;
  const Angle roll = Angle::Degrees(-bank_deg);
  for (int k = pitch_10 - pitch_span; k <= pitch_10 + pitch_span; k += 10) {
    if (k == 0)
      continue;
    DrawPitchMark(canvas, center, height, pitch_deg, bank_deg,
                  pitch_half_width, k, false);

    if (compact)
      continue;

    canvas.Select(look.mark_font);
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_WHITE);
    char buffer[5];
    StringFormatUnsafe(buffer, "%+3d", k);
    const PixelSize ts = canvas.CalcTextSize(buffer);
    const int y = PitchY(height, pitch_deg, k);
    PixelPoint label;
    Rotate({pitch_half_width + int(ts.height / 2),
            y - int(ts.height / 2)},
           center, roll, label);
    canvas.DrawText(label, buffer);
    canvas.Select(pitch_pen);
    canvas.Select(look.mark_brush);
  }

  if (!compact) {
    canvas.Select(pitch_pen);
    const int minor_half = std::max(1, pitch_half_width / 2);
    for (int minor = pitch_10 - 25; minor <= pitch_10 + 25; minor += 10)
      DrawPitchMark(canvas, center, height, pitch_deg, bank_deg,
                    minor_half, minor, true);
  }

  if (!compact) {
    const AircraftGlyph g = MakeDashGlyph(radius);
    const int gap = Fit(radius / 40, 2, radius / 10);
    const int arc_inner = radius - tick_inset - tick_len;
    const int sep = arc_inner - gap - g.span;
    const int drop = Fit(g.thick / 4, 1, g.thick);

    for (const int side : {-sep, sep})
      DrawDash(canvas, {center.x + side, center.y},
               g.span, g.half, drop,
               look.aircraft_color, look.aircraft_brush);
  }

  DrawAircraftSymbol(canvas, center, radius, look);

  const int tick_outer = tick_inset + tick_len;
  const int arrow_half_width = compact
    ? std::max(2, radius / 12)
    : Layout::Scale(4);
  const int arrow_tip = compact ? tick_outer : Layout::Scale(12);
  const int arrow_base = compact
    ? tick_outer + std::max(2, radius / 14)
    : Layout::Scale(22);

  const BulkPixelPoint triangle[3] = {
    {center.x - arrow_half_width, center.y - radius + arrow_base},
    {center.x, center.y - radius + arrow_tip},
    {center.x + arrow_half_width, center.y - radius + arrow_base},
  };

  if (!compact) {
    const int thick = MakeCenterGlyph(radius).thick;
    const int drop = Fit(thick / 3, 1, thick);
    DrawShadow(canvas, [&]{
      const BulkPixelPoint shade[3] = {
        {triangle[0].x, triangle[0].y + drop},
        {triangle[1].x, triangle[1].y + drop},
        {triangle[2].x, triangle[2].y + drop},
      };
      canvas.DrawPolygon(shade, 3);
    });
  }

  canvas.Select(Pen(Stroke(compact), look.aircraft_color));
  canvas.Select(look.aircraft_brush);
  canvas.DrawPolygon(triangle, 3);
}

/** One labelled mark is this many pixels apart. */
static int
TapePitch() noexcept
{
  const unsigned pitch = Layout::Scale(28);
  return int(pitch > 0 ? pitch : 28);
}

static int
TapeWidth(Canvas &canvas, const Font &font) noexcept
{
  if (!font.IsDefined())
    return 0;

  canvas.Select(font);
  const int text = int(canvas.CalcTextSize("00000").width);
  const int gap = int(Layout::Scale(8));
  return text + gap * 2;
}

/**
 * One even translucent plate.  The memory canvas cannot blend, so
 * the marks sit on the horizon with no plate.
 */
static void
ShadeStrip(Canvas &canvas, const PixelRect &strip) noexcept
{
#ifdef ENABLE_OPENGL
  if (!HasColors())
    return;

  const ScopeAlphaBlend blend;
  canvas.DrawFilledRectangle(strip, COLOR_BLACK.WithAlpha(140));
#else
  (void)canvas;
  (void)strip;
#endif
}

/**
 * Vertical tape beside the horizon.  @p value is already in the user
 * unit.  Ticks sit on the inner edge, toward the free space.
 */
static void
DrawTape(Canvas &canvas, const PixelRect &strip, const Font &font,
         const Font &value_font, const Color ink,
         std::optional<double> value, double step,
         bool ticks_on_right) noexcept
{
  if (strip.GetWidth() < 4 || strip.GetHeight() < 4)
    return;

  ShadeStrip(canvas, strip);

  canvas.Select(font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(ink);
  canvas.Select(Pen(PenPx(), ink));

  const int mid = (strip.top + strip.bottom) / 2;
  const int tick_len = std::max(2, int(Layout::Scale(8)));
  const int inner = ticks_on_right ? strip.right - 1 : strip.left;

  if (value && std::isfinite(*value) && step > 0 &&
      std::abs(*value) < 1000000) {
    const double px = double(TapePitch()) / step;
    const double span = (strip.GetHeight() / 2.0 + TapePitch()) / px;
    const int i0 = int(std::floor((*value - span) / step));
    const int i1 = int(std::ceil((*value + span) / step));
    if (i1 >= i0 && i1 - i0 <= 64) {
      for (int i = i0; i <= i1; ++i) {
        const double tick = i * step;
        const int y = mid - int(std::lround((tick - *value) * px));
        if (y <= strip.top || y >= strip.bottom)
          continue;

        canvas.DrawLine({inner - (ticks_on_right ? tick_len : 0), y},
                        {inner + (ticks_on_right ? 0 : tick_len), y});

        char label[16];
        StringFormatUnsafe(label, "%d", iround(tick));
        const PixelSize ls = canvas.CalcTextSize(label);
        if (y - int(ls.height) / 2 <= strip.top ||
            y + int(ls.height) / 2 >= strip.bottom)
          continue;

        const int x = ticks_on_right
          ? inner - tick_len - int(ls.width) - 2
          : inner + tick_len + 2;
        canvas.DrawText({x, y - int(ls.height) / 2}, label);
      }
    }
  }

  char bug[16] = "---";
  if (value && std::isfinite(*value))
    StringFormatUnsafe(bug, "%d", iround(*value));

  canvas.Select(value_font);
  canvas.SetTextColor(ink);
  const PixelSize ts = canvas.CalcTextSize(bug);
  const unsigned frame = std::max(1u, Layout::Scale(3u) / 4);
  const int bug_h = int(ts.height) + int(frame) * 2
    + int(Layout::Scale(2));
  const int tip = std::max(int(Layout::Scale(8)), bug_h / 3);
  const int gap = int(Layout::Scale(4));
  const int point_x = ticks_on_right
    ? strip.right - 1 - gap
    : strip.left + gap;
  const int shoulder = ticks_on_right ? point_x - tip : point_x + tip;
  const PixelRect body{ticks_on_right ? strip.left : shoulder,
                       mid - bug_h / 2,
                       ticks_on_right ? shoulder : strip.right,
                       mid + bug_h / 2};
  const Color plate = HasColors() ? COLOR_BLACK : COLOR_WHITE;
  const BulkPixelPoint chevron[3] = {
    {shoulder, body.top},
    {point_x, mid},
    {shoulder, body.bottom},
  };

  const int drop = std::max(1, int(Layout::Scale(1)));
  DrawShadow(canvas, [&]{
    const BulkPixelPoint shade_body[4] = {
      {body.left, body.top + drop},
      {body.right, body.top + drop},
      {body.right, body.bottom + drop},
      {body.left, body.bottom + drop},
    };
    const BulkPixelPoint shade_tip[3] = {
      {chevron[0].x, chevron[0].y + drop},
      {chevron[1].x, chevron[1].y + drop},
      {chevron[2].x, chevron[2].y + drop},
    };
    canvas.DrawPolygon(shade_body, 4);
    canvas.DrawPolygon(shade_tip, 3);
  });

  if (HasColors())
    canvas.SelectBlackBrush();
  else
    canvas.SelectWhiteBrush();
  canvas.SelectNullPen();
  canvas.DrawFilledRectangle(body, plate);
  canvas.DrawPolygon(chevron, 3);

  canvas.Select(Pen(frame, ink));
  if (ticks_on_right) {
    canvas.DrawLine({body.left, body.top}, {body.left, body.bottom});
    canvas.DrawLine({body.left, body.top}, {shoulder, body.top});
    canvas.DrawLine({body.left, body.bottom}, {shoulder, body.bottom});
  } else {
    canvas.DrawLine({body.right, body.top}, {body.right, body.bottom});
    canvas.DrawLine({shoulder, body.top}, {body.right, body.top});
    canvas.DrawLine({shoulder, body.bottom}, {body.right, body.bottom});
  }
  canvas.DrawLine({shoulder, body.top}, {point_x, mid});
  canvas.DrawLine({point_x, mid}, {shoulder, body.bottom});

  const int x = body.left
    + int((int(body.GetWidth()) - int(ts.width)) / 2);
  canvas.DrawText({x, mid - int(ts.height) / 2}, bug);
}

static void
DrawTapes(Canvas &canvas, const PixelRect &rc, int tape_w,
          const HorizonLook &look,
          const HorizonRenderer::HorizonReadout &readout) noexcept
{
  const Color ink = HasColors() ? COLOR_WHITE : COLOR_BLACK;
  const PixelRect speed{rc.left, rc.top, rc.left + tape_w, rc.bottom};
  const PixelRect altitude{rc.right - tape_w, rc.top,
                           rc.right, rc.bottom};

  DrawTape(canvas, speed, look.mark_font, look.value_font, ink,
           readout.speed, readout.speed_step, true);
  DrawTape(canvas, altitude, look.mark_font, look.value_font, ink,
           readout.altitude, readout.altitude_step, false);
}

void
HorizonRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                      const HorizonLook &look,
                      const AttitudeState &attitude,
                      const HorizonReadout &readout)
{
  /*
  This feature of having a backup artificial horizon based on inferred
  orientation from GPS and vario data is useful, and reasonably well
  tested, but has the issue of potentially invalidating use of XCSoar in
  FAI contests due to rule ref Annex A to Section 3 (2010 Edition) 4.1.2
  "No instruments permitting pilots to fly without visual reference to
  the ground may be carried on board, even if made unserviceable."  The
  quality of XCSoar's pseudo-AH is arguably good enough that this
  violates the rule.  We need to seek clarification as to whether this
  is the case or not.
  */

  RadarRenderer probe{Layout::Scale(1U)};
  probe.UpdateLayout(rc);
  const bool compact = IsCompact(probe.GetRadius());

  /* Sky and ground fill the whole window, under the tapes.  Marks
     stay in the gap so the plates do not cover them. */
  PixelRect ball = rc;
  int tape_w = 0;
  if (!compact) {
    tape_w = TapeWidth(canvas, look.value_font);
    if (tape_w > 0 && rc.GetWidth() > unsigned(tape_w) * 3 &&
        rc.GetHeight() > unsigned(Layout::Scale(48))) {
      ball.left += tape_w;
      ball.right -= tape_w;
    } else {
      tape_w = 0;
    }
  }

  RadarRenderer radar_renderer{Layout::Scale(1U)};
  radar_renderer.UpdateLayout(ball);

  const auto center = radar_renderer.GetCenter();
  const int radius = radar_renderer.GetRadius();
  const double bank = attitude.bank_angle_available
    ? attitude.bank_angle.Degrees()
    : 0.;
  const double pitch = attitude.pitch_angle_available
    ? attitude.pitch_angle.Degrees()
    : 0.;

  DrawSkyGround(canvas, rc, look, compact, center, bank, pitch);
  DrawScales(canvas, look, compact, center, radius,
             int(ball.GetHeight()), bank, pitch);

  if (tape_w > 0)
    DrawTapes(canvas, rc, tape_w, look, readout);

  if (!attitude.bank_angle_available && !attitude.pitch_angle_available)
    DrawNoAhrs(canvas, ball, compact);
}
