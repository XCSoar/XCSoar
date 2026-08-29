// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SatellitePageOverlay.hpp"
#include "co/Task.hxx"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "MapWindow/OverlayLimits.hpp"
#include "Operation/ProgressListener.hpp"
#include "lib/curl/Global.hxx"
#include "ui/canvas/Bitmap.hpp"
#include "util/BindMethod.hxx"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

static_assert(EUMETView::TILE_COUNT <= MapWindowOverlay::MAX_MAP_OVERLAYS,
              "the tile block has to fit the map's overlay slots");

/* PageSettings.hpp cannot include the layer table, so it carries its
   own copy of the default index; tie the two together here rather
   than letting them drift apart silently */
static_assert(PageLayout::SATELLITE_LAYER_DEFAULT == EUMETView::DEFAULT_LAYER,
              "the profile default must name the default layer");

/**
 * The imagery is drawn under the map, so it has to stay a background:
 * terrain, airspace and the task are what the pilot is flying by, and
 * an opaque infrared frame would bury them.
 */
static constexpr double OVERLAY_ALPHA = 0.55;

/**
 * A tile is a few kilobytes, and there are up to twenty-five of them.
 * Showing the download bar for each would flicker it across the top
 * of the map for the whole flight, so the tiles are fetched quietly.
 */
class QuietProgress final : public ProgressListener {
public:
  void SetProgressRange([[maybe_unused]] unsigned range) noexcept override {}
  void SetProgressPosition([[maybe_unused]] unsigned position) noexcept override {}
};

static QuietProgress quiet_progress;

SatelliteDownloadGlue *
GetSatelliteDownloadGlue() noexcept
{
  if (net_components == nullptr)
    return nullptr;

  return net_components->eumetview_satellite.get();
}

SatelliteDownloadGlue::SatelliteDownloadGlue(CurlGlobal &_curl) noexcept
  :curl(_curl),
   task(curl.GetEventLoop())
{
}

void
SatelliteDownloadGlue::BeginShutdown() noexcept
{
  task.BeginShutdown();
  complete_notify.ClearNotification();
  path = nullptr;
  completion_error = {};
}

