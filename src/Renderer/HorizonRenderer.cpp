// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonRenderer.hpp"
#include "Asset.hpp"
#include "Look/AutoFont.hpp"
#include "Look/FontDescription.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Look/HorizonLook.hpp"
#include "Math/Angle.hpp"
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

/* Window: faces, and how the rect is split. */

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
 * Pixels on the value face.  @p design is the size at an 18px face.
 * A zero face falls back to Layout::Scale for the InfoBox.
 */
static int
FacePx(int face, int design) noexcept
{
  if (face <= 0)
    return int(Layout::Scale(std::max(design, 1)));

  return std::max(1, face * design / 18);
}

/**
 * Dial fraction, raised to the value face when that floor still
 * fits in @p cap.  The radius is already in screen pixels.
 */
static int
Fit(int proportional, int design_px, int cap, int face) noexcept
{
  const int floor = FacePx(face, std::max(design_px, 1));
  int v = std::max(proportional, floor);
  if (v > cap)
    v = std::max(proportional, 1);
  return std::max(v, 1);
}

/** One labelled mark is this many pixels apart. */
static int
TapePitch() noexcept
{
  const unsigned pitch = Layout::Scale(28);
  return int(pitch > 0 ? pitch : 28);
}

static int
TapeWidth(Canvas &canvas, const Font &font, const char *sample) noexcept
{
  if (!font.IsDefined())
    return 0;

  canvas.Select(font);
  const int text = int(canvas.CalcTextSize(sample).width);
  const int gap = FacePx(int(font.GetHeight()), 8);
  return text + gap * 2;
}

static int
CourseHeight(Canvas &canvas, const Font &font) noexcept
{
  if (!font.IsDefined())
    return 0;

  canvas.Select(font);
  const int text = int(canvas.CalcTextSize("000").height);
  const int face = std::max(1, int(font.GetHeight()));
  const int gap = FacePx(face, 4);
  const int tip = std::max(FacePx(face, 8), text / 3);
  return text + gap + tip + gap;
}

/** Side column.  A fifth left the five-digit value too small. */
static unsigned
TapeBudget(unsigned width) noexcept
{
  return width / 4;
}

/**
 * What a fitted face was built for.  Resize changes the rect.  DPI,
 * text size, and viewing distance change the layout scales without a
 * new rect, and the face has to be built again.
 */
struct FaceKey {
  unsigned width = 0, height = 0, scale = 0, font = 0;

  [[nodiscard]]
  static FaceKey Now(unsigned width, unsigned height) noexcept {
    return {width, height, Layout::scale_1024, Layout::font_scale};
  }

  [[nodiscard]]
  bool Matches(unsigned width, unsigned height) const noexcept {
    return *this == Now(width, height);
  }

  [[nodiscard]]
  constexpr bool operator==(const FaceKey &other) const noexcept {
    return width == other.width && height == other.height &&
           scale == other.scale && font == other.font;
  }
};

/**
 * Bold face for the ribbon.  The full horizon uses a short tag,
 * three eighths of the width.  The InfoBox uses three quarters of
 * its box.  Separate faces: the text cache keys a Font by address.
 * The faces are leaked so quit does not destroy them after the
 * screen is gone.
 */
static const Font *
AhrsFont(const PixelRect &rc, const char *label, bool large) noexcept
{
  static Font *stored[2] = {};
  static FaceKey key[2];

  const unsigned slot = large ? 1u : 0u;

  const unsigned width = rc.GetWidth();
  const unsigned height = rc.GetHeight();
  if (width < 8 || height < 8)
    return nullptr;

  if (stored[slot] == nullptr) {
    try {
      stored[slot] = new Font();
    } catch (...) {
      return nullptr;
    }
  }
  Font &font = *stored[slot];

  if (font.IsDefined() && key[slot].Matches(width, height))
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

  key[slot] = FaceKey::Now(width, height);
  return &font;
}

struct HorizonFace {
  const Font *mark;
  const Font *value;
  int face;
};

/**
 * Value and tick faces fitted to this horizon.  Leaked like the
 * ribbon face, so quit does not destroy them after the screen is
 * gone.  A Layout-scaled face is wider than a high-DPI window and
 * the side tapes were dropped.  Refit when the rect or the layout
 * scale changes.
 */
