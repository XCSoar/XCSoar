// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermGeoJSONCleanup.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace XCThermGeoJSON {

namespace {

struct XY {
  double x, y;
};

/** Near-zero area in lon/lat degrees² (degenerate / collapsed). */
constexpr double AREA_EPS = 1e-18;

/** Vertex equality in lon/lat degrees. */
constexpr double VERTEX_EPS = 1e-12;

/** Treat nearly collinear turns as non-left (reflex/flat). */
constexpr double TURN_EPS = 1e-18;

[[gnu::const]]
static bool
Near(XY a, XY b) noexcept
{
  return std::fabs(a.x - b.x) <= VERTEX_EPS &&
    std::fabs(a.y - b.y) <= VERTEX_EPS;
}

[[gnu::pure]]
static XY
ToXY(const GeoPoint &p) noexcept
{
  return {p.longitude.Degrees(), p.latitude.Degrees()};
}

[[gnu::pure]]
static GeoPoint
ToGeo(XY p) noexcept
{
  return GeoPoint(Angle::Degrees(p.x), Angle::Degrees(p.y));
}

static void
StripClosingDuplicate(std::vector<XY> &pts) noexcept
{
  if (pts.size() >= 2 && Near(pts.front(), pts.back()))
    pts.pop_back();
}

static void
RemoveConsecutiveDuplicates(std::vector<XY> &pts) noexcept
{
  if (pts.empty())
    return;

  std::size_t w = 1;
  for (std::size_t r = 1; r < pts.size(); ++r) {
    if (!Near(pts[r], pts[w - 1]))
      pts[w++] = pts[r];
  }
  pts.resize(w);
}

[[gnu::pure]]
static double
SignedShoelace(const std::vector<XY> &pts) noexcept
{
  const std::size_t n = pts.size();
  if (n < 3)
    return 0;

  double sum = 0;
  for (std::size_t i = 0; i < n; ++i) {
    const XY a = pts[i];
    const XY b = pts[(i + 1) % n];
    sum += a.x * b.y - b.x * a.y;
  }
  return 0.5 * sum;
}

[[gnu::pure]]
static double
AbsShoelace(const std::vector<XY> &pts) noexcept
{
  return std::fabs(SignedShoelace(pts));
}

/** Cross product (b-a)×(c-a); positive = left turn. */
[[gnu::const]]
static double
Cross(XY a, XY b, XY c) noexcept
{
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static void
EnsureCCW(std::vector<XY> &pts) noexcept
{
  if (SignedShoelace(pts) < 0)
    std::reverse(pts.begin(), pts.end());
}

/**
 * Proper intersection of open segments AB and CD (shared endpoints do
 * not count). Returns the intersection point when segments cross.
 */
[[gnu::pure]]
static std::optional<XY>
ProperIntersection(XY a, XY b, XY c, XY d) noexcept
{
  const double rx = b.x - a.x;
  const double ry = b.y - a.y;
  const double sx = d.x - c.x;
  const double sy = d.y - c.y;
  const double den = rx * sy - ry * sx;
  if (std::fabs(den) <= VERTEX_EPS)
    return std::nullopt; /* parallel / collinear */

  const double qx = c.x - a.x;
  const double qy = c.y - a.y;
  const double t = (qx * sy - qy * sx) / den;
  const double u = (qx * ry - qy * rx) / den;

  /* Strictly between endpoints — excludes shared vertices. */
  constexpr double lo = 1e-9;
  constexpr double hi = 1.0 - 1e-9;
  if (t <= lo || t >= hi || u <= lo || u >= hi)
    return std::nullopt;

  return XY{a.x + t * rx, a.y + t * ry};
}

struct Crossing {
  std::size_t i; /* edge pts[i] → pts[i+1] */
  std::size_t j; /* edge pts[j] → pts[j+1] */
  XY point;
};

[[gnu::pure]]
static std::optional<Crossing>
FindFirstCrossing(const std::vector<XY> &pts) noexcept
{
  const std::size_t n = pts.size();
  if (n < 4)
    return std::nullopt;

  for (std::size_t i = 0; i < n; ++i) {
    const XY a = pts[i];
    const XY b = pts[(i + 1) % n];
    for (std::size_t j = i + 1; j < n; ++j) {
      /* Skip adjacent edges and the wrap-around adjacent pair. */
      if (j == i || j == (i + 1) % n || i == (j + 1) % n)
        continue;
      if (i == 0 && j == n - 1)
        continue;

      if (auto hit = ProperIntersection(a, b, pts[j],
                                        pts[(j + 1) % n])) {
        Crossing c;
        c.i = i;
        c.j = j;
        c.point = *hit;
        return c;
      }
    }
  }

  return std::nullopt;
}

/**
 * Split a ring at one proper edge crossing into two open rings
 * (no repeated closing vertex).
 */
static std::pair<std::vector<XY>, std::vector<XY>>
SplitAtCrossing(const std::vector<XY> &pts, Crossing c) noexcept
{
  const std::size_t n = pts.size();
  std::vector<XY> a, b;
  a.reserve(n);
  b.reserve(n);

  a.push_back(c.point);
  for (std::size_t k = (c.i + 1) % n; k != (c.j + 1) % n;
       k = (k + 1) % n)
    a.push_back(pts[k]);

  b.push_back(c.point);
  for (std::size_t k = (c.j + 1) % n; k != (c.i + 1) % n;
       k = (k + 1) % n)
    b.push_back(pts[k]);

  return {std::move(a), std::move(b)};
}

static Ring
ToClosedRing(const std::vector<XY> &pts) noexcept
{
  Ring ring;
  ring.reserve(pts.size() + 1);
  for (const XY p : pts)
    ring.push_back(ToGeo(p));
  if (!ring.empty())
    ring.push_back(ring.front());
  return ring;
}

[[gnu::pure]]
static bool
IsConvexCCW(const std::vector<XY> &pts) noexcept
{
  const std::size_t n = pts.size();
  if (n < 3)
    return false;

  for (std::size_t i = 0; i < n; ++i) {
    const XY a = pts[(i + n - 1) % n];
    const XY b = pts[i];
    const XY c = pts[(i + 1) % n];
    if (Cross(a, b, c) < -TURN_EPS)
      return false;
  }
  return true;
}

[[gnu::pure]]
static std::optional<std::size_t>
FindReflex(const std::vector<XY> &pts) noexcept
{
  const std::size_t n = pts.size();
  for (std::size_t i = 0; i < n; ++i) {
    const XY a = pts[(i + n - 1) % n];
    const XY b = pts[i];
    const XY c = pts[(i + 1) % n];
    if (Cross(a, b, c) < -TURN_EPS)
      return i;
  }
  return std::nullopt;
}

/** Ray-cast point-in-polygon (lon/lat as plane). */
[[gnu::pure]]
static bool
PointInRing(const std::vector<XY> &pts, XY p) noexcept
{
  bool inside = false;
  const std::size_t n = pts.size();
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
    const XY a = pts[i];
    const XY b = pts[j];
    if (((a.y > p.y) != (b.y > p.y)) &&
        (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x))
      inside = !inside;
  }
  return inside;
}

[[gnu::pure]]
static bool
SegmentClear(const std::vector<XY> &pts,
             std::size_t i, std::size_t j) noexcept
{
  const std::size_t n = pts.size();
  const XY a = pts[i];
  const XY b = pts[j];

  for (std::size_t e = 0; e < n; ++e) {
    const std::size_t f = (e + 1) % n;
    /* Skip edges incident to i or j. */
    if (e == i || f == i || e == j || f == j)
      continue;
    if (ProperIntersection(a, b, pts[e], pts[f]))
      return false;
  }
  return true;
}

/**
 * True if ij is an internal diagonal of a simple CCW ring.
 */
[[gnu::pure]]
static bool
DiagonalInside(const std::vector<XY> &pts,
               std::size_t i, std::size_t j) noexcept
{
  const std::size_t n = pts.size();
  if (n < 4)
    return false;

  const std::size_t d = (j + n - i) % n;
  if (d <= 1 || d >= n - 1)
    return false; /* adjacent */

  if (!SegmentClear(pts, i, j))
    return false;

  const XY mid{
    0.5 * (pts[i].x + pts[j].x),
    0.5 * (pts[i].y + pts[j].y),
  };
  return PointInRing(pts, mid);
}

static std::pair<std::vector<XY>, std::vector<XY>>
SplitAtDiagonal(const std::vector<XY> &pts,
                std::size_t i, std::size_t j) noexcept
{
  const std::size_t n = pts.size();
  if (i > j)
    std::swap(i, j);

  std::vector<XY> a, b;
  a.reserve(j - i + 1);
  b.reserve(n - (j - i) + 1);

  for (std::size_t k = i; k <= j; ++k)
    a.push_back(pts[k]);

  for (std::size_t k = j; k < n; ++k)
    b.push_back(pts[k]);
  for (std::size_t k = 0; k <= i; ++k)
    b.push_back(pts[k]);

  return {std::move(a), std::move(b)};
}

/**
 * Ear-clip into triangles when a diagonal split cannot be found.
 * Each triangle is convex.
 */
static void
TriangulateEars(std::vector<XY> pts,
                std::vector<std::vector<XY>> &convex_out,
                unsigned depth) noexcept
{
  if (depth > 64 || pts.size() < 3)
    return;

  RemoveConsecutiveDuplicates(pts);
  StripClosingDuplicate(pts);
  if (pts.size() < 3 || AbsShoelace(pts) <= AREA_EPS)
    return;

  EnsureCCW(pts);
  if (pts.size() == 3) {
    convex_out.push_back(std::move(pts));
    return;
  }

  const std::size_t n = pts.size();
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t prev = (i + n - 1) % n;
    const std::size_t next = (i + 1) % n;
    if (Cross(pts[prev], pts[i], pts[next]) <= TURN_EPS)
      continue; /* not a convex ear tip */
    if (!DiagonalInside(pts, prev, next))
      continue;

    convex_out.push_back({pts[prev], pts[i], pts[next]});

    std::vector<XY> rest;
    rest.reserve(n - 1);
    for (std::size_t k = 0; k < n; ++k)
      if (k != i)
        rest.push_back(pts[k]);
    TriangulateEars(std::move(rest), convex_out, depth + 1);
    return;
  }

