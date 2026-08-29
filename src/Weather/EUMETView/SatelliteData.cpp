// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Satellite.hpp"
#include "Geo/GeoBounds.hpp"
#include "Language/Language.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

namespace {

constexpr const char *SERVICE_URL = "https://view.eumetsat.int/geoserver/wms";

constexpr std::array<EUMETView::Layer, 6> LAYERS{{
  /* first, and the default: the closest counterpart to the pc_met
     vis_hrv images, and the one product here that covers Europe at
     the HRV sampling German pilots are used to reading */
  {"msg_fes:rgb_eview", N_("European HRV RGB"), 15, 30, 65},

  /* MTG, which resolves about twice as finely as SEVIRI HRV */
  {"mtg_fd:vis06_hrfi", N_("Visible 0.6 µm (MTG)"), 10, 25, 50},
  {"mtg_fd:ir105_hrfi", N_("Infrared 10.5 µm (MTG)"), 10, 25, 50},

  /* the RGB composites are assembled from several channels and reach
     the server a few minutes behind the single channel products */
  {"mtg_fd:rgb_truecolour", N_("True colour (MTG)"), 10, 30, 55},
  {"mtg_fd:rgb_geocolour", N_("Geo colour (MTG)"), 10, 30, 55},

  /* five minute cadence, and the promptest of them by a wide margin */
  {"msg_rss:rgb_natural_nrt", N_("Rapid scan natural colour"), 5, 15, 30},
}};

/**
 * Percent-encode the characters of a WMS layer name that are
 * reserved in a query value.  GeoServer accepts the colon raw, but a
 * name that survives any proxy in between costs one line.
 */
[[gnu::pure]]
std::string
EncodeLayerName(const char *name) noexcept
{
  std::string result;
  for (const char *p = name; *p != '\0'; ++p) {
    if (*p == ':')
      result += "%3A";
    else
      result.push_back(*p);
  }

  return result;
}

} // anonymous namespace

std::span<const EUMETView::Layer>
EUMETView::GetLayers() noexcept
{
  return LAYERS;
}

const EUMETView::Layer &
EUMETView::GetLayer(int index) noexcept
{
  /* clamped rather than checked: a profile written by a newer version
     may name a layer this build does not have, and falling back to
     the default beats refusing to draw anything */
  if (index < 0 || unsigned(index) >= LAYERS.size())
    index = DEFAULT_LAYER;

  return LAYERS[index];
}

uint16_t
EUMETView::ChooseZoom(const GeoBounds &screen) noexcept
{
  if (!screen.IsValid())
    return MAX_TILE_ZOOM;

  /* the widest the map shows; the block is square, so the larger of
     the two has to fit */
  const double view = std::max(screen.GetGeoWidth(), screen.GetGeoHeight());
  if (!(view > 0))
    return MAX_TILE_ZOOM;

  /* a tile at zoom z spans EQUATOR / 2^z along the parallel, and the
     block is (2 * TILE_RANGE + 1) of them.  Take the coarsest step
     that still covers the view, then refuse to go finer than the
     satellite's own detail. */
  constexpr double EQUATOR = 40075017;
  const double parallel =
    EQUATOR * std::cos(screen.GetCenter().latitude.Radians());
  const double tiles_across = 2 * TILE_RANGE + 1;

  const double ratio = tiles_across * parallel / view;
  if (!(ratio > 1))
    return MIN_TILE_ZOOM;

  const int zoom = int(std::floor(std::log2(ratio)));
  return uint16_t(std::clamp(zoom, int(MIN_TILE_ZOOM), int(MAX_TILE_ZOOM)));
}

GeoBitmap::TileData
EUMETView::GetAircraftTile(const GeoPoint &location, uint16_t zoom) noexcept
{
  if (!location.IsValid())
    return {};

  return GeoBitmap::GetTile(GeoBounds{location}, zoom);
}