void
SatelliteDownloadGlue::Start(int _layer_index,
                             const GeoBitmap::TileData &_tile,
                             const BrokenDateTime &_frame_time) noexcept
{
  if (task.IsShuttingDown() || task.IsRunning())
    /* leave the running request alone: overwriting what it was asked
       for would georeference its image by this tile's corners */
    return;

  if (!_tile.IsValid() || !_frame_time.IsPlausible())
    return;

  layer_index = _layer_index;
  tile = _tile;
  frame_time = _frame_time;
  path = nullptr;
  completion_error = {};

  task.Start(RunDownload(), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
SatelliteDownloadGlue::RunDownload()
{
  path = co_await EUMETView::DownloadTile(EUMETView::GetLayer(layer_index),
                                          tile, frame_time, curl,
                                          quiet_progress);
}

void
SatelliteDownloadGlue::OnCompletion(std::exception_ptr error) noexcept
{
  completion_error = std::move(error);
  complete_notify.SendNotification();
}

namespace {

/**
 * One overlay slot, and the tile it is showing.
 *
 * The slot index is the map's overlay index, so a tile that survives
 * a move stays exactly where it is and is never re-uploaded.
 */
struct Slot {
  /** the overlay we installed, so we never remove somebody else's */
  const MapOverlay *overlay = nullptr;

  GeoBitmap::TileData tile{};

  /** which product this tile is of */
  int layer = -1;

  /** the frame this tile depicts */
  BrokenDateTime frame_time = BrokenDateTime::Invalid();

  constexpr bool IsUsed() const noexcept {
    return overlay != nullptr;
  }
};

std::array<Slot, EUMETView::TILE_COUNT> slots;

/** the layer the open page selected, or -1 when no page shows one */
int active_layer = -1;

/**
 * Set while the map is panning, when the page is momentarily replaced
 * by a layout carrying no overlay.  That is not the pilot leaving the
 * imagery behind, so the tiles stay.
 */
bool suspended_for_pan = false;

/** the layer the block is being filled with */
int block_layer = -1;

/** the frame the block is being filled with */
BrokenDateTime block_frame = BrokenDateTime::Invalid();

/** the aircraft's tile the current block is centred on */
GeoBitmap::TileData block_base{};

/** the tiles the block wants, nearest to the aircraft first */
std::vector<GeoBitmap::TileData> wanted;

/**
 * Tiles that failed, so a layer the server will not serve at all does
 * not spin through the whole block once a second.  Cleared whenever
 * the frame moves on.
 */
unsigned consecutive_failures = 0;

/**
 * How old the oldest tile still on the map is, or a negative duration
 * when nothing is on the map.
 *
 * Measured against the tiles actually drawn rather than the frame we
 * are currently chasing: while a new frame is being fetched the two
 * differ, and it is what the pilot can see that has to be judged.
 */
[[gnu::pure]]
std::chrono::system_clock::duration
DisplayedAge() noexcept
{
  const auto now = BrokenDateTime::NowUTC();
  if (!now.IsPlausible())
    return std::chrono::system_clock::duration{-1};

  auto oldest = std::chrono::system_clock::duration{-1};
  for (const auto &slot : slots) {
    if (!slot.IsUsed() || !slot.frame_time.IsPlausible())
      continue;

    if (const auto age = now - slot.frame_time; age > oldest)
      oldest = age;
  }

  return oldest;
}

/** Drop one slot, if the map is still showing what we put there. */
void
ReleaseSlot(unsigned index) noexcept
{
  auto &slot = slots[index];
  if (!slot.IsUsed())
    return;

  if (auto *map = UIGlobals::GetMap();
      map != nullptr && map->GetOverlay(index) == slot.overlay)
    map->SetOverlay(index, nullptr);

  slot = {};
}

/** Is this tile already on the map, showing the right frame? */
[[gnu::pure]]
bool
IsShown(const GeoBitmap::TileData &tile, int layer,
        const BrokenDateTime &frame_time) noexcept
{
  const auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return false;

  for (unsigned i = 0; i < slots.size(); ++i) {
    const auto &slot = slots[i];
    if (slot.IsUsed() && map->GetOverlay(i) == slot.overlay &&
        EUMETView::IsSameTile(slot.tile, tile) && slot.layer == layer &&
        slot.frame_time == frame_time)
      return true;
  }

  return false;
}

/**
 * Give up the slots holding tiles the block no longer wants -- the
 * ones left behind as the aircraft flew on, and everything at all
 * when the frame moved on.
 */
void
ReleaseUnwantedSlots() noexcept
{
  for (unsigned i = 0; i < slots.size(); ++i) {
    const auto &slot = slots[i];
    if (!slot.IsUsed())
      continue;

    /* only the position decides.  A tile showing an older frame stays
       on the map until the new frame's tile for the same ground has
       actually arrived, so a newly published frame does not blank the
       overlay for the length of twenty-five requests.  Letting it go
       stale is bounded by the age limit below, which is the mechanism
       for that; this one is only about which ground is covered. */
    const bool keep = slot.layer == block_layer &&
      std::any_of(wanted.begin(), wanted.end(),
                  [&slot](const auto &t){
                    return EUMETView::IsSameTile(slot.tile, t);
                  });

    if (!keep)
      ReleaseSlot(i);
  }
}

/**
 * Where to put a tile: the slot already covering that ground, so a
 * newer frame replaces the older picture of it rather than piling a
 * second overlay on top and exhausting the slots; otherwise any free
 * one.
 */
[[gnu::pure]]
int
FindSlotFor(const GeoBitmap::TileData &tile) noexcept
{
  for (unsigned i = 0; i < slots.size(); ++i)
    if (slots[i].IsUsed() && slots[i].layer == block_layer &&
        EUMETView::IsSameTile(slots[i].tile, tile))
      return int(i);

  for (unsigned i = 0; i < slots.size(); ++i)
    if (!slots[i].IsUsed())
      return int(i);

  return -1;
}

[[nodiscard]]
bool
InstallTile(Path path, int layer_index, const GeoBitmap::TileData &tile,
            const BrokenDateTime &frame_time) noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr || path == nullptr)
    return false;

  const int index = FindSlotFor(tile);
  if (index < 0)
    return false;

  Bitmap bitmap;
  try {
    if (!bitmap.LoadFile(path))
      return false;
  } catch (...) {
    LogError(std::current_exception(), "Satellite overlay");
    return false;
  }

  /* EUMETSAT's data policy makes the attribution mandatory wherever
     the imagery is shown; the overlay label carries it, so tapping
     the map names the source */
  const auto &layer = EUMETView::GetLayer(layer_index);
  /* the frame time is the part the pilot needs: it says how old the
     cloud picture is.  The year alone said nothing. */
  const auto label = fmt::format("{} {:02}:{:02}Z — {} {}",
                                 gettext(layer.label),
                                 frame_time.hour, frame_time.minute,
                                 EUMETView::ATTRIBUTION, frame_time.year);

  auto bmp = std::make_unique<MapOverlayBitmap>(std::move(bitmap),
                                                GeoBitmap::GetGeoQuadrilateral(tile),
                                                label.c_str());
  bmp->SetAlpha(OVERLAY_ALPHA);

  auto &slot = slots[index];
  slot.overlay = bmp.get();
  slot.tile = tile;
  slot.layer = layer_index;
  slot.frame_time = frame_time;
  map->SetOverlay(unsigned(index), std::move(bmp));
  return true;
}

/**
 * The next tile of the block that is not on the map yet, or an
 * invalid tile when the block is complete.  #wanted is already in
 * nearest first order, so this walks outwards from the aircraft.
 */
[[gnu::pure]]
GeoBitmap::TileData
NextMissingTile() noexcept
{
  for (const auto &tile : wanted)
    if (!IsShown(tile, block_layer, block_frame))
      return tile;

  return {};
}

} // anonymous namespace