  /* No ear found — drop as unrecoverable. */
}

/**
 * Recursively split a simple CCW ring into convex pieces.
 */
static void
DecomposeConvex(std::vector<XY> pts,
                std::vector<std::vector<XY>> &convex_out,
                unsigned depth) noexcept
{
  if (depth > 64)
    return;

  RemoveConsecutiveDuplicates(pts);
  StripClosingDuplicate(pts);
  if (pts.size() < 3 || AbsShoelace(pts) <= AREA_EPS)
    return;

  EnsureCCW(pts);

  if (IsConvexCCW(pts)) {
    convex_out.push_back(std::move(pts));
    return;
  }

  const auto reflex = FindReflex(pts);
  if (!reflex) {
    /* Numerically odd but not marked reflex — force triangulation. */
    TriangulateEars(std::move(pts), convex_out, 0);
    return;
  }

  const std::size_t i = *reflex;
  const std::size_t n = pts.size();
  for (std::size_t j = 0; j < n; ++j) {
    if (!DiagonalInside(pts, i, j))
      continue;

    auto [left, right] = SplitAtDiagonal(pts, i, j);
    DecomposeConvex(std::move(left), convex_out, depth + 1);
    DecomposeConvex(std::move(right), convex_out, depth + 1);
    return;
  }

  TriangulateEars(std::move(pts), convex_out, 0);
}

