// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/EUMETView/Satellite.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "TestUtil.hpp"

#include <span>
#include <string>

[[gnu::pure]]
static bool
Contains(const std::string &haystack, const char *needle) noexcept
{
  return haystack.find(needle) != std::string::npos;
}

int main()
{
  plan_tests(46);

  const auto layers = EUMETView::GetLayers();
  ok1(!layers.empty());

  /* every layer has to be able to hold a frame long enough to be
     replaced: a frame is already latency..latency+cadence minutes old
     when it arrives, so an age limit inside that window would expire
     the picture as it was installed, and the overlay would flicker
     rather than refresh */
  bool ages_are_clear = true;
  bool table_is_sane = true;
  for (const auto &layer : layers) {
    /* the margin has to leave room for one refresh to fail */
    if (layer.max_age_minutes <
        layer.latency_minutes + 2 * layer.cadence_minutes)
      ages_are_clear = false;

    if (layer.cadence_minutes == 0 || 60 % layer.cadence_minutes != 0)
      /* the frame grid has to divide the hour, or rounding the minute
         down to a multiple of it would name times that never occur */
      table_is_sane = false;

    if (layer.name == nullptr || layer.label == nullptr)
      table_is_sane = false;
  }

  ok1(ages_are_clear);
  ok1(table_is_sane);

  /* an index out of range must fall back rather than read past the
     table: a profile written by a newer version can name a layer this
     build does not have */
  ok1(&EUMETView::GetLayer(-1) == &layers[EUMETView::DEFAULT_LAYER]);
  ok1(&EUMETView::GetLayer(int(layers.size())) ==
      &layers[EUMETView::DEFAULT_LAYER]);
  ok1(&EUMETView::GetLayer(0) == &layers[0]);

  /* ---------------- frame times ---------------- */

  const auto &ten = EUMETView::GetLayer(1);
  ok1(ten.cadence_minutes == 10);

  const BrokenDateTime noon{BrokenDate{2026, 8, 28}, BrokenTime{12, 30, 45}};
  const auto frame = EUMETView::FrameTime(ten, noon);
  ok1(frame.IsPlausible());
  ok1(frame.minute % ten.cadence_minutes == 0);
  ok1(frame.second == 0);

  /* it is never newer than the delay, and never older than the delay
     plus one cadence step */
  const auto age = noon - frame;
  ok1(age >= std::chrono::minutes{ten.latency_minutes});
  ok1(age < std::chrono::minutes{ten.latency_minutes + ten.cadence_minutes});

  /* the delay may carry the frame back across midnight */
  const BrokenDateTime after_midnight{BrokenDate{2026, 8, 28},
                                      BrokenTime{0, 2, 0}};
  const auto wrapped = EUMETView::FrameTime(ten, after_midnight);
  ok1(wrapped.IsPlausible());
  ok1(wrapped.day == 27);
  ok1(wrapped.hour == 23);

  /* without a clock we must not guess: asking EUMETView for a frame
     that does not exist is an error, not an older picture */
  ok1(!EUMETView::FrameTime(ten, BrokenDateTime::Invalid()).IsPlausible());

  /* ---------------- the tile block ---------------- */

  const GeoPoint aircraft{Angle::Degrees(11), Angle::Degrees(50)};
  const auto base = EUMETView::GetAircraftTile(aircraft);
  ok1(base.IsValid());
  ok1(base.zoom == EUMETView::TILE_ZOOM);

  /* the aircraft has to be inside its own tile, or the block would be
     centred on the wrong ground */
  ok1(GeoBitmap::GetBounds(base).IsInside(aircraft));

  /* without a fix there is nothing to centre on */
  ok1(!EUMETView::GetAircraftTile(GeoPoint::Invalid()).IsValid());

  const auto tiles = EUMETView::CollectTiles(base);
  ok1(tiles.size() == EUMETView::TILE_COUNT);

  /* the aircraft's own tile is fetched first.  This is the whole
     point of the ordering: on a link that may die at any moment, the
     ground under the glider has to be the part that arrives. */
  ok1(EUMETView::IsSameTile(tiles.front(), base));

  /* and the rest follow outwards, never getting closer again, so the
     picture grows as rings around the aircraft */
  bool rings_grow = true;
  bool all_distinct = true;
  unsigned previous = 0;
  for (std::size_t i = 0; i < tiles.size(); ++i) {
    const auto dx = int(tiles[i].x) - int(base.x);
    const auto dy = int(tiles[i].y) - int(base.y);
    const unsigned distance = unsigned(dx * dx + dy * dy);
    if (distance < previous)
      rings_grow = false;

    previous = distance;

    for (std::size_t j = 0; j < i; ++j)
      if (EUMETView::IsSameTile(tiles[i], tiles[j]))
        all_distinct = false;
  }

  ok1(rings_grow);
  ok1(all_distinct);

  /* the block has to reach far enough to be worth flying by: at least
     fifty kilometres in every direction from the aircraft */
  GeoBounds block = GeoBitmap::GetBounds(tiles.front());
  for (const auto &tile : tiles) {
    const auto b = GeoBitmap::GetBounds(tile);
    block.Extend(b.GetNorthWest());
    block.Extend(b.GetSouthEast());
  }

  ok1(block.IsInside(aircraft));
  ok1(block.GetGeoWidth() >= 100000);
  ok1(block.GetGeoHeight() >= 100000);

  /* one tile is a useful picture on its own, not a postage stamp */
  const auto centre = GeoBitmap::GetBounds(base);
  ok1(centre.GetGeoWidth() > 10000);
  ok1(centre.GetGeoWidth() < 60000);

  /* an invalid base tile yields no work rather than a garbage block */
  ok1(EUMETView::CollectTiles({}).empty());

  /* near the pole the block runs off the top of the world; the tiles
     that remain must still be a valid, shorter list */
  const auto polar = EUMETView::GetAircraftTile({Angle::Degrees(0),
                                                 Angle::Degrees(84.9)});
  const auto polar_tiles = EUMETView::CollectTiles(polar);
  ok1(polar_tiles.size() <= EUMETView::TILE_COUNT);
  ok1(!polar_tiles.empty());

  /* longitude wraps, so a flight over the date line keeps a whole
     block instead of losing the half that fell off the edge */
  const auto dateline = EUMETView::GetAircraftTile({Angle::Degrees(179.99),
                                                    Angle::Degrees(50)});
  ok1(EUMETView::CollectTiles(dateline).size() == EUMETView::TILE_COUNT);

  /* ---------------- the request ---------------- */

  const auto url = EUMETView::MakeTileURL(ten, base, frame);
  ok1(Contains(url, "https://view.eumetsat.int/geoserver/wms?"));
  ok1(Contains(url, "REQUEST=GetMap"));

  /* the layer name's colon is reserved in a query value */
  ok1(Contains(url, "LAYERS=mtg_fd%3Avis06_hrfi"));

  /* CRS:84 and not EPSG:4326, whose axis order is latitude first in
     WMS 1.3.0; getting this wrong transposes the image silently */
  ok1(Contains(url, "CRS=CRS:84"));
  ok1(Contains(url, "&TIME=2026-08-28T"));
  ok1(Contains(url, "&WIDTH=128&HEIGHT=128"));

  /* an unusable request must produce no URL at all rather than one
     the server will reject */
  ok1(EUMETView::MakeTileURL(ten, {}, frame).empty());
  ok1(EUMETView::MakeTileURL(ten, base, BrokenDateTime::Invalid()).empty());

  /* ---------------- what came back ---------------- */

  static constexpr std::byte PNG[]{
    std::byte(0x89), std::byte('P'), std::byte('N'), std::byte('G'),
    std::byte(0x0d), std::byte(0x0a), std::byte(0x1a), std::byte(0x0a),
    std::byte(0), std::byte(0),
  };
  ok1(EUMETView::IsPNG(PNG));
  ok1(!EUMETView::IsPNG(std::span<const std::byte>{PNG}.first(4)));

  /* EUMETView reports its errors as a ServiceExceptionReport inside a
     perfectly ordinary "200 OK", so the image we asked for may in
     fact be XML; it has to be recognised while the message can still
     be read */
  static constexpr char EXCEPTION[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<ServiceExceptionReport version=\"1.3.0\">"
    "   <ServiceException code=\"InvalidDimensionValue\" locator=\"time\">\n"
    "      No nearest match found on mtg_fd:vis06_hrfi\n"
    "</ServiceException></ServiceExceptionReport>";
  ok1(!EUMETView::IsPNG(std::as_bytes(std::span{EXCEPTION}).first(8)));
  ok1(EUMETView::ExtractServiceException(EXCEPTION) ==
      "No nearest match found on mtg_fd:vis06_hrfi");
  ok1(EUMETView::ExtractServiceException("not xml at all").empty());

  return exit_status();
}