void
SatelliteDownloadGlue::OnCompleteNotify() noexcept
{
  if (task.IsShuttingDown())
    return;

  if (completion_error) {
    LogError(std::exchange(completion_error, {}), "Satellite download");

    /* send the tile that failed to the back of the queue, so one the
       server will not serve does not stand in front of the rest of
       the block for the whole flight */
    if (const auto i = std::find_if(wanted.begin(), wanted.end(),
                                    [this](const auto &t){
                                      return EUMETView::IsSameTile(t, tile);
                                    });
        i != wanted.end())
      std::rotate(i, i + 1, wanted.end());

    /* one lost tile is nothing on a link like this; a whole block of
       them means the layer or the connection is gone, and hammering
       it would only spend the pilot's data */
    ++consecutive_failures;
    return;
  }

  if (layer_index != block_layer)
    /* the pilot changed product while this was in flight; the image
       is of something else entirely */
    return;

  if (std::none_of(wanted.begin(), wanted.end(),
                   [this](const auto &t){
                     return EUMETView::IsSameTile(t, tile);
                   }))
    /* the block moved out from under this request: the aircraft
       crossed into another tile while it was in flight, the block was
       rebuilt around the new centre and this tile's slot was given
       back.  The frame never changed, so checking that would not
       catch it, and installing now would put a tile outside the block
       on the map and hold a slot the block still wants. */
    return;

  /* a tile that arrives but will not load must count as a failure,
     or NextMissingTile() would hand back the same tile for ever and
     we would download it in a loop */
  if (InstallTile(path, layer_index, tile, frame_time))
    consecutive_failures = 0;
  else
    ++consecutive_failures;

  /* straight on to the next tile, so the block fills as fast as the
     link allows rather than one tile per timer tick */
  EUMETView::ActivatePageOverlay(active_layer);
}

