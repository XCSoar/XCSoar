// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * Orbit a patch of a terrain file.  The mesh is a height field in a
 * local east/north frame, coloured with a map terrain ramp and lit by
 * one directional light.  Samples outside --range are omitted.
 */

#define ENABLE_MAIN_WINDOW
#define ENABLE_CMDLINE
#define USAGE "[-WxH] [--range METRES] [--pitch DEG] [--ramp N] " \
              "[--lat DEG] [--lon DEG] [--airspace PATH] PATH"

#include "Main.hpp"
#include "ProductName.hpp"
#include "Version.hpp"
#include "system/StandardVersion.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/Loader.hpp"
#include "Terrain/TerrainColor.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/XShape.hpp"
#include "Projection/WindowProjection.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/ZipLineReader.hpp"
#include "io/BufferedReader.hxx"
#include "io/FileReader.hxx"
#include "io/ZipReader.hpp"
#include "Geo/GeoVector.hpp"
#include "io/ZipArchive.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/Airspaces.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "thread/SharedMutex.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/event/Timer.hpp"
#include "util/StringAPI.hxx"
#include "Math/Util.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Attribute.hpp"
#include "ui/canvas/opengl/FrameBuffer.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/RenderBuffer.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/opengl/System.hpp"

#include <cstddef>
#include <memory>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static constexpr const char canonical_name[] = "RunTerrainView";

/* Four times the first mesh.  At the default 20 km radius the
   cap wins, and a quad is about 100 m instead of 420 m. */
static constexpr unsigned MAX_N = 384;
static constexpr double CELL_M = 55;
static constexpr double FOV_DEG = 45;
/** Lowest the eye may sit above the ground under it. */
static constexpr double MIN_AGL = 50;

static Path terrain_path{nullptr};
static double range_m = 20000;
static double pitch_deg = 45;
static unsigned ramp_index = 0;
static bool have_lat, have_lon;
static double start_lat, start_lon;
static Path airspace_path{nullptr};
static Airspaces airspaces;
static AirspaceRendererSettings airspace_look;
static bool have_airspace;

struct Face {
  BulkPixelPoint p[4];
  float z[4];
  Color color[4];
  double depth;
};

/** Map camera depth into the canvas ortho, where nearer is GL_LESS. */
static float
ClipZ(double depth) noexcept
{
  const double far_m = range_m * 12;
  double t = (depth - 20) / (far_m - 20);
  if (t < 0)
    t = 0;
  if (t > 1)
    t = 1;
  return float(1 - 2 * t);
}

#ifdef ENABLE_OPENGL
struct TerrainVertex {
  float x, y, z;
  uint8_t r, g, b, a;
};
#endif

struct Vec3 {
  double x, y, z;
};

struct Cell {
  double e, n, z;
  bool ok;
  bool water;
  /** Height was read for this lattice point. */
  bool sampled;
  bool valid;
  int li, lj;
};

static Cell cells[MAX_N][MAX_N];
static Color face_color[MAX_N][MAX_N];
static bool face_ok[MAX_N][MAX_N];

static bool face_lit[MAX_N][MAX_N];
static int face_li[MAX_N][MAX_N];
static int face_lj[MAX_N][MAX_N];
static unsigned grid_n = 0;
static double target_z = 0;
/** Fixed ground origin. Lattice points are CELL_M multiples from here. */
static GeoPoint mesh_origin;
static bool mesh_origin_set;
static int anchor_i, anchor_j;
static double anchor_e, anchor_n;
static bool anchor_set;
/** World spacing of the lattice. CELL_M until the sample cap widens it. */
static double lattice_m = CELL_M;

struct TopoNode {
  float e, n, z;
  uint8_t r, g, b, a;
  bool start;
};

static std::vector<TopoNode> topo_nodes;
/** Area triangles, three nodes each. */
static std::vector<TopoNode> topo_fill;

struct ScreenTri {
  BulkPixelPoint p[3];
  float z[3];
  Color color;
};

static std::vector<ScreenTri> screen_fill;

static ZipArchive *terrain_zip;
static RasterMap *terrain_map;
static SharedMutex terrain_mutex;
static GeoPoint view_center;
/** Center the loaded topology was built for. */
static GeoPoint topo_center;
static bool topo_center_set;
/** Fine tiles arrived after the shapes were sampled. */
static bool topo_resync;
static Serial terrain_serial;
static TopographyStore *topo_store;
static bool have_topo;

#ifdef ENABLE_OPENGL
static std::vector<TerrainVertex> topo_draw;
#endif

