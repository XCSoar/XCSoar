// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"
#include "Math/Point2D.hpp"
#include "system/Path.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

struct BrokenDateTime;
struct GeoPoint;
class GeoBounds;
class CurlGlobal;
class ProgressListener;
namespace Co { template<typename T> class Task; }

/**
 * The EUMETNET OPERA radar composite, a pan-European reflectivity
 * mosaic published every five minutes under CC BY 4.0.
 *
 * The composite is one cloud-optimized GeoTIFF of about five
 * megabytes covering the whole continent.  Pulling all of it down to
 * draw the weather around one glider would be absurd over a flight
 * connection, so this module reads it the way it was meant to be
 * read: a small range request for the directory, then a range request
 * per tile, nearest to the aircraft first.
 *
 * @see https://eumetnet.github.io/openradardata-documentation/
 */
namespace OPERA {

/** New composites appear on this grid, in minutes past the hour. */
static constexpr unsigned CADENCE_MINUTES = 5;

/**
 * How far behind the wall clock to look for the newest composite.  A
 * frame is not on the server the instant it is nominally acquired.
 *
 * Measured over a full day of the DBZH series: the delay between the
 * time a frame depicts and the time it appears in the bucket was 4.2
 * min at the median, 4.7 min at the 99th percentile and 6.3 min at
 * its worst.  This constant is itself the margin: when the wall clock
 * lands on a slot boundary the frame we ask for is only
 * #LATENCY_MINUTES old, so anything below the worst observed delay
 * would sometimes request a frame that is not there yet.
 */
static constexpr unsigned LATENCY_MINUTES = 7;

/**
 * How old the frame on the map may get, counted from the time it
 * depicts, before it is taken down.
 *
 * A frame is already #LATENCY_MINUTES to #LATENCY_MINUTES +
 * #CADENCE_MINUTES old when it arrives, so this has to stay clear of
 * that.  Twenty minutes leaves room for one failed refresh before the
 * picture disappears, and puts a hard bound on how stale an echo the
 * pilot can be looking at.
 */
static constexpr unsigned MAX_AGE_MINUTES = 20;

/**
 * Build the URL of the composite covering the given UTC time.  The
 * time is rounded down to #CADENCE_MINUTES.
 *
 * @return an empty string if the time is not plausible
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
std::string MakeCompositeURL(const BrokenDateTime &utc);

/**
 * The time the composite that #MakeCompositeURL() points at nominally
 * depicts.  The caller needs it to tell how old the picture on the
 * map has become.
 *
 * @return an invalid time if @p utc is not plausible
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
BrokenDateTime CompositeTime(const BrokenDateTime &utc);

/**
 * Project a geographic location onto the composite's grid, which is
 * Lambert azimuthal equal-area centred on 55N 10E.  The result is in
 * metres in the projection plane, x east and y north.
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
DoublePoint2D Project(const GeoPoint &p) noexcept;

/**
 * Map a reflectivity to one of the colour classes.  A value that is
 * not a number means the radar looked and saw nothing.
 *
 * The class boundaries were measured against the German weather
 * service's own European product rather than derived from a Z-R
 * relation: the offset between this composite, which is a maximum
 * over all elevations, and a near-ground product is not constant but
 * shrinks as the echo grows stronger, so no single exponent fits.
 *
 * Exposed for the unit test.
 *
 * @return the class index, or -1 for "no precipitation"
 */
[[gnu::const]]
int ClassifyReflectivity(double dbz) noexcept;

/**
 * Is this "not a number"?
 *
 * The composite marks "the radar looked and saw nothing" that way,
 * and we are built with -ffast-math, under which the compiler may
 * assume no such value ever occurs; so this looks at the bits rather
 * than comparing.
 *
 * Exposed for the unit test.
 */
[[gnu::const]]
bool IsNotANumber(double value) noexcept;

/** The number of colour classes #ClassifyReflectivity() may return. */
static constexpr unsigned N_CLASSES = 15;

/**
 * The colour of one class, as 0xRRGGBB.  These are the classes the
 * Deutscher Wetterdienst uses for its radar images, so the result
 * looks like what German pilots are used to reading.
 */
[[gnu::const]]
uint32_t GetClassColour(unsigned i) noexcept;

/* ------------------------------------------------------------------
 * the display grid
 * ------------------------------------------------------------------ */

/**
 * The zoom levels the overlay grid may use.
 *
 * The lower bound keeps one tile from spanning so much of the globe
 * that the flat quadrilateral the map draws it with stops matching
 * the ground; the upper bound is where a tile is finer than the one
 * kilometre the radar actually resolves and further zoom would only
 * magnify the same pixels.
 */
static constexpr uint16_t MIN_TILE_ZOOM = 4;
static constexpr uint16_t MAX_TILE_ZOOM = 10;

/**
 * How many tiles to either side of the aircraft's own tile, so the
 * block is (2 * #TILE_RANGE + 1) squared.
 */
static constexpr unsigned TILE_RANGE = 2;

/** The number of tiles in the block. */
static constexpr unsigned TILE_COUNT =
  (2 * TILE_RANGE + 1) * (2 * TILE_RANGE + 1);

/**
 * The edge length rendered per tile, in pixels.
 *
 * The grid zoom is chosen so that a tile is about a third of the
 * screen diagonal, which at typical soaring scales puts this at two
 * to five hundred metres per pixel -- oversampling the composite's
 * kilometre grid, so that the overlay does not draw blocky under a
 * map the pilot zooms.  Where the map is wider than the pyramid's
 * finest level is worth reading, #SelectLevel() drops to a coarser
 * one and the cost falls with it.
 */
static constexpr unsigned TILE_PIXELS = 256;

/**
 * How much finer than the map's own scale the tile grid is chosen.
 *
 * GeoBitmap::GetTile() sizes a tile to the screen diagonal; one step
 * finer makes the block of #TILE_COUNT cover the screen with a margin
 * to pan into, rather than five screens of imagery the pilot will
 * never look at.
 */
static constexpr unsigned TILE_ZOOM_STEP = 1;

/**
 * The tile the aircraft is in.
 *
 * @return an invalid tile if @p location is not valid
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
GeoBitmap::TileData GetAircraftTile(const GeoPoint &location,
                                    uint16_t zoom) noexcept;

/**
 * The tiles to fetch around @p base, nearest first, so that on a slow
 * or intermittent link the ground under the aircraft is drawn before
 * anything else and the picture then fills outwards.
 *
 * Tiles that fall off the top or bottom of the world are dropped, so
 * the result can be shorter than #TILE_COUNT.
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
std::vector<GeoBitmap::TileData>
CollectTiles(const GeoBitmap::TileData &base) noexcept;

/** Are these the same tile? */
[[gnu::const]]
constexpr bool
IsSameTile(const GeoBitmap::TileData &a,
           const GeoBitmap::TileData &b) noexcept
{
  return a.zoom == b.zoom && a.x == b.x && a.y == b.y;
}

/* ------------------------------------------------------------------
 * the composite's own tiling
 * ------------------------------------------------------------------ */

/** One level of the composite's overview pyramid. */
struct CompositeLevel {
  unsigned width = 0, height = 0;
  unsigned tile_width = 0, tile_height = 0;
  unsigned samples = 0;