static HorizonFace
LoadHorizonFace(const PixelRect &rc, const HorizonLook &look) noexcept
{
  static Font *mark_store;
  static Font *value_store;
  static FaceKey key;

  const unsigned width = rc.GetWidth();
  const unsigned height = rc.GetHeight();
  const unsigned budget = TapeBudget(width);
  if (budget < 16 || height < 16)
    return {&look.mark_font, &look.value_font,
            int(look.value_font.GetHeight())};

  if (mark_store != nullptr && value_store != nullptr &&
      key.Matches(width, height) &&
      mark_store->IsDefined() && value_store->IsDefined())
    return {mark_store, value_store, int(value_store->GetHeight())};

  try {
    if (mark_store == nullptr)
      mark_store = new Font();
    if (value_store == nullptr)
      value_store = new Font();

    FontDescription value_d(10, true);
    /* Three fifths of the column.  Filling it made the chevron digits huge. */
    AutoSizeFont(value_d, std::max(8u, budget * 3 / 5), "00000");
    /* The course strip is about twice this face and stays under a third. */
    const unsigned max_h = std::max(8u, height / 8);
    if (value_d.GetHeight() > max_h)
      value_d.SetHeight(max_h);
    value_store->Load(value_d);

    const unsigned mark_h = std::max(6u, value_d.GetHeight() * 2 / 3);
    mark_store->Load(FontDescription(mark_h));
  } catch (...) {
    return {&look.mark_font, &look.value_font,
            int(look.value_font.GetHeight())};
  }

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif

  key = FaceKey::Now(width, height);
  return {mark_store, value_store, int(value_store->GetHeight())};
}

struct TapeColumns {
  int speed_w = 0;
  int alt_w = 0;
  int course_h = 0;
};

/** Column widths, or zeros when the horizon is too small for tapes. */
static TapeColumns
FitTapes(Canvas &canvas, const PixelRect &rc, const Font &value) noexcept
{
  TapeColumns cols;
  cols.course_h = CourseHeight(canvas, value);
  if (cols.course_h <= 0 ||
      rc.GetHeight() <= unsigned(cols.course_h) * 3)
    cols.course_h = 0;

  /* Altitude is five digits, airspeed three, so the right column
     is wider. */
  const int budget = int(TapeBudget(rc.GetWidth()));
  cols.alt_w = TapeWidth(canvas, value, "00000");
  cols.speed_w = TapeWidth(canvas, value, "000");
  if (budget > 0 && cols.alt_w > budget)
    cols.alt_w = budget;
  if (cols.alt_w > 0 && cols.speed_w > cols.alt_w)
    cols.speed_w = cols.alt_w;

  const unsigned dial_h = rc.GetHeight() - unsigned(cols.course_h);
  const unsigned pair = unsigned(cols.speed_w + cols.alt_w);
  if (!(cols.speed_w > 0 && cols.alt_w > 0 &&
        rc.GetWidth() > pair + unsigned(cols.alt_w) &&
        dial_h > unsigned(cols.alt_w))) {
    cols.speed_w = 0;
    cols.alt_w = 0;
  }
  return cols;
}

/* Helpers. */