std::vector<GeoBitmap::TileData>
EUMETView::CollectTiles(const GeoBitmap::TileData &base) noexcept
{
  std::vector<GeoBitmap::TileData> result;
  if (!base.IsValid())
    return result;

  const int tiles_per_axis = 1 << base.zoom;

  /* longitude wraps, so a flight over the date line keeps a complete
     block instead of losing half of it */
  const auto normalise_x = [tiles_per_axis](int value) {
    int result = value % tiles_per_axis;
    if (result < 0)
      result += tiles_per_axis;

    return uint32_t(result);
  };

  struct Candidate {
    GeoBitmap::TileData tile;
    unsigned priority;
  };

  std::vector<Candidate> candidates;
  candidates.reserve(TILE_COUNT);

  const int range = int(TILE_RANGE);
  for (int dx = -range; dx <= range; ++dx) {
    for (int dy = -range; dy <= range; ++dy) {
      const int y = int(base.y) + dy;
      if (y < 0 || y >= tiles_per_axis)
        /* latitude does not wrap: off the top or bottom of the world
           there is no tile to ask for */
        continue;

      candidates.push_back({
        {base.zoom, normalise_x(int(base.x) + dx), uint32_t(y)},
        unsigned(dx * dx + dy * dy),
      });
    }
  }

  /* squared distance from the aircraft's own tile, so the block is
     walked as rings growing outwards from underneath the glider */
  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const auto &a, const auto &b) {
                     return a.priority < b.priority;
                   });

  result.reserve(candidates.size());
  for (const auto &candidate : candidates)
    result.push_back(candidate.tile);

  return result;
}

BrokenDateTime
EUMETView::FrameTime(const Layer &layer, const BrokenDateTime &utc) noexcept
{
  if (!utc.IsPlausible())
    /* without a clock we cannot tell which frame is the current one,
       and guessing would ask for one that does not exist */
    return BrokenDateTime::Invalid();

  auto t = utc - std::chrono::minutes{layer.latency_minutes};
  t.minute = (t.minute / layer.cadence_minutes) * layer.cadence_minutes;
  t.second = 0;
  return t;
}

std::string
EUMETView::MakeTileURL(const Layer &layer, const GeoBitmap::TileData &tile,
                       const BrokenDateTime &frame_time)
{
  if (!tile.IsValid() || !frame_time.IsPlausible())
    return {};

  const auto bounds = GeoBitmap::GetBounds(tile);
  if (!bounds.IsValid())
    return {};

  /* The tile grid is the Web Mercator one, but the image is asked for
     in CRS:84 and therefore comes back in plate carree.  That is the
     projection MapOverlayBitmap assumes when it interpolates between
     the four corners, so the georeference is exact; requesting
     EPSG:3857 would return a Mercator image and leave the renderer
     interpolating latitude linearly through it.

     CRS:84 rather than EPSG:4326 because the latter puts latitude
     first in WMS 1.3.0, a standing source of silently transposed
     maps. */
  return fmt::format("{}?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
                     "&LAYERS={}&STYLES=&CRS=CRS:84"
                     "&BBOX={:.6f},{:.6f},{:.6f},{:.6f}"
                     "&WIDTH={}&HEIGHT={}&FORMAT=image/png&TRANSPARENT=TRUE"
                     "&TIME={:04}-{:02}-{:02}T{:02}:{:02}:00Z",
                     SERVICE_URL, EncodeLayerName(layer.name),
                     bounds.GetWest().Degrees(), bounds.GetSouth().Degrees(),
                     bounds.GetEast().Degrees(), bounds.GetNorth().Degrees(),
                     TILE_PIXELS, TILE_PIXELS,
                     frame_time.year, frame_time.month, frame_time.day,
                     frame_time.hour, frame_time.minute);
}

bool
EUMETView::IsPNG(std::span<const std::byte> data) noexcept
{
  static constexpr std::array<unsigned char, 8> SIGNATURE{
    0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a,
  };

  if (data.size() < SIGNATURE.size())
    return false;

  return std::equal(SIGNATURE.begin(), SIGNATURE.end(), data.begin(),
                    [](unsigned char a, std::byte b){
                      return a == (unsigned char)b;
                    });
}

std::string
EUMETView::ExtractServiceException(std::string_view data) noexcept
{
  /* the report's own element is <ServiceExceptionReport>, which
     starts with the same characters, so a plain search would find the
     wrapper and return the inner tag along with the message */
  std::string_view::size_type open = 0;
  while (true) {
    open = data.find("<ServiceException", open);
    if (open == data.npos)
      return {};

    const auto after = open + std::strlen("<ServiceException");
    if (after >= data.size())
      return {};

    if (const char ch = data[after];
        ch == '>' || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
      break;

    ++open;
  }

  const auto text = data.find('>', open);
  if (text == data.npos)
    return {};

  const auto close = data.find("</ServiceException>", text);
  if (close == data.npos)
    return {};

  auto message = data.substr(text + 1, close - text - 1);

  /* the report is pretty-printed across several lines; fold it into
     one so it fits a log entry */
  std::string result;
  bool space = true;
  for (char ch : message) {
    if (ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ') {
      if (!space && !result.empty())
        result.push_back(' ');
      space = true;
    } else {
      result.push_back(ch);
      space = false;
    }
  }

  while (!result.empty() && result.back() == ' ')
    result.pop_back();

  return result;
}