  /** metres per pixel */
  double scale = 0;

  /** the projected coordinate of the grid's top left corner */
  double origin_x = 0, origin_y = 0;

  std::vector<uint_least32_t> tile_offset, tile_length;

  [[gnu::pure]]
  unsigned TilesAcross() const noexcept {
    return tile_width > 0 ? (width + tile_width - 1) / tile_width : 0;
  }

  [[gnu::pure]]
  unsigned TilesDown() const noexcept {
    return tile_height > 0 ? (height + tile_height - 1) / tile_height : 0;
  }

  [[gnu::pure]]
  bool IsUsable() const noexcept {
    return width > 0 && height > 0 && tile_width > 0 && tile_height > 0 &&
      scale > 0 && !tile_offset.empty() &&
      tile_offset.size() == tile_length.size();
  }
};

/**
 * The composite's directory: what the file contains and where each
 * tile of it lies, without any of the pixels.
 */
struct CompositeIndex {
  /** ordered fine to coarse, as the file stores them */
  std::vector<CompositeLevel> levels;

  /** does the file's byte order differ from this machine's? */
  bool foreign_byte_order = false;

  [[gnu::pure]]
  bool IsValid() const noexcept {
    return !levels.empty() && levels.front().IsUsable();
  }
};

/**
 * How much of the file to fetch to be sure of holding the whole
 * directory.
 *
 * Measured against the live service: the header, all five image file
 * directories and every tile offset and length table end within the
 * first 5 691 bytes.  Sixteen kilobytes is room for the file to grow
 * without another round trip, and is still a thousandth of it.
 */
static constexpr std::size_t INDEX_BYTES = 16384;

/**
 * Parse the composite's directory out of the head of the file.
 *
 * Throws if @p header is not the start of a composite, or if the
 * directory reaches past what was fetched.
 *
 * Exposed for the unit test.
 */
CompositeIndex ParseIndex(std::span<const std::byte> header);

/**
 * Projects a north-up latitude/longitude raster onto the composite's
 * grid, a row at a time.
 *
 * A row shares one latitude and every row shares the same longitudes,
 * so the trigonometry belongs outside the inner loop.  That matters
 * on the processors XCSoar runs on: once the download has shrunk to a
 * few tiles, the reprojection is the only expensive step left.
 *
 * Exposed for the unit test.
 */
class RasterProjector {
  std::vector<DoublePoint2D> delta;

public:
  /**
   * @param west the longitude of the left edge, in degrees
   * @param east the longitude of the right edge, in degrees
   * @param width how many pixels lie between them
   */
  RasterProjector(double west, double east, unsigned width);

  [[gnu::pure]]
  unsigned GetWidth() const noexcept { return delta.size(); }