static void
Rotate(PixelPoint point, PixelPoint center, Angle a,
       PixelPoint &rotated) noexcept
{
  rotated.x = int(point.x * a.cos() - point.y * a.sin() + center.x);
  rotated.y = int(point.x * a.sin() + point.y * a.cos() + center.y);
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

static int
PitchY(int height, double pitch, int deg) noexcept
{
  return int(height / 2.f * (pitch - deg) / MAX_PITCH_DEG);
}

/** Center reference: a low "-^-", longer than the side dashes. */
struct AircraftGlyph {
  int span, gap, half, peak, thick;
};

static AircraftGlyph
MakeCenterGlyph(int radius, int face) noexcept
{
  AircraftGlyph g;
  /* The InfoBox follows the dial.  The full horizon follows the
     value face, and still stays inside the circle. */
  if (IsCompact(radius)) {
    g.span = std::max(3, radius / 6);
    g.thick = 2;
    g.half = 2;
    g.peak = std::max(g.half + 2, g.span / 6);
    g.gap = std::max(2, g.span / 3);
    return g;
  }

  g.span = Fit(radius / 3, 8, radius / 2, face);
  g.thick = Fit(g.span / 12, 2, std::max(2, g.span / 4), face);
  g.half = std::max(1, g.thick / 2);
  g.peak = Fit(g.span / 6, 2, std::max(g.half + 1, g.span / 3), face);
  g.peak = std::max(g.peak, g.half + 1);
  g.gap = Fit(g.span / 3, 1, std::max(g.half + 1, g.span * 2 / 3),
              face);
  g.gap = std::max(g.gap, g.half + 1);
  return g;
}

static AircraftGlyph
MakeDashGlyph(int radius, int face) noexcept
{
  AircraftGlyph g;
  g.span = Fit(radius / 12, 4, radius / 6, face);
  g.thick = Fit(g.span / 4, 2, std::max(2, g.span / 2), face);
  g.half = std::max(1, g.thick / 2);
  g.peak = 0;
  g.gap = 0;
  return g;
}

/** Bearing in 0..359. */
static int
BearingDeg(double deg) noexcept
{
  int v = iround(deg) % 360;
  if (v < 0)
    v += 360;
  return v;
}

/* Drawing, back to front. */

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

static void
DrawAircraftSymbol(Canvas &canvas, const PixelPoint &center, int radius,
                   const HorizonLook &look, int face) noexcept
{
  const AircraftGlyph g = MakeCenterGlyph(radius, face);
  const bool compact = IsCompact(radius);

  if (!compact)
    DrawShadow(canvas, [&]{
      DrawChevron(canvas, center, g.span, g.gap, g.half, g.peak,
                  Fit(g.thick / 3, 1, g.thick, face));
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

static void
DrawScales(Canvas &canvas, const HorizonLook &look, bool compact,
           PixelPoint center, int radius, int height,
           double bank_deg, double pitch_deg,
           const Font &label_font, int face) noexcept
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

    canvas.Select(label_font);
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
    const AircraftGlyph g = MakeDashGlyph(radius, face);
    const int gap = Fit(radius / 40, 2, radius / 10, face);
    const int arc_inner = radius - tick_inset - tick_len;
    const int sep = arc_inner - gap - g.span;
    const int drop = Fit(g.thick / 4, 1, g.thick, face);

    for (const int side : {-sep, sep})
      DrawDash(canvas, {center.x + side, center.y},
               g.span, g.half, drop,
               look.aircraft_color, look.aircraft_brush);
  }

  DrawAircraftSymbol(canvas, center, radius, look, face);

  const int tick_outer = tick_inset + tick_len;
  const int arrow_half_width = compact
    ? std::max(2, radius / 12)
    : std::min(std::max(2, radius / 10), FacePx(face, 4));
  const int arrow_tip = compact
    ? tick_outer
    : std::min(std::max(2, radius / 6), FacePx(face, 12));
  const int arrow_base = compact
    ? tick_outer + std::max(2, radius / 14)
    : std::min(std::max(arrow_tip + 1, radius / 4), FacePx(face, 22));

  const BulkPixelPoint triangle[3] = {
    {center.x - arrow_half_width, center.y - radius + arrow_base},
    {center.x, center.y - radius + arrow_tip},
    {center.x + arrow_half_width, center.y - radius + arrow_base},
  };

  if (!compact) {
    const int thick = MakeCenterGlyph(radius, face).thick;
    const int drop = Fit(thick / 3, 1, thick, face);
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

/** Filled plate and triangle, with the drop shadow under both. */
static void
DrawBugFill(Canvas &canvas, const PixelRect &body,
            const BulkPixelPoint chevron[3], int face) noexcept
{
  const Color plate = HasColors() ? COLOR_BLACK : COLOR_WHITE;
  const int drop = std::max(1, FacePx(face, 1));
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
}

/**
 * Vertical tape beside the horizon.  @p value is already in the user
 * unit.  Ticks sit on the inner edge, toward the free space.
 * @p speed is the left tape: its scale stops at zero.
 */
static void
DrawTape(Canvas &canvas, const PixelRect &strip, const Font &font,
         const Font &value_font, const Color ink,
         std::optional<double> value, double step, bool speed) noexcept
{
  if (strip.GetWidth() < 4 || strip.GetHeight() < 4)
    return;

  ShadeStrip(canvas, strip);

  canvas.Select(font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(ink);
  canvas.Select(Pen(PenPx(), ink));

  const int face = std::max(1, int(value_font.GetHeight()));
  const bool ticks_on_right = speed;
  const int mid = (strip.top + strip.bottom) / 2;
  const int tick_len = std::min(int(strip.GetWidth()) / 5,
                                std::max(2, FacePx(face, 8)));
  const int inner = ticks_on_right ? strip.right - 1 : strip.left;

  if (value && std::isfinite(*value) && step > 0 &&
      std::abs(*value) < 1000000) {
    const double px = double(TapePitch()) / step;
    const double span = (strip.GetHeight() / 2.0 + TapePitch()) / px;
    int i0 = int(std::floor((*value - span) / step));
    const int i1 = int(std::ceil((*value + span) / step));
    /* The speed scale stops at zero. */
    if (speed && i0 < 0)
      i0 = 0;
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
  const unsigned frame = unsigned(FacePx(face, 2));
  const int bug_h = int(ts.height) + int(frame) * 2
    + FacePx(face, 2);
  const int tip = std::max(FacePx(face, 6), bug_h / 4);
  /* Hug the digits, and sit the plate on the window edge. */
  const int body_w = int(ts.width) + int(frame) * 2 + FacePx(face, 4);
  int shoulder, point_x;
  if (ticks_on_right) {
    const int edge = strip.left;
    shoulder = edge + body_w;
    point_x = shoulder + tip;
    if (point_x >= strip.right) {
      point_x = strip.right - 1;
      shoulder = std::max(edge, point_x - tip);
    }
  } else {
    const int edge = strip.right;
    shoulder = edge - body_w;
    point_x = shoulder - tip;
    if (point_x < strip.left) {
      point_x = strip.left;
      shoulder = std::min(edge, point_x + tip);
    }
  }
  const PixelRect body{ticks_on_right ? strip.left : shoulder,
                       mid - bug_h / 2,
                       ticks_on_right ? shoulder : strip.right,
                       mid + bug_h / 2};
  const BulkPixelPoint chevron[3] = {
    {shoulder, body.top},
    {point_x, mid},
    {shoulder, body.bottom},
  };
  DrawBugFill(canvas, body, chevron, face);

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

/**
 * Horizontal track tape along the top.  Higher bearings sit to
 * the right.  The centre bug is the current true track.
 */
static void
DrawCourse(Canvas &canvas, const PixelRect &strip, const Font &font,
           const Font &value_font, const Color ink,
           std::optional<double> course) noexcept
{
  if (strip.GetWidth() < 4 || strip.GetHeight() < 4)
    return;

  ShadeStrip(canvas, strip);

  canvas.Select(font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(ink);
  canvas.Select(Pen(PenPx(), ink));

  const int face = std::max(1, int(value_font.GetHeight()));
  const int mid = (strip.left + strip.right) / 2;
  const int tick_len = std::min(int(strip.GetHeight()) / 4,
                                std::max(2, FacePx(face, 8)));
  const int edge = strip.bottom - 1;
  constexpr double step = 10;

  if (course && std::isfinite(*course)) {
    const double px = double(TapePitch()) / step;
    const double span = (strip.GetWidth() / 2.0 + TapePitch()) / px;
    const int i0 = int(std::floor((*course - span) / step));
    const int i1 = int(std::ceil((*course + span) / step));
    if (i1 >= i0 && i1 - i0 <= 64) {
      for (int i = i0; i <= i1; ++i) {
        const double tick = i * step;
        const int x = mid + int(std::lround((tick - *course) * px));
        if (x <= strip.left || x >= strip.right)
          continue;

        canvas.DrawLine({x, edge - tick_len}, {x, edge});

        if (BearingDeg(tick) % 30 != 0)
          continue;

        char label[8];
        StringFormatUnsafe(label, "%d", BearingDeg(tick));
        const PixelSize ls = canvas.CalcTextSize(label);
        const int label_y = edge - tick_len - int(ls.height);
        if (label_y < strip.top ||
            x - int(ls.width) / 2 <= strip.left ||
            x + int(ls.width) / 2 >= strip.right)
          continue;

        canvas.DrawText({x - int(ls.width) / 2, label_y}, label);
      }
    }
  }

  char bug[16] = "---";
  if (course && std::isfinite(*course))
    FormatBearing(bug, sizeof(bug), Angle::Degrees(*course));

  canvas.Select(value_font);
  canvas.SetTextColor(ink);
  const PixelSize ts = canvas.CalcTextSize(bug);
  const unsigned frame = unsigned(FacePx(face, 2));
  const int gap = std::min(int(strip.GetHeight()) / 6, FacePx(face, 4));
  const int tip = std::min(int(strip.GetHeight()) / 3,
                           std::max(FacePx(face, 8), int(ts.height) / 3));
  const int point_y = strip.bottom - 1 - gap;
  const int shoulder = point_y - tip;
  const int body_w = int(ts.width) + int(frame) * 2 + FacePx(face, 4);
  const PixelRect body{mid - body_w / 2, strip.top,
                       mid + body_w / 2, shoulder};
  const BulkPixelPoint chevron[3] = {
    {body.left, shoulder},
    {mid, point_y},
    {body.right, shoulder},
  };
  DrawBugFill(canvas, body, chevron, face);

  canvas.Select(Pen(frame, ink));
  canvas.DrawLine({body.left, body.top}, {body.right, body.top});
  canvas.DrawLine({body.left, body.top}, {body.left, shoulder});
  canvas.DrawLine({body.right, body.top}, {body.right, shoulder});
  canvas.DrawLine({body.left, shoulder}, {mid, point_y});
  canvas.DrawLine({mid, point_y}, {body.right, shoulder});

  const int x = body.left
    + int((int(body.GetWidth()) - int(ts.width)) / 2);
  canvas.DrawText({x, body.top + int((int(body.GetHeight())
                                      - int(ts.height)) / 2)},
                  bug);
}

static void
DrawTapes(Canvas &canvas, const PixelRect &rc, int speed_w, int alt_w,
          const Font &mark, const Font &value,
          const HorizonRenderer::HorizonReadout &readout) noexcept
{
  const Color ink = HasColors() ? COLOR_WHITE : COLOR_BLACK;
  const PixelRect speed{rc.left, rc.top, rc.left + speed_w, rc.bottom};
  const PixelRect altitude{rc.right - alt_w, rc.top,
                           rc.right, rc.bottom};

  DrawTape(canvas, speed, mark, value, ink,
           readout.speed, readout.speed_step, true);
  DrawTape(canvas, altitude, mark, value, ink,
           readout.altitude, readout.altitude_step, false);
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

  const char *label = "NO AHRS";
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
  const HorizonFace fonts = compact
    ? HorizonFace{&look.mark_font, &look.value_font, 0}
    : LoadHorizonFace(rc, look);

  /* Sky and ground fill the whole window, under the tapes.  Marks
     stay in the gap so the plates do not cover them. */
  const TapeColumns cols = compact
    ? TapeColumns{}
    : FitTapes(canvas, rc, *fonts.value);
  PixelRect ball = rc;
  if (cols.speed_w > 0) {
    ball.left += cols.speed_w;
    ball.right -= cols.alt_w;
  }
  if (cols.course_h > 0)
    ball.top += cols.course_h;

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
             int(ball.GetHeight()), bank, pitch,
             *fonts.mark, fonts.face);

  if (cols.speed_w > 0 && cols.alt_w > 0)
    DrawTapes(canvas, rc, cols.speed_w, cols.alt_w,
              *fonts.mark, *fonts.value, readout);

  if (cols.course_h > 0) {
    const Color ink = HasColors() ? COLOR_WHITE : COLOR_BLACK;
    const PixelRect strip{rc.left + cols.speed_w, rc.top,
                          rc.right - cols.alt_w,
                          rc.top + cols.course_h};
    DrawCourse(canvas, strip, *fonts.mark, *fonts.value, ink,
               readout.course);
  }

  if (!attitude.bank_angle_available && !attitude.pitch_angle_available)
    DrawNoAhrs(canvas, ball, compact);
}