static void
AppendCleaned(std::vector<XY> pts,
              std::vector<std::vector<Ring>> &out,
              unsigned depth) noexcept
{
  /* Pathological rings could recurse; bail out rather than blow the
     stack — the leftover geometry is dropped as junk. */
  if (depth > 32)
    return;

  RemoveConsecutiveDuplicates(pts);
  StripClosingDuplicate(pts);

  if (pts.size() < 3)
    return;

  /* Self-crossing rings often have near-zero *signed* area (lobes
     cancel).  Split before the area junk check. */
  if (auto cross = FindFirstCrossing(pts)) {
    auto [left, right] = SplitAtCrossing(pts, *cross);
    AppendCleaned(std::move(left), out, depth + 1);
    AppendCleaned(std::move(right), out, depth + 1);
    return;
  }

  if (AbsShoelace(pts) <= AREA_EPS)
    return;

  /* Simple ring: cut into convex pieces for reliable ear-clip draw. */
  std::vector<std::vector<XY>> convex_parts;
  DecomposeConvex(std::move(pts), convex_parts, 0);
  for (auto &part : convex_parts) {
    if (part.size() >= 3 && AbsShoelace(part) > AREA_EPS)
      out.push_back({ToClosedRing(part)});
  }
}

} // namespace

std::vector<std::vector<Ring>>
CleanExterior(const Ring &exterior) noexcept
{
  std::vector<XY> pts;
  pts.reserve(exterior.size());
  for (const GeoPoint &p : exterior)
    pts.push_back(ToXY(p));

  std::vector<std::vector<Ring>> out;
  AppendCleaned(std::move(pts), out, 0);
  return out;
}

void
CleanBandPolygons(WindBand &band) noexcept
{
  std::vector<std::vector<Ring>> cleaned;
  cleaned.reserve(band.polygons.size());

  for (const auto &polygon : band.polygons) {
    if (polygon.empty())
      continue;

    /* Holes are not drawn — discard them at import. */
    auto parts = CleanExterior(polygon[0]);
    for (auto &part : parts)
      cleaned.push_back(std::move(part));
  }

  band.polygons = std::move(cleaned);
}

void
SortBandsByAbsMid(ForecastLayer &layer) noexcept
{
  std::sort(layer.bands.begin(), layer.bands.end(),
            [](const WindBand &a, const WindBand &b) noexcept {
              const double mid_a = (a.min_ms + a.max_ms) * 0.5;
              const double mid_b = (b.min_ms + b.max_ms) * 0.5;
              return std::fabs(mid_a) < std::fabs(mid_b);
            });
}

} // namespace XCThermGeoJSON