  /**
   * Project one row of pixel centres.
   *
   * @param latitude the row's latitude, in degrees
   * @param out receives #GetWidth() points, x east and y north in
   * metres in the projection plane
   */
  void ProjectRow(double latitude, DoublePoint2D *out) const noexcept;
};

/**
 * The ground resolution an area would be drawn at, in metres per
 * pixel, measured in the composite's own projection.
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
double TileMetresPerPixel(const GeoBounds &bounds, unsigned pixels) noexcept;

/**
 * How much coarser than the requested resolution a level may be and
 * still be chosen.
 *
 * #TILE_PIXELS oversamples the composite by design, and the composite
 * is a maximum product, so halving the sampling loses no echo -- only
 * a little of the shape of its edge.  It does, though, quarter what
 * has to travel: one step up the pyramid is four times fewer pixels.
 * Measured over a live frame, insisting on the finer level cost 12.5%
 * of the whole file at the scale a 200 km tile is drawn at, against
 * 3% for the coarser one.
 */
static constexpr double LEVEL_TOLERANCE = 2;

/**
 * The coarsest level that still resolves @p metres_per_pixel to
 * within #LEVEL_TOLERANCE, which is the cheapest one worth fetching
 * for that scale.
 *
 * Exposed for the unit test.
 *
 * @return an index into CompositeIndex::levels
 */
[[gnu::pure]]
unsigned SelectLevel(const CompositeIndex &index,
                     double metres_per_pixel) noexcept;

/**
 * The tiles of @p level that cover @p bounds, in the file's own tile
 * numbering.
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
std::vector<unsigned>
CoveringSourceTiles(const CompositeLevel &level,
                    const GeoBounds &bounds) noexcept;

/* ------------------------------------------------------------------
 * fetching and drawing
 * ------------------------------------------------------------------ */

/**
 * One composite frame: its directory, and the tiles of it fetched so
 * far.
 *
 * Held across display tiles and across map movements so that a frame
 * is read from the network once.  Lives on the network thread.
 */
class RadarComposite {
  /**
   * One decoded tile of the composite, kept as colour classes rather
   * than reflectivities.
   *
   * A class is one byte where a reflectivity is four, and the
   * classification is monotonic, so taking the strongest class over
   * an area gives the same answer as classifying the strongest
   * reflectivity.  On the machines XCSoar runs on that is worth
   * having: a tile of the full resolution level is 256 kB this way
   * rather than a megabyte.
   */
  struct SourceTile {
    unsigned level, index;

    /** the colour class per pixel, or -1 where there is no echo */
    std::vector<int8_t> data;
  };

  /**
   * How many decoded tiles to keep.
   *
   * This has to hold a whole block, or tiles would be evicted while
   * the block that needs them is still filling and would travel a
   * second time.  Measured against a live frame, a block needs two
   * source tiles at the scales one flies at, eight at the widest
   * zoom, and fifteen at the worst point in between -- where the
   * block is wide enough to span many of them but still fine enough
   * to want the second level of the pyramid.
   *
   * Every level tiles at 512 by 512, so a tile is 256 kB as a class
   * per pixel whichever level it came from, and this bounds the cache
   * at four megabytes.
   */
  static constexpr std::size_t MAX_CACHED_TILES = 16;

  std::string url;
  CompositeIndex index;
  std::vector<SourceTile> tiles;

public:
  /** The composite this holds, empty before #Open(). */
  [[gnu::pure]]
  const std::string &GetURL() const noexcept { return url; }

  [[gnu::pure]]
  bool IsOpen() const noexcept { return index.IsValid(); }

  [[gnu::pure]]
  const CompositeIndex &GetIndex() const noexcept { return index; }

  /** Forget everything, because the frame has moved on. */
  void Reset() noexcept {
    url.clear();
    index = {};
    tiles.clear();
  }

  [[gnu::pure]]
  bool HasTile(unsigned level, unsigned tile) const noexcept;

  /**
   * Read the directory of @p url, unless it is already open.
   *
   * Throws on error.
   */
  Co::Task<void> Open(std::string _url, CurlGlobal &curl);

  /**
   * Fetch one tile of one level, unless it is already held.
   *
   * Throws on error.
   */
  Co::Task<void> FetchTile(unsigned level, unsigned tile, CurlGlobal &curl);

  /**
   * Draw the part of the composite covering @p bounds into a PNG at
   * @p path, north up.
   *
   * Tiles that were never fetched come out transparent, so a picture
   * that is still filling in shows what has arrived rather than
   * nothing.
   *
   * Throws on error.
   *
   * @return whether anything was drawn; where the sky is clear there
   * is no point spending a map overlay slot on the result
   */
  bool Render(unsigned level, const GeoBounds &bounds,
              unsigned width, unsigned height, Path path) const;
};

/**
 * Download the newest composite and draw the part covering @p bounds
 * as a single image.
 *
 * This is the Weather dialog's path, which shows one picture of the
 * area the map happens to be looking at inside a modal download.  The
 * page overlay does not use it: it fills the map tile by tile instead,
 * reusing one #RadarComposite across them.
 *
 * Throws on error.
 *
 * @return the path of the rendered image
 */
Co::Task<AllocatedPath>
DownloadArea(const GeoBounds &bounds, unsigned width, unsigned height,
             CurlGlobal &curl, ProgressListener &progress);

} // namespace OPERA