static Vec3
Cross(Vec3 a, Vec3 b) noexcept
{
  return {a.y * b.z - a.z * b.y,
          a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

static double
Dot(Vec3 a, Vec3 b) noexcept
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3
Normalise(Vec3 v) noexcept
{
  const double len = std::sqrt(Dot(v, v));
  if (len < 1e-6)
    return {0, 0, 1};
  return {v.x / len, v.y / len, v.z / len};
}

/** Light travels from the north-west, above the ground. */
static const Vec3 light = Normalise({-1, 1, 1.3});

static Color
Lit(RGB8Color base, double ndotl) noexcept
{
  const double shade = std::min(1.0, 0.30 + 0.70 * std::max(0.0, ndotl));
  return Color(uint8_t(base.Red() * shade),
               uint8_t(base.Green() * shade),
               uint8_t(base.Blue() * shade));
}

/** One normal per vertex, shared by every quad that meets there. */
static Color
ShadeVertex(unsigned iz, unsigned ix) noexcept
{
  const Cell &c = cells[iz][ix];
  const RGB8Color base = LookupTerrainColor(ramp_index, int(c.z), c.water);

  double e0 = c.e, e1 = c.e, z0 = c.z, z1 = c.z;
  double n0 = c.n, n1 = c.n, zn0 = c.z, zn1 = c.z;
  if (ix > 0 && cells[iz][ix - 1].ok) {
    e0 = cells[iz][ix - 1].e;
    z0 = cells[iz][ix - 1].z;
  }
  if (ix + 1 < grid_n && cells[iz][ix + 1].ok) {
    e1 = cells[iz][ix + 1].e;
    z1 = cells[iz][ix + 1].z;
  }
  if (iz > 0 && cells[iz - 1][ix].ok) {
    n0 = cells[iz - 1][ix].n;
    zn0 = cells[iz - 1][ix].z;
  }
  if (iz + 1 < grid_n && cells[iz + 1][ix].ok) {
    n1 = cells[iz + 1][ix].n;
    zn1 = cells[iz + 1][ix].z;
  }

  const double de = e1 - e0;
  const double dn = n1 - n0;
  if (std::abs(de) < 1e-3 || std::abs(dn) < 1e-3)
    return Lit(base, 1);

  const Vec3 normal = Normalise({-(z1 - z0) * dn, -de * (zn1 - zn0),
                                 de * dn});
  return Lit(base, Dot(normal, light));
}

static void
PrintHelp() noexcept
{
  std::printf(
    "Usage: %s [-WxH] [--range METRES] [--pitch DEG] [--ramp N]\n"
    "          [--lat DEG] [--lon DEG] [--airspace PATH] PATH\n"
    "\n"
    "Show a terrain zip as a lit height mesh.\n"
    "Hold WASD to move. Shift and Ctrl change height.\n"
    "The camera stays at least 50 m above the ground.\n"
    "Area shapes are filled.\n"
    "\n"
    "  -WxH             window size (default 640x480)\n"
    "  --range METRES   radius of the mesh (default 20000)\n"
    "  --pitch DEG      camera elevation above the horizon (default 45)\n"
    "  --ramp N         map terrain ramp index (default 0)\n"
    "  --lat DEG        start latitude (with --lon)\n"
    "  --lon DEG        start longitude (with --lat)\n"
    "  --airspace PATH  OpenAir file, drawn as class-coloured volumes\n"
    "  -h, --help       display this help and exit\n"
    "  --version        output version information and exit\n"
    "\n"
    "Drag to orbit.  Mouse wheel, Page Up and Page Down change distance.\n"
    "Arrow keys yaw and pitch.\n"
    "\n"
    "Report bugs to: <%s>\n"
    "%s home page: <%s>\n",
    canonical_name, PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

static void
ParseCommandLine(Args &args)
{
  while (const char *a = args.PeekNext()) {
    if (a[0] != '-')
      break;
    args.GetNext();
    if (StringIsEqual(a, "--help") || StringIsEqual(a, "-h")) {
      PrintHelp();
      std::exit(EXIT_SUCCESS);
    }
    if (StringIsEqual(a, "--version")) {
      PrintStandardVersion(canonical_name, XCSoar_Version);
      std::exit(EXIT_SUCCESS);
    }
    if (StringIsEqual(a, "--range"))
      range_m = args.ExpectNextDouble();
    else if (StringIsEqual(a, "--pitch"))
      pitch_deg = args.ExpectNextDouble();
    else if (StringIsEqual(a, "--ramp"))
      ramp_index = unsigned(args.ExpectNextInt());
    else if (StringIsEqual(a, "--lat")) {
      start_lat = args.ExpectNextDouble();
      have_lat = true;
    } else if (StringIsEqual(a, "--lon")) {
      start_lon = args.ExpectNextDouble();
      have_lon = true;
    } else if (StringIsEqual(a, "--airspace")) {
      airspace_path = args.ExpectNextPath();
    } else
      args.UsageError();
  }

  if (have_lat != have_lon ||
      range_m < 500 || pitch_deg < 5 || pitch_deg > 80 ||
      ramp_index >= TerrainRendererSettings::NUM_RAMPS ||
      (have_lat && (start_lat < -90 || start_lat > 90 ||
                    start_lon < -180 || start_lon > 180)))
    args.UsageError();

  terrain_path = args.ExpectNextPath();
}

/** East/north metres from the fixed mesh origin. Same frame as the lattice. */
static void
OriginEN(const GeoPoint &geo, double &east, double &north) noexcept
{
  east = 0;
  north = 0;
  if (!mesh_origin_set || !geo.IsValid())
    return;

  const GeoVector step(mesh_origin, geo);
  if (!step.IsValid() || step.distance <= 1)
    return;

  const double bearing = step.bearing.Radians();
  east = step.distance * std::sin(bearing);
  north = step.distance * std::cos(bearing);
}

static void
ViewOffset(const GeoPoint &center, double &east, double &north) noexcept
{
  OriginEN(center, east, north);
}

/** Slide the window over the lattice. Copied cells keep their shade. */
static void
ShiftLattice(int di, int dj) noexcept
{
  if (di == 0 && dj == 0)
    return;

  static Cell tmp_cell[MAX_N][MAX_N];
  static Color tmp_color[MAX_N][MAX_N];
  static bool tmp_ok[MAX_N][MAX_N];
  static bool tmp_lit[MAX_N][MAX_N];
  static int tmp_li[MAX_N][MAX_N];
  static int tmp_lj[MAX_N][MAX_N];

  for (unsigned iz = 0; iz < grid_n; ++iz) {
    for (unsigned ix = 0; ix < grid_n; ++ix) {
      const int ox = int(ix) + di;
      const int oz = int(iz) + dj;
      const bool keep = ox >= 0 && oz >= 0 &&
                        ox < int(grid_n) && oz < int(grid_n);
      if (keep)
        tmp_cell[iz][ix] = cells[oz][ox];
      else {
        tmp_cell[iz][ix].sampled = false;
        tmp_cell[iz][ix].valid = false;
        tmp_cell[iz][ix].ok = false;
      }

      if (iz + 1 >= grid_n || ix + 1 >= grid_n)
        continue;
      const bool keep_face = keep &&
                             ox + 1 < int(grid_n) &&
                             oz + 1 < int(grid_n);
      if (keep_face) {
        tmp_color[iz][ix] = face_color[oz][ox];
        tmp_ok[iz][ix] = face_ok[oz][ox];
        tmp_lit[iz][ix] = face_lit[oz][ox];
        tmp_li[iz][ix] = face_li[oz][ox];
        tmp_lj[iz][ix] = face_lj[oz][ox];
      } else {
        tmp_lit[iz][ix] = false;
        tmp_ok[iz][ix] = false;
      }
    }
  }

  for (unsigned iz = 0; iz < grid_n; ++iz) {
    for (unsigned ix = 0; ix < grid_n; ++ix) {
      cells[iz][ix] = tmp_cell[iz][ix];
      if (iz + 1 >= grid_n || ix + 1 >= grid_n)
        continue;
      face_color[iz][ix] = tmp_color[iz][ix];
      face_ok[iz][ix] = tmp_ok[iz][ix];
      face_lit[iz][ix] = tmp_lit[iz][ix];
      face_li[iz][ix] = tmp_li[iz][ix];
      face_lj[iz][ix] = tmp_lj[iz][ix];
    }
  }
}

static void
FillGrid(const RasterMap &map, const GeoPoint &center) noexcept
{
  if (grid_n < 2 || !mesh_origin_set)
    return;

  /* A tile batch changed the heights. Samples from the overview
     must not stick. */
  if (terrain_serial != map.GetSerial()) {
    terrain_serial = map.GetSerial();
    for (unsigned iz = 0; iz < grid_n; ++iz) {
      for (unsigned ix = 0; ix < grid_n; ++ix) {
        cells[iz][ix].sampled = false;
        if (iz + 1 < grid_n && ix + 1 < grid_n)
          face_lit[iz][ix] = false;
      }
    }
  }

  double view_e, view_n;
  ViewOffset(center, view_e, view_n);
  const int i0 = int(std::floor(view_e / lattice_m)) - int(grid_n) / 2;
  const int j0 = int(std::floor(view_n / lattice_m)) - int(grid_n) / 2;
  if (anchor_set)
    ShiftLattice(i0 - anchor_i, j0 - anchor_j);
  anchor_i = i0;
  anchor_j = j0;
  anchor_e = view_e;
  anchor_n = view_n;
  anchor_set = true;

  const double limit2 = range_m * range_m;
  for (unsigned iz = 0; iz < grid_n; ++iz) {
    for (unsigned ix = 0; ix < grid_n; ++ix) {
      const int li = i0 + int(ix);
      const int lj = j0 + int(iz);
      const double world_e = li * lattice_m;
      const double world_n = lj * lattice_m;
      Cell &cell = cells[iz][ix];
      cell.e = world_e - view_e;
      cell.n = world_n - view_n;
      const bool inside = cell.e * cell.e + cell.n * cell.n <= limit2;
      const bool same = cell.sampled && cell.li == li && cell.lj == lj;
      if (!same) {
        cell.li = li;
        cell.lj = lj;
        cell.sampled = false;
        cell.valid = false;
        cell.ok = false;
        cell.water = false;
        cell.z = 0;
        if (!inside)
          continue;

        GeoPoint pt = mesh_origin;
        const double distance = std::hypot(world_e, world_n);
        if (distance > 1)
          pt = GeoVector(distance,
                         Angle::Radians(std::atan2(world_e, world_n)))
                 .EndPoint(mesh_origin);

        cell.sampled = true;
        const auto hgt = map.GetInterpolatedHeight(pt);
        if (hgt.IsInvalid())
          continue;
        cell.valid = true;
        cell.water = hgt.IsWater();
        cell.z = hgt.GetValueOr0();
        cell.ok = true;
      } else {
        cell.ok = inside && cell.valid;
      }
    }
  }

  const auto origin = map.GetHeight(center);
  target_z = origin.IsInvalid() || origin.IsWater()
    ? 0 : origin.GetValue();

  for (unsigned iz = 0; iz + 1 < grid_n; ++iz) {
    for (unsigned ix = 0; ix + 1 < grid_n; ++ix) {
      const int li = i0 + int(ix);
      const int lj = j0 + int(iz);
      const Cell &a = cells[iz][ix];
      const Cell &b = cells[iz][ix + 1];
      const Cell &c = cells[iz + 1][ix + 1];
      const Cell &d = cells[iz + 1][ix];
      const bool corners = a.ok && b.ok && c.ok && d.ok;
      if (face_lit[iz][ix] &&
          face_li[iz][ix] == li && face_lj[iz][ix] == lj) {
        face_ok[iz][ix] = corners;
        continue;
      }

      face_lit[iz][ix] = false;
      face_ok[iz][ix] = false;
      if (!corners)
        continue;

      const Vec3 east{b.e - a.e, b.n - a.n, b.z - a.z};
      const Vec3 north{d.e - a.e, d.n - a.n, d.z - a.z};
      Vec3 normal = Normalise(Cross(east, north));
      if (normal.z < 0) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
      }
      const double h = (a.z + b.z + c.z + d.z) / 4;
      const bool water = a.water && b.water && c.water && d.water;
      face_color[iz][ix] = Lit(LookupTerrainColor(ramp_index, int(h), water),
                               Dot(normal, light));
      face_ok[iz][ix] = true;
      face_lit[iz][ix] = true;
      face_li[iz][ix] = li;
      face_lj[iz][ix] = lj;
    }
  }
}

static bool
InRange(float e, float n) noexcept
{
  return double(e) * e + double(n) * n <= range_m * range_m;
}

/** East/north in the mesh view frame (origin minus the current view). */
static void
ViewEN(const GeoPoint &center, const GeoPoint &geo,
       float &e, float &n) noexcept
{
  double ge, gn, ce, cn;
  OriginEN(geo, ge, gn);
  OriginEN(center, ce, cn);
  e = float(ge - ce);
  n = float(gn - cn);
}

/** East/north and ground height. False when the point has no terrain. */
static bool
Place(const RasterMap &map, const GeoPoint &center, const GeoPoint &geo,
      float &e, float &n, float &z) noexcept
{
  e = 0;
  n = 0;
  z = 0;
  ViewEN(center, geo, e, n);
  const auto hgt = map.GetHeight(geo);
  if (hgt.IsInvalid())
    return false;
  if (!hgt.IsWater())
    z = float(hgt.GetValue());
  return true;
}

/** Intersection of an inside→outside segment with the terrain disk. */
static bool
ClipDisk(float e0, float n0, float z0,
         float e1, float n1, float z1,
         float &e, float &n, float &z) noexcept
{
  const double dx = double(e1) - e0, dy = double(n1) - n0;
  const double A = dx * dx + dy * dy;
  if (A < 1e-6)
    return false;
  const double B = 2 * (double(e0) * dx + double(n0) * dy);
  const double C = double(e0) * e0 + double(n0) * n0 - range_m * range_m;
  const double disc = B * B - 4 * A * C;
  if (disc < 0)
    return false;
  const double root = std::sqrt(disc);
  double t = (-B - root) / (2 * A);
  if (t < 0 || t > 1)
    t = (-B + root) / (2 * A);
  if (t < 0 || t > 1)
    return false;
  e = float(double(e0) + dx * t);
  n = float(double(n0) + dy * t);
  z = float(double(z0) + (double(z1) - z0) * t);
  return true;
}

static constexpr unsigned MAX_TOPO = 400000;

struct V2 {
  float e, n, z;
};

struct Deg {
  double lon, lat;
};

static unsigned
ClipDeg(const Deg *in, unsigned n, Deg *out,
        double edge, unsigned axis, bool keep_low) noexcept
{
  if (n == 0)
    return 0;

  auto coord = [&](const Deg &p) {
    return axis == 0 ? p.lon : p.lat;
  };
  auto inside = [&](const Deg &p) {
    return keep_low ? coord(p) <= edge : coord(p) >= edge;
  };

  unsigned m = 0;
  Deg prev = in[n - 1];
  bool prev_in = inside(prev);
  for (unsigned i = 0; i < n; ++i) {
    const Deg cur = in[i];
    const bool cur_in = inside(cur);
    if (cur_in != prev_in && m < 46) {
      const double va = coord(prev), vb = coord(cur);
      double t = 0;
      if (vb != va)
        t = (edge - va) / (vb - va);
      Deg cut{prev.lon + (cur.lon - prev.lon) * t,
              prev.lat + (cur.lat - prev.lat) * t};
      if (axis == 0)
        cut.lon = edge;
      else
        cut.lat = edge;
      out[m++] = cut;
    }
    if (cur_in && m < 46)
      out[m++] = cur;
    prev = cur;
    prev_in = cur_in;
  }
  return m;
}

static V2
LerpV(const V2 &a, const V2 &b, double t) noexcept
{
  return {
    float(a.e + (double(b.e) - a.e) * t),
    float(a.n + (double(b.n) - a.n) * t),
    float(a.z + (double(b.z) - a.z) * t),
  };
}

static void
PushV(V2 *out, unsigned &m, const V2 &v) noexcept
{
  if (m >= 16)
    return;
  if (m > 0) {
    const double de = double(v.e) - out[m - 1].e;
    const double dn = double(v.n) - out[m - 1].n;
    if (de * de + dn * dn < 1)
      return;
  }
  out[m++] = v;
}

/** One pass around the polygon. Points outside the disk are dropped. */
static unsigned
ClipPolyDisk(const V2 *in, unsigned n, V2 *out) noexcept
{
  unsigned m = 0;
  const double r2 = range_m * range_m;
  for (unsigned i = 0; i < n; ++i) {
    const V2 &a = in[i];
    const V2 &b = in[(i + 1) % n];
    const bool inb = InRange(b.e, b.n);
    const double dx = double(b.e) - a.e, dy = double(b.n) - a.n;
    const double A = dx * dx + dy * dy;
    double t0 = 0, t1 = 0;
    int nroot = 0;
    if (A > 1e-8) {
      const double B = 2 * (double(a.e) * dx + double(a.n) * dy);
      const double C = double(a.e) * a.e + double(a.n) * a.n - r2;
      const double disc = B * B - 4 * A * C;
      if (disc >= 0) {
        const double s = std::sqrt(disc);
        t0 = (-B - s) / (2 * A);
        t1 = (-B + s) / (2 * A);
        if (t0 > t1)
          std::swap(t0, t1);
        nroot = 2;
      }
    }
    auto keep = [](double t) { return t > 0 && t < 1; };
    if (nroot == 2 && keep(t0))
      PushV(out, m, LerpV(a, b, t0));
    if (nroot == 2 && keep(t1))
      PushV(out, m, LerpV(a, b, t1));
    if (inb)
      PushV(out, m, b);
  }
  return m;
}

static double
Cross(const V2 &a, const V2 &b, float e, float n) noexcept
{
  return (double(b.e) - a.e) * (double(n) - a.n) -
         (double(b.n) - a.n) * (double(e) - a.e);
}

static float
PlaneZ(const V2 &a, const V2 &b, const V2 &c,
       float e, float n) noexcept
{
  const double abx = double(b.e) - a.e, aby = double(b.n) - a.n;
  const double acx = double(c.e) - a.e, acy = double(c.n) - a.n;
  const double den = abx * acy - aby * acx;
  if (std::abs(den) < 1e-6)
    return a.z;
  const double apx = double(e) - a.e, apy = double(n) - a.n;
  const double v = (apx * acy - apy * acx) / den;
  const double w = (abx * apy - aby * apx) / den;
  return float((1 - v - w) * a.z + v * b.z + w * c.z);
}

/**
 * Circle samples kept inside the polygon. Used when the disk sits in
 * a shape whose own vertices are all outside it.
 */
static unsigned
ClipCircle(const V2 *in, unsigned n, V2 *out) noexcept
{
  constexpr unsigned N = 12;
  V2 gon[2][16];
  for (unsigned k = 0; k < N; ++k) {
    const double ang = k * (2 * 3.141592653589793 / N);
    const float e = float(range_m * std::cos(ang));
    const float nn = float(range_m * std::sin(ang));
    gon[0][k] = {e, nn, PlaneZ(in[0], in[1], in[2], e, nn)};
  }

  unsigned src = 0, m = N;
  for (unsigned e = 0; e < n; ++e) {
    const V2 &p = in[e];
    const V2 &q = in[(e + 1) % n];
    const V2 &r = in[(e + 2) % n];
    const double side = Cross(p, q, r.e, r.n);
    const unsigned dst = src ^ 1;
    unsigned w = 0;
    const V2 *poly = gon[src];
    auto inside = [&](const V2 &v) {
      return Cross(p, q, v.e, v.n) * side >= 0;
    };
    for (unsigned i = 0; i < m; ++i) {
      const V2 &a = poly[i];
      const V2 &b = poly[(i + 1) % m];
      const bool ina = inside(a), inb = inside(b);
      if (ina != inb && w < 16) {
        const double da = Cross(p, q, a.e, a.n);
        const double db = Cross(p, q, b.e, b.n);
        double t = 0;
        if (da != db)
          t = da / (da - db);
        gon[dst][w++] = LerpV(a, b, t);
      }
      if (inb && w < 16)
        gon[dst][w++] = b;
    }
    src = dst;
    m = w;
    if (m < 3)
      return 0;
  }
  for (unsigned i = 0; i < m; ++i)
    out[i] = gon[src][i];
  return m;
}

static unsigned
ClipToDisk(const V2 *in, unsigned n, V2 *out) noexcept
{
  for (unsigned i = 0; i < n; ++i)
    if (InRange(in[i].e, in[i].n))
      return ClipPolyDisk(in, n, out);
  if (n < 3)
    return 0;
  return ClipCircle(in, n, out);
}

static void
EmitFan(const V2 *p, unsigned n,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
  for (unsigned i = 1; i + 1 < n; ++i) {
    if (topo_fill.size() + 3 > MAX_TOPO)
      return;
    const V2 vs[3] = {p[0], p[i], p[i + 1]};
    for (const V2 &v : vs)
      topo_fill.push_back({v.e, v.n, v.z, r, g, b, a, false});
  }
}

/** Shape triangle, clipped to the terrain file and the drawn disk. */
static void
PushTri(const RasterMap &map, const GeoPoint &center,
        const GeoPoint &g0, const GeoPoint &g1, const GeoPoint &g2,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
  if (topo_fill.size() + 3 > MAX_TOPO)
    return;

  const GeoPoint src[3] = {g0, g1, g2};
  V2 vert[3];
  bool need_bounds = false, need_disk = false;
  for (unsigned k = 0; k < 3; ++k) {
    float e, n, z;
    if (!Place(map, center, src[k], e, n, z))
      need_bounds = true;
    if (!InRange(e, n))
      need_disk = true;
    vert[k] = {e, n, z};
  }
  if (!need_bounds && !need_disk) {
    EmitFan(vert, 3, r, g, b, a);
    return;
  }

  const GeoBounds &bounds = map.GetBounds();
  /* shortcut: file crosses the dateline, drop straddling triangles */
  if (need_bounds &&
      bounds.GetEast().Degrees() < bounds.GetWest().Degrees())
    return;

  V2 poly[48];
  unsigned n = 0;
  if (!need_bounds) {
    n = 3;
    for (unsigned k = 0; k < 3; ++k)
      poly[k] = vert[k];
  } else {
    Deg deg[2][48];
    unsigned nd = 3;
    for (unsigned k = 0; k < 3; ++k) {
      deg[0][k].lon = src[k].longitude.Degrees();
      deg[0][k].lat = src[k].latitude.Degrees();
    }
    const double west = bounds.GetWest().Degrees();
    const double east = bounds.GetEast().Degrees();
    const double south = bounds.GetSouth().Degrees();
    const double north = bounds.GetNorth().Degrees();
    const double pad = 1e-6;
    nd = ClipDeg(deg[0], nd, deg[1], west + pad, 0, false);
    nd = ClipDeg(deg[1], nd, deg[0], east - pad, 0, true);
    nd = ClipDeg(deg[0], nd, deg[1], south + pad, 1, false);
    nd = ClipDeg(deg[1], nd, deg[0], north - pad, 1, true);
    for (unsigned k = 0; k < nd && n < 48; ++k) {
      const GeoPoint pt(Angle::Degrees(deg[0][k].lon),
                        Angle::Degrees(deg[0][k].lat));
      float e, nloc, z;
      if (!Place(map, center, pt, e, nloc, z)) {
        n = 0;
        break;
      }
      if (!InRange(e, nloc))
        need_disk = true;
      poly[n++] = {e, nloc, z};
    }
  }
  if (n < 3)
    return;

  if (need_disk) {
    V2 clipped[16];
    const unsigned m = ClipToDisk(poly, n, clipped);
    EmitFan(clipped, m, r, g, b, a);
    return;
  }

  EmitFan(poly, n, r, g, b, a);
}

/** OpenGL map padding brush. A solid class fill uses 48. */
static constexpr uint8_t AIRSPACE_PAD_ALPHA = 90;
static constexpr uint8_t AIRSPACE_FILL_ALPHA = 48;

/**
 * Disk clip for an airspace ring. ClipPolyDisk stops at 16 vertices,
 * which is enough for a terrain triangle and not for a CTR.
 */
static void
ClipAirPoly(const std::vector<V2> &in, std::vector<V2> &out) noexcept
{
  out.clear();
  const unsigned n = unsigned(in.size());
  if (n < 3)
    return;

  bool any = false;
  for (const V2 &p : in)
    if (InRange(p.e, p.n)) {
      any = true;
      break;
    }

  if (!any) {
    /* shortcut: a shape that covers the view is a 12-gon of the disk */
    V2 tmp[16];
    const unsigned m = ClipCircle(in.data(), n, tmp);
    out.assign(tmp, tmp + m);
    return;
  }

  const double r2 = range_m * range_m;
  for (unsigned i = 0; i < n; ++i) {
    const V2 &a = in[i];
    const V2 &b = in[(i + 1) % n];
    const bool inb = InRange(b.e, b.n);
    const double dx = double(b.e) - a.e;
    const double dy = double(b.n) - a.n;
    const double A = dx * dx + dy * dy;
    double t0 = 0, t1 = 0;
    int nroot = 0;
    if (A > 1e-8) {
      const double B = 2 * (double(a.e) * dx + double(a.n) * dy);
      const double C = double(a.e) * a.e + double(a.n) * a.n - r2;
      const double disc = B * B - 4 * A * C;
      if (disc >= 0) {
        const double s = std::sqrt(disc);
        t0 = (-B - s) / (2 * A);
        t1 = (-B + s) / (2 * A);
        if (t0 > t1)
          std::swap(t0, t1);
        nroot = 2;
      }
    }
    auto push = [&](const V2 &v) {
      if (!out.empty()) {
        const double de = double(v.e) - out.back().e;
        const double dn = double(v.n) - out.back().n;
        if (de * de + dn * dn < 1)
          return;
      }
      out.push_back(v);
    };
    auto keep = [](double t) { return t > 0 && t < 1; };
    if (nroot == 2 && keep(t0))
      push(LerpV(a, b, t0));
    if (nroot == 2 && keep(t1))
      push(LerpV(a, b, t1));
    if (inb)
      push(b);
  }
}

static void
PushWall(float e0, float n0, float e1, float n1,
         float z0, float z1,
         uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
  if (topo_fill.size() + 6 > MAX_TOPO)
    return;

  const float es[6] = {e0, e1, e1, e0, e1, e0};
  const float ns[6] = {n0, n1, n1, n0, n1, n0};
  const float zs[6] = {z0, z0, z1, z0, z1, z1};
  for (unsigned k = 0; k < 6; ++k)
    topo_fill.push_back({es[k], ns[k], zs[k], r, g, b, a, false});
}

/** Extruded class-coloured volumes, clipped to the drawn disk. */
static void
FillAirspace(const RasterMap &map, const GeoPoint &center) noexcept
{
  if (!have_airspace)
    return;

  using GFill = AirspaceRendererSettings::FillMode;
  using CFill = AirspaceClassRendererSettings::FillMode;

  std::vector<V2> poly, clipped;
  for (const auto &i : airspaces.QueryWithinRange(center, range_m)) {
    AbstractAirspace &as = i.GetAirspace();
    AirspaceClass cls = as.GetTypeOrClass();
    if (!airspace_look.classes[cls].display)
      cls = as.GetClass();
    const auto &look = airspace_look.classes[cls];
    if (!look.display)
      continue;

    if (as.NeedGroundLevel()) {
      const auto hgt =
        map.GetInterpolatedHeight(as.GetReferenceLocation());
      as.SetGroundLevel(hgt.IsInvalid() ? 0 : hgt.GetValueOr0());
    }

    const float z0 = float(as.GetBase().altitude);
    const float z1 = float(as.GetTop().altitude);
    if (!(z1 > z0))
      continue;

    const SearchPointVector &pts = as.GetPoints();
    if (pts.size() < 3)
      continue;

    poly.clear();
    poly.reserve(pts.size());
    for (const SearchPoint &sp : pts) {
      float e, n;
      ViewEN(center, sp.GetLocation(), e, n);
      poly.push_back({e, n, 0});
    }
    if (poly.size() >= 2) {
      const double de = double(poly.front().e) - poly.back().e;
      const double dn = double(poly.front().n) - poly.back().n;
      if (de * de + dn * dn < 1)
        poly.pop_back();
    }

    ClipAirPoly(poly, clipped);
    if (clipped.size() < 3)
      continue;

    const bool solid = airspace_look.fill_mode == GFill::ALL ||
                       look.fill_mode == CFill::ALL;
    const bool pad = !solid &&
                     airspace_look.fill_mode != GFill::NONE &&
                     look.fill_mode != CFill::NONE;
    const uint8_t alpha = solid ? AIRSPACE_FILL_ALPHA :
                          pad ? AIRSPACE_PAD_ALPHA : 0;
    const RGB8Color fill = look.fill_color;
    const unsigned nv = unsigned(clipped.size());
    if (alpha != 0) {
      for (unsigned k = 0; k < nv; ++k) {
        const V2 &a = clipped[k];
        const V2 &b = clipped[(k + 1) % nv];
        PushWall(a.e, a.n, b.e, b.n, z0, z1,
                 fill.Red(), fill.Green(), fill.Blue(), alpha);
      }
      for (V2 &p : clipped)
        p.z = z1;
      EmitFan(clipped.data(), nv,
              fill.Red(), fill.Green(), fill.Blue(), alpha);
      if (!as.GetBase().IsTerrain()) {
        for (V2 &p : clipped)
          p.z = z0;
        EmitFan(clipped.data(), nv,
                fill.Red(), fill.Green(), fill.Blue(), alpha);
      }
    }

    if (look.border_width == 0 ||
        topo_nodes.size() + nv + 1 > MAX_TOPO)
      continue;

    const RGB8Color border = airspace_look.black_outline
      ? RGB8_BLACK : look.border_color;
    bool start = true;
    for (const V2 &p : clipped) {
      topo_nodes.push_back({p.e, p.n, z1, border.Red(), border.Green(),
                            border.Blue(), 255, start});
      start = false;
    }
    const V2 &first = clipped.front();
    topo_nodes.push_back({first.e, first.n, z1, border.Red(),
                          border.Green(), border.Blue(), 255, false});
  }
}

static void
FillTopology(RasterMap &map, const GeoPoint &center)
{
  topo_nodes.clear();
  topo_fill.clear();
  if (topo_store == nullptr) {
    FillAirspace(map, center);
    return;
  }

  WindowProjection projection;
  projection.SetScreenSize({640, 640});
  projection.SetScaleFromRadius(range_m);
  projection.SetGeoLocation(center);
  projection.SetScreenOrigin(320, 320);
  projection.UpdateScreenBounds();
  topo_store->ScanVisibility(projection);

  for (const TopographyFile &file : *topo_store) {
    const auto color = file.GetColor();
    const uint8_t cr = color.Red();
    const uint8_t cg = color.Green();
    const uint8_t cb = color.Blue();
    const uint8_t ca = color.Alpha();
    const std::lock_guard lock{file.mutex};
    for (const XShape &shape : file) {
      const auto lines = shape.GetLines();
      const auto *pts = shape.GetPoints();
      if (pts == nullptr)
        continue;

      if (shape.get_type() == MS_SHAPE_POLYGON) {
#ifdef ENABLE_OPENGL
        const auto idx = shape.GetIndices(0, ShapeScalar{0});
        if (idx.indices == nullptr || idx.count == nullptr)
          continue;
        unsigned npts = 0;
        for (uint16_t count : lines)
          npts += count;
        const unsigned n = *idx.count;
        for (unsigned i = 2; i < n; ++i) {
          const unsigned a = idx.indices[i - 2];
          const unsigned b = idx.indices[i - 1];
          const unsigned c = idx.indices[i];
          if (a == b || b == c || a == c)
            continue;
          if (a >= npts || b >= npts || c >= npts)
            continue;
          PushTri(map, center,
                  file.ToGeoPoint(pts[a]),
                  file.ToGeoPoint(pts[b]),
                  file.ToGeoPoint(pts[c]),
                  cr, cg, cb, ca);
        }
#else
        unsigned base = 0;
        for (uint16_t count : lines) {
          std::vector<GeoPoint> ring;
          ring.reserve(count);
          for (unsigned i = 0; i < count; ++i)
            ring.push_back(pts[base + i]);
          if (ring.size() >= 2 &&
              ring.front().longitude == ring.back().longitude &&
              ring.front().latitude == ring.back().latitude)
            ring.pop_back();
          for (unsigned i = 1; i + 1 < ring.size(); ++i)
            PushTri(map, center, ring[0], ring[i], ring[i + 1],
                    cr, cg, cb, ca);
          base += count;
        }
#endif
        continue;
      }

      if (shape.get_type() != MS_SHAPE_LINE)
        continue;

      unsigned base = 0;
      for (uint16_t count : lines) {
        bool start = true;
        for (unsigned i = 0; i < count; ++i) {
          if (topo_nodes.size() >= MAX_TOPO) {
            FillAirspace(map, center);
            return;
          }
#ifdef ENABLE_OPENGL
          const GeoPoint geo = file.ToGeoPoint(pts[base + i]);
#else
          const GeoPoint geo = pts[base + i];
#endif
          float e, n, z;
          if (!Place(map, center, geo, e, n, z)) {
            start = true;
            continue;
          }
          if (!InRange(e, n)) {
            if (!start && !topo_nodes.empty()) {
              const TopoNode &prev = topo_nodes.back();
              float ce, cn, cz;
              if (InRange(prev.e, prev.n) &&
                  ClipDisk(prev.e, prev.n, prev.z, e, n, z, ce, cn, cz))
                topo_nodes.push_back({ce, cn, cz, cr, cg, cb, ca, false});
            }
            start = true;
            continue;
          }
          topo_nodes.push_back({e, n, z, cr, cg, cb, ca, start});
          start = false;
        }
        base += count;
      }
    }
  }

  FillAirspace(map, center);
}

/** One JPEG2000 batch. The loader caps how many tiles that is. */
static void
LoadTileBatch() noexcept
{
  if (terrain_zip == nullptr || terrain_map == nullptr ||
      !view_center.IsValid())
    return;

  UpdateTerrainTiles(terrain_zip->get(), terrain_map->GetTileCache(),
                     terrain_mutex, terrain_map->GetProjection(),
                     view_center, range_m);
  if (terrain_map->IsDirty())
    topo_resync = true;
}

static void
ShiftTopo(const GeoPoint &from, const GeoPoint &to) noexcept
{
  /* Mesh local position is origin east/north minus the view. A geodesic
     from the old view to the new one is another tangent plane, so the
     shapes drift off the ground they were draped on. */
  double fe, fn, te, tn;
  OriginEN(from, fe, fn);
  OriginEN(to, te, tn);
  const float de = float(te - fe);
  const float dn = float(tn - fn);
  if (de == 0 && dn == 0)
    return;

  for (TopoNode &node : topo_nodes) {
    node.e -= de;
    node.n -= dn;
  }
  for (TopoNode &node : topo_fill) {
    node.e -= de;
    node.n -= dn;
  }
}

static void
SyncTopo() noexcept
{
  if ((!have_topo && !have_airspace) || terrain_map == nullptr)
    return;

  const GeoVector step(topo_center, view_center);
  const double moved = topo_center_set && step.IsValid() ? step.distance : 1e9;
  /* shortcut: edges stay stale until the view moves a fifth of the range */
  if (!topo_center_set || moved > range_m * 0.2)
    FillTopology(*terrain_map, view_center);
  else
    ShiftTopo(topo_center, view_center);
  topo_center = view_center;
  topo_center_set = true;
}

static void
RefreshView()
{
  if (terrain_zip == nullptr || terrain_map == nullptr ||
      !view_center.IsValid() || grid_n < 2)
    return;

  LoadTileBatch();
  FillGrid(*terrain_map, view_center);
  SyncTopo();
}

static void
LoadMesh(ZipArchive &archive, RasterMap &map)
{
  {
    ConsoleOperationEnvironment operation;
    LoadTerrainOverview(archive.get(), map.GetTileCache(), operation);
  }
  map.UpdateProjection();

  GeoPoint center = map.GetMapCenter();
  if (have_lat)
    center = GeoPoint(Angle::Degrees(start_lon),
                      Angle::Degrees(start_lat));
  if (!center.IsValid() || !map.GetBounds().IsInside(center))
    throw std::runtime_error("Start position is outside the terrain");

  grid_n = unsigned(std::lround((2 * range_m) / CELL_M)) + 1;
  if (grid_n < 24)
    grid_n = 24;
  if (grid_n > MAX_N)
    grid_n = MAX_N;

  view_center = center;
  if (!mesh_origin_set) {
    mesh_origin = center;
    mesh_origin_set = true;
  }
  lattice_m = (2 * range_m) / (grid_n - 1);
  RefreshView();
}

static void
ReadAirspace(Reader &src, AirspaceOrigin origin)
{
  BufferedReader reader(src);
  ParseAirspaceFile(airspaces, reader, origin);
}

static void
LoadAirspace(ZipArchive &archive)
{
  airspace_look.SetDefaults();

  if (archive.Exists("airspace.txt")) {
    try {
      ZipReader zip(archive.get(), "airspace.txt");
      ReadAirspace(zip, AirspaceOrigin::MAP);
    } catch (const std::runtime_error &e) {
      std::fprintf(stderr, "RunTerrainView: %s\n", e.what());
    }
  }

  if (airspace_path != nullptr) {
    try {
      FileReader file(airspace_path);
      ReadAirspace(file, AirspaceOrigin::FILE);
    } catch (const std::runtime_error &e) {
      std::fprintf(stderr, "RunTerrainView: %s\n", e.what());
    }
  }

  airspaces.Optimise();
  if (airspaces.IsEmpty())
    return;

  airspaces.SetFlightLevels(AtmosphericPressure::Standard());
  have_airspace = true;
  std::fprintf(stderr, "RunTerrainView: %u airspaces\n",
               airspaces.GetSize());
}

static void
LoadTopology(ZipArchive &archive, RasterMap &map,
             TopographyStore &store, const GeoPoint &center)
{
  try {
    ZipLineReaderA reader(archive.get(), "topology.tpl");
    store.Load(reader, nullptr, archive.get());
    topo_store = &store;
    have_topo = true;
  } catch (const std::runtime_error &e) {
    std::fprintf(stderr, "RunTerrainView: no topology (%s)\n", e.what());
  }

  if (!have_topo && !have_airspace)
    return;

  topo_nodes.reserve(65536);
  topo_fill.reserve(65536);
  FillTopology(map, center);
  topo_center = center;
  topo_center_set = true;
  topo_resync = terrain_map != nullptr && terrain_map->IsDirty();
  std::fprintf(stderr, "RunTerrainView: %u topology lines, %u area vertices\n",
               unsigned(topo_nodes.size()), unsigned(topo_fill.size()));
}

/** Height of the mesh at a local east/north point. */
static double
GroundHeight(double east, double north) noexcept
{
  if (!anchor_set || grid_n < 2 || lattice_m <= 0)
    return target_z;

  const double fx = (east + anchor_e) / lattice_m - anchor_i;
  const double fz = (north + anchor_n) / lattice_m - anchor_j;
  if (fx < 0 || fz < 0 || fx >= grid_n - 1 || fz >= grid_n - 1)
    return target_z;

  const unsigned ix = unsigned(fx);
  const unsigned iz = unsigned(fz);
  const Cell &a = cells[iz][ix];
  const Cell &b = cells[iz][ix + 1];
  const Cell &c = cells[iz + 1][ix + 1];
  const Cell &d = cells[iz + 1][ix];
  if (!a.ok || !b.ok || !c.ok || !d.ok)
    return target_z;

  const double tx = fx - ix;
  const double tz = fz - iz;
  const double z0 = a.z + (b.z - a.z) * tx;
  const double z1 = d.z + (c.z - d.z) * tx;
  return z0 + (z1 - z0) * tz;
}

static GeoPoint
OffsetGeo(double e, double n) noexcept
{
  if (!view_center.IsValid())
    return view_center;
  const double d = std::hypot(e, n);
  if (d <= 1)
    return view_center;
  return GeoVector(d, Angle::Radians(std::atan2(e, n)))
           .EndPoint(view_center);
}

static bool
OnTerrain(const GeoPoint &pt) noexcept
{
  if (terrain_map == nullptr || !pt.IsValid())
    return false;
  if (!terrain_map->GetBounds().IsInside(pt))
    return false;
  return !terrain_map->GetHeight(pt).IsInvalid();
}

/** Pull an eye offset back until it sits on the drawn terrain. */
static void
ShortenToTerrain(double &e, double &n) noexcept
{
  const double lim = range_m * 0.98;
  const double h = std::hypot(e, n);
  if (h > lim && h > 1e-6) {
    const double s = lim / h;
    e *= s;
    n *= s;
  }

  if (std::hypot(e, n) < 1 || OnTerrain(OffsetGeo(e, n)))
    return;

  const double e0 = e, n0 = n;
  double lo = 0, hi = 1;
  for (int i = 0; i < 14; ++i) {
    const double t = (lo + hi) * 0.5;
    if (OnTerrain(OffsetGeo(e0 * t, n0 * t)))
      lo = t;
    else
      hi = t;
  }
  e = e0 * lo;
  n = n0 * lo;
}

static void
OrbitOffset(double yaw_deg, double pitch_deg, double distance,
            double &e, double &n) noexcept
{
  const double yaw = Angle::Degrees(yaw_deg).Radians();
  const double pitch = Angle::Degrees(pitch_deg).Radians();
  const double horiz = distance * std::cos(pitch);
  e = -horiz * std::sin(yaw);
  n = -horiz * std::cos(yaw);
}

struct Camera {
  double yaw_deg = 20;
  double pitch_deg = 45;
  double distance = 0;
  /** Metres above MIN_AGL. Zero sits on the terrain. */
  double extra_agl = 0;

  Vec3 Eye() const noexcept {
    double e, n;
    OrbitOffset(yaw_deg, pitch_deg, distance, e, n);
    ShortenToTerrain(e, n);
    return {e, n, GroundHeight(e, n) + MIN_AGL + extra_agl};
  }
};

class TerrainWindow : public PaintWindow {
  Camera camera;
  bool dragging = false;
  PixelPoint last{};
  bool move_fwd = false, move_back = false;
  bool move_left = false, move_right = false;
  bool move_up = false, move_down = false;
  UI::Timer move_timer{[this]() { StepMove(); }};

#ifdef ENABLE_OPENGL
  /* The window itself has no depth image.  This target does. */
  std::unique_ptr<GLTexture> color_tex;
  std::unique_ptr<GLRenderBuffer> depth_rb;
  std::unique_ptr<GLFrameBuffer> depth_fbo;
  PixelSize depth_size{0, 0};
  bool depth_failed = false;

  bool PrepareDepth(PixelSize size) noexcept;
  bool DrawDepth(Canvas &canvas, const Face *faces,
                 unsigned n_faces,
                 const TerrainVertex *topo, unsigned n_topo,
                 const ScreenTri *fill, unsigned n_fill) noexcept;

  /* The GL surface can be larger than this child.  Paint that. */
  PixelSize SurfaceSize(const Canvas &canvas) const noexcept {
    if (OpenGL::viewport_size.x > 0 && OpenGL::viewport_size.y > 0)
      return PixelSize(OpenGL::viewport_size);
    return canvas.GetSize();
  }
#endif

public:
  TerrainWindow() {
    camera.pitch_deg = pitch_deg;
    /* Start inside the patch. FitDistance keeps the eye on terrain. */
    const double pitch = Angle::Degrees(pitch_deg).Radians();
    camera.distance = range_m * 0.85 / std::max(std::cos(pitch), 0.02);
    camera.distance = std::clamp(camera.distance,
                                 range_m * 0.2, range_m * 8);
    FitDistance();
    const double orbit_z =
      target_z + camera.distance * std::sin(pitch);
    double e, n;
    OrbitOffset(camera.yaw_deg, camera.pitch_deg, camera.distance,
                e, n);
    ShortenToTerrain(e, n);
    camera.extra_agl =
      std::max(0.0, orbit_z - GroundHeight(e, n) - MIN_AGL);
  }

  void FitDistance() noexcept {
    const double pitch = Angle::Degrees(camera.pitch_deg).Radians();
    const double c = std::max(std::cos(pitch), 0.02);
    double e, n;
    OrbitOffset(camera.yaw_deg, camera.pitch_deg, camera.distance, e, n);
    const double before = std::hypot(e, n);
    ShortenToTerrain(e, n);
    const double after = std::hypot(e, n);
    if (after + 0.5 < before)
      camera.distance = after / c;
  }

protected:
  void Orbit(double dyaw, double dpitch) noexcept {
    camera.yaw_deg += dyaw;
    camera.pitch_deg = std::clamp(camera.pitch_deg + dpitch, 8.0, 80.0);
    FitDistance();
    Invalidate();
  }

  void Zoom(double factor) noexcept {
    camera.distance = std::clamp(camera.distance * factor,
                                 range_m * 0.2, range_m * 8);
    FitDistance();
    Invalidate();
  }

  bool AnyMoveKey() const noexcept {
    return move_fwd || move_back || move_left || move_right ||
           move_up || move_down;
  }

  /** Look direction is +north at yaw 0. Right is east of that. */
  void StepMove() noexcept {
    const double step = std::max(80.0, range_m / 100.0);
    if (move_up)
      camera.extra_agl += step;
    if (move_down)
      camera.extra_agl = std::max(0.0, camera.extra_agl - step);

    const double yaw = Angle::Degrees(camera.yaw_deg).Radians();
    const double fe = std::sin(yaw);
    const double fn = std::cos(yaw);
    double e = 0, n = 0;
    if (move_fwd) {
      e += fe;
      n += fn;
    }
    if (move_back) {
      e -= fe;
      n -= fn;
    }
    if (move_right) {
      e += fn;
      n -= fe;
    }
    if (move_left) {
      e -= fn;
      n += fe;
    }
    if (std::hypot(e, n) >= 1e-6 && view_center.IsValid()) {
      const Angle bearing = Angle::Radians(std::atan2(e, n));
      double go = step;
      if (!OnTerrain(GeoVector(go, bearing).EndPoint(view_center))) {
        double lo = 0, hi = step;
        for (int i = 0; i < 8; ++i) {
          const double mid = (lo + hi) * 0.5;
          if (OnTerrain(GeoVector(mid, bearing).EndPoint(view_center)))
            lo = mid;
          else
            hi = mid;
        }
        go = lo;
      }
      if (go > 1) {
        view_center = GeoVector(go, bearing).EndPoint(view_center);
        RefreshView();
      }
      FitDistance();
    } else if (!move_up && !move_down) {
      move_timer.Cancel();
      return;
    }

    Invalidate();
    move_timer.Schedule(std::chrono::milliseconds(80));
  }

  bool SetMoveKey(unsigned key_code, bool down) noexcept {
    bool *flag = nullptr;
    switch (key_code) {
    case 'w':
    case 'W':
      flag = &move_fwd;
      break;
    case 's':
    case 'S':
      flag = &move_back;
      break;
    case 'a':
    case 'A':
      flag = &move_left;
      break;
    case 'd':
    case 'D':
      flag = &move_right;
      break;
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
      flag = &move_up;
      break;
    /* Right Ctrl is evdev 97, the same code as 'a'. */
    case KEY_LEFTCTRL:
      flag = &move_down;
      break;
    default:
      return false;
    }

    if (*flag == down)
      return true;
    *flag = down;
    if (down)
      StepMove();
    else if (!AnyMoveKey())
      move_timer.Cancel();
    return true;
  }

  bool OnMouseDown(PixelPoint p) noexcept override {
    dragging = true;
    last = p;
    SetFocus();
    return true;
  }

  bool OnMouseUp(PixelPoint) noexcept override {
    dragging = false;
    return true;
  }

  bool OnMouseMove(PixelPoint p, unsigned) noexcept override {
    if (!dragging)
      return false;
    Orbit((p.x - last.x) * 0.35, (p.y - last.y) * 0.25);
    last = p;
    return true;
  }

  bool OnMouseWheel(PixelPoint, int delta) noexcept override {
    Zoom(delta > 0 ? 0.9 : 1.1);
    return true;
  }

  bool OnKeyDown(unsigned key_code) noexcept override {
    if (SetMoveKey(key_code, true))
      return true;

    switch (key_code) {
    case KEY_LEFT:
      Orbit(-4, 0);
      return true;
    case KEY_RIGHT:
      Orbit(4, 0);
      return true;
    case KEY_UP:
      Orbit(0, 3);
      return true;
    case KEY_DOWN:
      Orbit(0, -3);
      return true;
    case KEY_PRIOR:
      Zoom(0.9);
      return true;
    case KEY_NEXT:
      Zoom(1.1);
      return true;
    default:
      return false;
    }
  }

  bool OnKeyUp(unsigned key_code) noexcept override {
    return SetMoveKey(key_code, false);
  }

  bool Project(Vec3 world, const Vec3 &eye, Vec3 right, Vec3 up, Vec3 fwd,
               double focal, PixelPoint center, PixelPoint &out,
               double &depth) const noexcept {
    const Vec3 d{world.x - eye.x, world.y - eye.y, world.z - eye.z};
    depth = Dot(d, fwd);
    if (depth < 20)
      return false;

    const double cx = Dot(d, right);
    const double cy = Dot(d, up);
    out.x = center.x + iround(cx * focal / depth);
    out.y = center.y - iround(cy * focal / depth);
    return out.x > -20000 && out.x < 20000 &&
           out.y > -20000 && out.y < 20000;
  }

  void OnPaint(Canvas &canvas) noexcept override {
    if (terrain_map != nullptr && view_center.IsValid() && grid_n >= 2) {
      if (terrain_map->IsDirty()) {
        LoadTileBatch();
        FillGrid(*terrain_map, view_center);
        Invalidate();
      } else if (topo_resync && (have_topo || have_airspace)) {
        FillTopology(*terrain_map, view_center);
        topo_center = view_center;
        topo_center_set = true;
        topo_resync = false;
        Invalidate();
      }
    }

    canvas.Clear(Color(140, 190, 230));

#ifdef ENABLE_OPENGL
    const PixelSize surface = SurfaceSize(canvas);
    if (GetSize() != surface)
      Resize(surface);
#else
    const PixelSize surface = canvas.GetSize();
#endif

    const Vec3 eye = camera.Eye();
    const Vec3 target{0, 0, target_z};
    const Vec3 fwd = Normalise({target.x - eye.x, target.y - eye.y,
                                target.z - eye.z});
    const Vec3 right = Normalise(Cross(fwd, {0, 0, 1}));
    const Vec3 up = Cross(right, fwd);

    const PixelRect rc(surface);
    const PixelPoint origin{int(rc.GetWidth() / 2),
                            int(rc.GetHeight() / 2)};
    const double half_fov = Angle::Degrees(FOV_DEG * 0.5).Radians();
    const double focal = (rc.GetHeight() * 0.5) / std::tan(half_fov);

    static Face faces[MAX_N * MAX_N];
    unsigned n_faces = 0;

    for (unsigned iz = 0; iz + 1 < grid_n; ++iz) {
      for (unsigned ix = 0; ix + 1 < grid_n; ++ix) {
        if (!face_ok[iz][ix])
          continue;

        const Cell corner[4] = {
          cells[iz][ix], cells[iz][ix + 1],
          cells[iz + 1][ix + 1], cells[iz + 1][ix],
        };
        Face face;
        face.color[0] = ShadeVertex(iz, ix);
        face.color[1] = ShadeVertex(iz, ix + 1);
        face.color[2] = ShadeVertex(iz + 1, ix + 1);
        face.color[3] = ShadeVertex(iz + 1, ix);
        face.depth = 0;
        bool visible = true;
        for (unsigned k = 0; k < 4; ++k) {
          PixelPoint sp;
          double depth = 0;
          const Vec3 world{corner[k].e, corner[k].n, corner[k].z};
          if (!Project(world, eye, right, up, fwd, focal, origin,
                       sp, depth)) {
            visible = false;
            break;
          }
          face.p[k] = sp;
          face.z[k] = ClipZ(depth);
          face.depth += depth;
        }
        if (!visible)
          continue;
        faces[n_faces++] = face;
      }
    }

    screen_fill.clear();
    for (std::size_t i = 0; i + 2 < topo_fill.size(); i += 3) {
      const TopoNode &a = topo_fill[i];
      const TopoNode &b = topo_fill[i + 1];
      const TopoNode &c = topo_fill[i + 2];
      const TopoNode *node[3] = {&a, &b, &c};
      ScreenTri tri;
      tri.color = Color(a.r, a.g, a.b, a.a);
      bool ok = true;
      for (unsigned k = 0; k < 3; ++k) {
        PixelPoint sp;
        double depth = 0;
        const Vec3 world{node[k]->e, node[k]->n, node[k]->z};
        if (!Project(world, eye, right, up, fwd, focal, origin,
                     sp, depth)) {
          ok = false;
          break;
        }
        tri.p[k] = sp;
        tri.z[k] = ClipZ(depth);
      }
      if (ok)
        screen_fill.push_back(tri);
    }

#ifdef ENABLE_OPENGL
    topo_draw.clear();
    bool pen_down = false;
    PixelPoint prev_px{};
    float prev_z = 0;
    uint8_t prev_r = 0, prev_g = 0, prev_b = 0, prev_a = 255;
    for (const TopoNode &node : topo_nodes) {
      PixelPoint sp;
      double depth = 0;
      const Vec3 world{node.e, node.n, node.z};
      if (!Project(world, eye, right, up, fwd, focal, origin,
                   sp, depth)) {
        pen_down = false;
        continue;
      }
      const float z = ClipZ(depth);
      if (node.start || !pen_down) {
        pen_down = true;
        prev_px = sp;
        prev_z = z;
        prev_r = node.r;
        prev_g = node.g;
        prev_b = node.b;
        prev_a = node.a;
        continue;
      }
      if (topo_draw.size() + 2 > topo_draw.capacity())
        break;
      topo_draw.push_back({float(prev_px.x), float(prev_px.y), prev_z,
                           prev_r, prev_g, prev_b, prev_a});
      topo_draw.push_back({float(sp.x), float(sp.y), z,
                           node.r, node.g, node.b, node.a});
      prev_px = sp;
      prev_z = z;
      prev_r = node.r;
      prev_g = node.g;
      prev_b = node.b;
      prev_a = node.a;
    }

    if (DrawDepth(canvas, faces, n_faces,
                  topo_draw.data(), unsigned(topo_draw.size()),
                  screen_fill.data(), unsigned(screen_fill.size())))
      return;
#endif

    std::sort(faces, faces + n_faces,
              [](const Face &a, const Face &b) {
                return a.depth > b.depth;
              });

    canvas.SelectNullPen();
    for (unsigned i = 0; i < n_faces; ++i) {
      const Face &face = faces[i];
      canvas.Select(Brush(face.color[0]));
      canvas.DrawPolygon(face.p, 4);
    }

    for (const ScreenTri &tri : screen_fill) {
      canvas.Select(Brush(tri.color));
      canvas.DrawPolygon(tri.p, 3);
    }

    bool down = false;
    PixelPoint prev{};
    Color prev_color = COLOR_BLACK;
    for (const TopoNode &node : topo_nodes) {
      PixelPoint sp;
      double depth = 0;
      const Vec3 world{node.e, node.n, node.z};
      if (!Project(world, eye, right, up, fwd, focal, origin,
                   sp, depth)) {
        down = false;
        continue;
      }
      const Color color(node.r, node.g, node.b, node.a);
      if (node.start || !down) {
        down = true;
        prev = sp;
        prev_color = color;
        continue;
      }
      canvas.Select(Pen(1, prev_color));
      canvas.DrawLine(prev, sp);
      prev = sp;
      prev_color = color;
    }
  }
};

#ifdef ENABLE_OPENGL

bool
TerrainWindow::PrepareDepth(PixelSize size) noexcept
{
  if (depth_failed)
    return false;
  if (depth_fbo && depth_size == size)
    return true;
  if (size.width < 1 || size.height < 1)
    return false;

  depth_fbo.reset();
  depth_rb.reset();
  color_tex.reset();

  /* FBO images are bottom-up.  The flipped flag puts canvas y=0
     at the top, same as BufferCanvas. */
  color_tex = std::make_unique<GLTexture>(GL_RGB, size, GL_RGB,
                                         GL_UNSIGNED_BYTE, true);
  depth_rb = std::make_unique<GLRenderBuffer>();
  depth_fbo = std::make_unique<GLFrameBuffer>();

  depth_fbo->Bind();
  color_tex->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);
  depth_rb->Bind();
  const PixelSize allocated = color_tex->GetAllocatedSize();
  GLRenderBuffer::Storage(GL_DEPTH_COMPONENT16,
                          allocated.width, allocated.height);
  depth_rb->AttachFramebuffer(FBO::DEPTH_ATTACHMENT);
  GLRenderBuffer::Unbind();

  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  GLFrameBuffer::Unbind();
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::fprintf(stderr, "RunTerrainView: no depth buffer (%#x)\n",
                 unsigned(status));
    depth_fbo.reset();
    depth_rb.reset();
    color_tex.reset();
    depth_failed = true;
    return false;
  }

  depth_size = size;
  return true;
}

bool
TerrainWindow::DrawDepth(Canvas &canvas, const Face *faces,
                         unsigned n_faces,
                         const TerrainVertex *topo,
                         unsigned n_topo,
                         const ScreenTri *fill, unsigned n_fill) noexcept
{
  const PixelSize size = SurfaceSize(canvas);
  if (!PrepareDepth(size))
    return false;

  static TerrainVertex verts[(MAX_N - 1) * (MAX_N - 1) * 6];
  unsigned n_verts = 0;
  for (unsigned i = 0; i < n_faces; ++i) {
    const unsigned order[6] = {0, 1, 2, 0, 2, 3};
    for (unsigned k = 0; k < 6; ++k) {
      const unsigned corner = order[k];
      TerrainVertex &v = verts[n_verts++];
      v.x = faces[i].p[corner].x;
      v.y = faces[i].p[corner].y;
      v.z = faces[i].z[corner];
      v.r = faces[i].color[corner].Red();
      v.g = faces[i].color[corner].Green();
      v.b = faces[i].color[corner].Blue();
      v.a = 255;
    }
  }

  GLint saved_vp[4];
  glGetIntegerv(GL_VIEWPORT, saved_vp);
  const GLboolean blend = glIsEnabled(GL_BLEND);
  const GLboolean cull = glIsEnabled(GL_CULL_FACE);

  depth_fbo->Bind();
  glViewport(0, 0, size.width, size.height);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  OpenGL::solid_shader->Use();
  glUniform2f(OpenGL::solid_translate, 0, 0);

  glClearColor(140 / 255.f, 190 / 255.f, 230 / 255.f, 1);
  glClearDepthf(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  static std::vector<TerrainVertex> fill_verts;
  fill_verts.clear();
  if (n_fill > 0)
    fill_verts.reserve(n_fill * 3);
  for (unsigned i = 0; i < n_fill; ++i) {
    for (unsigned k = 0; k < 3; ++k) {
      TerrainVertex v;
      v.x = fill[i].p[k].x;
      v.y = fill[i].p[k].y;
      v.z = fill[i].z[k];
      v.r = fill[i].color.Red();
      v.g = fill[i].color.Green();
      v.b = fill[i].color.Blue();
      v.a = fill[i].color.Alpha();
      fill_verts.push_back(v);
    }
  }

  if (n_verts > 0 || !fill_verts.empty() || n_topo > 0) {
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
    glEnableVertexAttribArray(OpenGL::Attribute::COLOR);
    if (n_verts > 0) {
      /* Push the mesh back so draped shapes win the depth test. */
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(1.5f, 2.f);
      glVertexAttribPointer(OpenGL::Attribute::POSITION, 3, GL_FLOAT,
                            GL_FALSE, sizeof(TerrainVertex), verts);
      glVertexAttribPointer(OpenGL::Attribute::COLOR, 4, GL_UNSIGNED_BYTE,
                            GL_TRUE, sizeof(TerrainVertex),
                            reinterpret_cast<const char *>(verts) +
                            offsetof(TerrainVertex, r));
      glDrawArrays(GL_TRIANGLES, 0, n_verts);
      glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if (!fill_verts.empty() || n_topo > 0) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if (!fill_verts.empty()) {
      /* Areas sit on the un-offset depth. Lines are drawn after. */
      glDepthFunc(GL_LEQUAL);
      glVertexAttribPointer(OpenGL::Attribute::POSITION, 3, GL_FLOAT,
                            GL_FALSE, sizeof(TerrainVertex),
                            fill_verts.data());
      glVertexAttribPointer(OpenGL::Attribute::COLOR, 4, GL_UNSIGNED_BYTE,
                            GL_TRUE, sizeof(TerrainVertex),
                            reinterpret_cast<const char *>(fill_verts.data()) +
                            offsetof(TerrainVertex, r));
      glDrawArrays(GL_TRIANGLES, 0, unsigned(fill_verts.size()));
    }
    if (n_topo > 0) {
      glDepthFunc(GL_LEQUAL);
      glVertexAttribPointer(OpenGL::Attribute::POSITION, 3, GL_FLOAT,
                            GL_FALSE, sizeof(TerrainVertex), topo);
      glVertexAttribPointer(OpenGL::Attribute::COLOR, 4, GL_UNSIGNED_BYTE,
                            GL_TRUE, sizeof(TerrainVertex),
                            reinterpret_cast<const char *>(topo) +
                            offsetof(TerrainVertex, r));
      glDrawArrays(GL_LINES, 0, n_topo);
    }
    glDepthFunc(GL_LESS);
    glDisableVertexAttribArray(OpenGL::Attribute::COLOR);
    glDisableVertexAttribArray(OpenGL::Attribute::POSITION);
  }
  glVertexAttrib4f(OpenGL::Attribute::COLOR, 1, 1, 1, 1);

  glUniform2f(OpenGL::solid_translate,
              GLfloat(OpenGL::translate.x),
              GLfloat(OpenGL::translate.y));
  glDisable(GL_DEPTH_TEST);
  if (blend)
    glEnable(GL_BLEND);
  else
    glDisable(GL_BLEND);
  if (cull)
    glEnable(GL_CULL_FACE);

  GLFrameBuffer::Unbind();
  glViewport(saved_vp[0], saved_vp[1], saved_vp[2], saved_vp[3]);

  color_tex->Bind();
  canvas.Stretch({0, 0}, size, *color_tex);
  return true;
}

#endif

static void
Main(TestMainWindow &main_window)
{
  ZipArchive archive(terrain_path);
  RasterMap map;
  TopographyStore store;
  terrain_zip = &archive;
  terrain_map = &map;
  LoadMesh(archive, map);
  LoadAirspace(archive);
  LoadTopology(archive, map, store, view_center);
#ifdef ENABLE_OPENGL
  topo_draw.reserve(200000);
#endif
  screen_fill.reserve(65536);

  TerrainWindow view;
  view.Create(main_window, main_window.GetClientRect(), WindowStyle());
  main_window.SetFullWindow(view);
  view.SetFocus();
  main_window.RunEventLoop();

  have_topo = false;
  topo_store = nullptr;
  terrain_zip = nullptr;
  terrain_map = nullptr;
  store.Reset();
}
