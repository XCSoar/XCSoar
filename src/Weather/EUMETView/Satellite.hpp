// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"
#include "system/Path.hpp"

#include <cstddef>
#include <stdexcept>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct BrokenDateTime;
struct GeoPoint;
class GeoBounds;
class CurlGlobal;
class ProgressListener;
namespace Co { template<typename T> class Task; }

/**
 * Meteosat imagery from EUMETView, EUMETSAT's public web map service.
 *
 * The projection is a request parameter here, so we ask for the very
 * box we want to draw and get a north-up image back.  The four corner
 * georeference #MapOverlayBitmap works with is therefore correct by
 * construction, and no reprojection happens on the device.
 *
 * The rendered image products are CC BY 4.0; `Fees` and
 * `AccessConstraints` are both declared `none` in the capabilities
 * document, so no account is needed.
 *
 * @see https://user.eumetsat.int/resources/user-guides/eumetview-user-guide
 */
namespace EUMETView {

/**
 * The attribution EUMETSAT's data policy requires us to display
 * wherever the imagery is shown.  The year of the frame being drawn
 * is appended to it.
 */
static constexpr const char *ATTRIBUTION =
  "Contains modified EUMETSAT Meteosat data";

/**
 * One layer offered to the pilot.
 *
 * The set is fixed rather than read from `GetCapabilities`: the
 * service publishes over a hundred layers, most of them of no use in
 * a glider, and a hardcoded table costs no XML parser and no request
 * before the first image.
 */
struct Layer {
  /** the WMS layer name, as it appears in `GetCapabilities` */
  const char *name;

  /** what the configuration dialog calls it */
  const char *label;

  /**
   * New frames appear on this grid, in minutes past the hour; the
   * layer's `PT..M` period.
   */
  unsigned cadence_minutes;

  /**
   * How old the frame on the map may get, counted from the time it
   * depicts, before it is taken down.  This also bounds the search
   * for the newest frame that exists: there is no point asking for
   * one so old that it would be taken down on arrival.
   */
  unsigned max_age_minutes;
};

/** Every layer we offer, in the order the configuration lists them. */
[[gnu::const]]
std::span<const Layer> GetLayers() noexcept;

/**
 * The layer with the given index, clamped, so that a profile written
 * by a newer version cannot select a layer that is not there.
 */
[[gnu::const]]
const Layer &GetLayer(int index) noexcept;

/**
 * The index of the layer shown when the pilot has not chosen one.
 *
 * `mtg_fd:ir105_hrfi`, because infrared is the only kind of product
 * here that carries a picture around the clock.  The visible ones --
 * and the RGB composites built on them -- are masked to fully
 * transparent where the sun is not up, which would leave a pilot who
 * enables the overlay before dawn or after dusk looking at an empty
 * map.
 */
static constexpr int DEFAULT_LAYER = 2;

/**
 * The finest tile grid we ever fetch on.
 *
 * A tile is then about 25 km across in mid latitudes, so the 5 x 5
 * block reaches at least fifty kilometres in every direction from the
 * aircraft, as an overlay meant to be flown by should.
 *
 * Zooming the map in never goes finer than this: the satellite has no
 * more detail to give, and a finer grid would only spend data
 * upsampling what is already there.
 */
static constexpr uint16_t MAX_TILE_ZOOM = 10;

/**
 * The coarsest grid, where a tile is some four hundred kilometres
 * across and the block spans a couple of thousand.  Past that the map
 * shows more than the satellite can see anyway.
 */
static constexpr uint16_t MIN_TILE_ZOOM = 6;

/**
 * How many tiles to either side of the aircraft's own tile, so the
 * block is (2 * #TILE_RANGE + 1) squared.
 */
static constexpr unsigned TILE_RANGE = 2;

/** The number of tiles in the block. */
static constexpr unsigned TILE_COUNT =
  (2 * TILE_RANGE + 1) * (2 * TILE_RANGE + 1);

/**
 * The edge length requested per tile, in pixels.
 *
 * About 195 m per pixel, which oversamples every layer here by two to
 * three times.  That is deliberate and nearly free: the interpolation
 * happens on the server, a tile still costs only about three
 * kilobytes, and the alternative -- one pixel per satellite sample --
 * would draw visibly blocky under a map the pilot zooms.
 */
static constexpr unsigned TILE_PIXELS = 128;

/**
 * The grid to fetch on for a given map view: the finest one whose
 * block still covers what the map shows, so that zooming out widens
 * the imagery instead of leaving it covering the middle of the
 * screen.
 *
 * Coarsening is the only thing the map scale decides.  Each step is a
 * halving, so an ordinary pinch does not move it, and it never goes
 * finer than #MAX_TILE_ZOOM.
 *
 * @param screen what the map shows, or an invalid bounds if unknown
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
uint16_t ChooseZoom(const GeoBounds &screen) noexcept;

/**
 * The tile the aircraft is in, on the given grid.
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
constexpr bool
IsSameTile(const GeoBitmap::TileData &a,
           const GeoBitmap::TileData &b) noexcept
{
  return a.zoom == b.zoom && a.x == b.x && a.y == b.y;
}

/**
 * Thrown when the server has no frame for the time asked for, which
 * is how it answers a frame that has not been published yet.  Told
 * apart from the other failures because it is not a lost tile but an
 * instruction to look further back.
 */
class MissingFrame final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

/**
 * The newest frame of @p layer that could exist at @p utc, i.e. the
 * wall clock rounded down to the layer's cadence.
 *
 * This is deliberately optimistic.  A frame is not on the server the
 * instant it is nominally acquired, so the first request of a block
 * may well be answered with #MissingFrame; the caller then steps back
 * a cadence at a time until one exists.  Starting from the clock
 * every time, rather than from a measured latency, means a delay that
 * grows is followed automatically and one that shrinks is picked up
 * on the very next frame instead of being carried around in a
 * constant that was true on the day it was written.
 *
 * @return an invalid time if @p utc is not plausible
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
BrokenDateTime FrameTime(const Layer &layer,
                         const BrokenDateTime &utc) noexcept;

/**
 * Build the `GetMap` URL for one tile of one frame.
 *
 * @return an empty string if the time or the tile is not usable
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
std::string MakeTileURL(const Layer &layer, const GeoBitmap::TileData &tile,
                        const BrokenDateTime &frame_time);

/**
 * Is this response a PNG?
 *
 * Worth asking, because EUMETView reports its errors as a WMS
 * `ServiceExceptionReport` carried in a perfectly ordinary `200 OK`.
 * Without this check the XML would be stored as if it were the image
 * and fail later, namelessly, in the bitmap loader.
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
bool IsPNG(std::span<const std::byte> data) noexcept;

/**
 * The message out of a WMS `ServiceExceptionReport`, for the log.
 *
 * @return an empty string if @p data is not one, or carries no
 * message
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
std::string ExtractServiceException(std::string_view data) noexcept;

/**
 * Download one tile of @p layer.
 *
 * Throws on error.
 *
 * @return the path of the downloaded image
 */
Co::Task<AllocatedPath>
DownloadTile(const Layer &layer, const GeoBitmap::TileData &tile,
             const BrokenDateTime &frame_time,
             CurlGlobal &curl, ProgressListener &progress);

} // namespace EUMETView