void
SatelliteDownloadGlue::Schedule() noexcept
{
  /* the fastest layer publishes every five minutes, so once a minute
     catches a new frame soon enough; it also retries whatever the
     link dropped */
  timer.Schedule(std::chrono::minutes{1});
}

void
SatelliteDownloadGlue::Cancel() noexcept
{
  timer.Cancel();
}

void
SatelliteDownloadGlue::OnTimer() noexcept
{
  if (active_layer < 0)
    /* no page is showing the imagery */
    return;

  const auto age = DisplayedAge();
  if (age >= std::chrono::system_clock::duration::zero() &&
      age > std::chrono::minutes{
        EUMETView::GetLayer(active_layer).max_age_minutes}) {
    /* down it comes before anything is fetched, so that a refresh
       which hangs or fails cannot leave an hours-old cloud picture
       standing with nothing to say so */
    EUMETView::ClearMapOverlay();
  }

  /* a new minute is a new chance for whatever the link dropped */
  consecutive_failures = 0;

  EUMETView::ActivatePageOverlay(active_layer);
}

void
EUMETView::ActivatePageOverlay(int layer_index) noexcept
{
  auto *glue = GetSatelliteDownloadGlue();
  const auto *map = UIGlobals::GetMap();
  if (glue == nullptr || map == nullptr || layer_index < 0)
    return;

  active_layer = layer_index;

  /* arm the timer first: it has to keep running even on the turns
     where there is nothing to fetch, or the picture would never age
     out and a dropped tile would never be retried */
  glue->Schedule();

  const auto &basic = CommonInterface::Basic();
  if (!basic.location_available)
    /* the block is centred on the aircraft, so without a fix there is
       nothing to centre it on */
    return;

  const auto &layer = GetLayer(layer_index);
  const auto frame_time = FrameTime(layer, BrokenDateTime::NowUTC());
  if (!frame_time.IsPlausible())
    return;

  const auto base = GetAircraftTile(basic.location);
  if (!base.IsValid())
    return;

  /* recompute the block when the aircraft has crossed into another
     tile, or when a newer frame is due.  Everything is keyed on the
     tile grid, so a few kilometres of flight change nothing and the
     tiles already fetched stay on the map. */
  if (layer_index != block_layer || !IsSameTile(base, block_base) ||
      !(frame_time == block_frame)) {
    block_layer = layer_index;
    block_base = base;
    block_frame = frame_time;
    wanted = CollectTiles(base);
    ReleaseUnwantedSlots();
    consecutive_failures = 0;
  }

  if (glue->IsRunning() || wanted.empty())
    return;

  if (consecutive_failures >= wanted.size())
    /* the whole block failed in a row; wait for the timer rather than
       spin */
    return;

  const auto next = NextMissingTile();
  if (!next.IsValid())
    /* the block is complete */
    return;

  glue->Start(layer_index, next, frame_time);
}

int
EUMETView::GetActiveLayer() noexcept
{
  return active_layer;
}

void
EUMETView::ClearMapOverlay() noexcept
{
  for (unsigned i = 0; i < slots.size(); ++i)
    ReleaseSlot(i);

  block_layer = -1;
  block_frame = BrokenDateTime::Invalid();
  block_base = {};
  wanted.clear();
  consecutive_failures = 0;
}

void
EUMETView::DeactivatePageOverlay() noexcept
{
  if (suspended_for_pan)
    /* the page was only replaced by the full screen pan layout.  The
       tiles still show the right ground, and throwing them away here
       would cost the whole block again the moment panning ends. */
    return;

  if (auto *glue = GetSatelliteDownloadGlue(); glue != nullptr)
    glue->Cancel();

  active_layer = -1;
  ClearMapOverlay();
}

void
EUMETView::SuspendForPan() noexcept
{
  suspended_for_pan = true;
}

void
EUMETView::ResumeAfterPan() noexcept
{
  suspended_for_pan = false;
}
