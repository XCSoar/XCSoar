// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadarPageOverlay.hpp"
#include "co/Task.hxx"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Quadrilateral.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "MapWindow/OverlayLimits.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "lib/curl/Global.hxx"
#include "ui/canvas/Bitmap.hpp"
#include "util/BindMethod.hxx"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <utility>
#include <vector>

static_assert(OPERA::TILE_COUNT <= MapWindowOverlay::MAX_MAP_OVERLAYS,
              "the radar block must fit in the map's overlay slots");

RadarDownloadGlue *
GetRadarDownloadGlue() noexcept
{
  if (net_components == nullptr)
    return nullptr;

  return net_components->opera_radar.get();
}

RadarDownloadGlue::RadarDownloadGlue(CurlGlobal &_curl) noexcept
  :curl(_curl),
   task(curl.GetEventLoop())
{
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

  /** the frame this tile depicts */
  BrokenDateTime frame_time = BrokenDateTime::Invalid();

  /**
   * Held for a download in flight, or for a tile that turned out to
   * hold no echo and so has nothing to install.  Either way the slot
   * is spoken for and must not be handed out again.
   */
  bool reserved = false;

  constexpr bool IsUsed() const noexcept {
    return overlay != nullptr || reserved;
  }
};

std::array<Slot, OPERA::TILE_COUNT> slots;

/** does any page show the radar? */
bool active = false;

/**
 * Is the radar being held on the map across a pan?  Panning swaps in
 * a layout with no overlay, which would otherwise tear the block down
 * and cost a full refetch on the way back.
 */
bool suspended_for_pan = false;

/** the frame the block is being filled with */
BrokenDateTime block_frame = BrokenDateTime::Invalid();

/** the aircraft's tile the current block is centred on */
GeoBitmap::TileData block_base{};

/** the tiles the block wants, nearest to the aircraft first */
std::vector<GeoBitmap::TileData> wanted;

/**
 * Tiles that failed in a row, so a composite the server will not
 * serve does not spin through the whole block once a second.
 */
unsigned consecutive_failures = 0;

/** is the progress bar up for the block being filled? */
bool progress_shown = false;

/**
 * Set when a frame was taken off the map for age and the refresh that
 * should replace it has not succeeded yet.  If that refresh fails,
 * the pilot is told the radar is gone rather than left wondering.
 */
bool stale_removed = false;

void
BeginProgress() noexcept
{
  if (progress_shown)
    return;

  BackgroundDownloadProgress::Get().Begin(_("Downloading radar..."));
  progress_shown = true;
}

void
EndProgress() noexcept
{
  if (!progress_shown)
    return;

  BackgroundDownloadProgress::Get().End();
  progress_shown = false;
}

[[gnu::pure]]
bool
IsStaleWarningHidden() noexcept
{
  bool hidden = false;
  Profile::Get(ProfileKeys::HideRadarStaleWarning, hidden);
  return hidden;
}

void
WarnStale() noexcept
{
  if (IsStaleWarningHidden())
    return;

  /* the opt-out follows the Quick Guide dialog: answering "no" writes
     the choice to the profile, and the Look configuration page can
     switch it back on */
  if (ShowMessageBox(_("The radar image could not be refreshed and has "
                       "been removed, so that no outdated echo is shown.\n\n"
                       "Warn again when this happens?"),
                     _("Radar"), MB_YESNO | MB_ICONWARNING) == IDNO) {
    Profile::Set(ProfileKeys::HideRadarStaleWarning, true);
    Profile::Save();
  }
}

/**
 * How long ago the frame on the map was taken, or a negative duration
 * when nothing is on the map.
 */
[[gnu::pure]]
std::chrono::system_clock::duration
BlockAge() noexcept
{
  const auto now = BrokenDateTime::NowUTC();
  if (!block_frame.IsPlausible() || !now.IsPlausible())
    return std::chrono::system_clock::duration{-1};

  return now - block_frame;
}

/** Drop one slot, if the map is still showing what we put there. */
void
ReleaseSlot(unsigned index) noexcept
{
  auto &slot = slots[index];
  if (!slot.IsUsed())
    return;

  if (auto *map = UIGlobals::GetMap();
      map != nullptr && slot.overlay != nullptr &&
      map->GetOverlay(index) == slot.overlay)
    map->SetOverlay(index, nullptr);

  slot = {};
}

/** Is this tile already dealt with, showing the right frame? */
[[gnu::pure]]
bool
IsShown(const GeoBitmap::TileData &tile,
        const BrokenDateTime &frame_time) noexcept
{
  const auto *map = UIGlobals::GetMap();

  for (unsigned i = 0; i < slots.size(); ++i) {
    const auto &slot = slots[i];
    if (!slot.IsUsed() || !OPERA::IsSameTile(slot.tile, tile) ||
        !(slot.frame_time == frame_time))
      continue;

    if (slot.overlay == nullptr)
      /* reserved: either in flight or known to hold no echo -- either
         way there is nothing left to fetch for it */
      return true;

    if (map != nullptr && map->GetOverlay(i) == slot.overlay)
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

    const bool keep = slot.frame_time == block_frame &&
      std::any_of(wanted.begin(), wanted.end(),
                  [&slot](const auto &t){
                    return OPERA::IsSameTile(slot.tile, t);
                  });

    if (!keep)
      ReleaseSlot(i);
  }
}

/** The first slot not holding a tile we want to keep. */
[[gnu::pure]]
int
FindFreeSlot() noexcept
{
  for (unsigned i = 0; i < slots.size(); ++i)
    if (!slots[i].IsUsed())
      return int(i);

  return -1;
}

void
InstallTile(unsigned index, Path path, const GeoBitmap::TileData &tile,
            const BrokenDateTime &frame_time) noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr || path == nullptr || index >= slots.size())
    return;

  Bitmap bitmap;
  try {
    if (!bitmap.LoadFile(path))
      return;
  } catch (...) {
    LogError(std::current_exception(), "Radar overlay");
    return;
  }

  /* OPERA is published under CC BY 4.0, so the attribution has to
     travel with the picture; the overlay label carries it, which is
     what the map shows when the pilot taps it */
  const auto label = fmt::format("{} — EUMETNET OPERA", gettext(N_("Radar")));

  auto bmp = std::make_unique<MapOverlayBitmap>(std::move(bitmap),
                                                GeoBitmap::GetGeoQuadrilateral(tile),
                                                label.c_str());
  bmp->SetAlpha(0.6);

  auto &slot = slots[index];
  slot.overlay = bmp.get();
  slot.tile = tile;
  slot.frame_time = frame_time;
  slot.reserved = false;
  map->SetOverlay(index, std::move(bmp));

  stale_removed = false;
}

/**
 * The next tile of the block that is not dealt with yet, or an
 * invalid tile when the block is complete.  #wanted is already in
 * nearest first order, so this walks outwards from the aircraft.
 */
[[gnu::pure]]
GeoBitmap::TileData
NextMissingTile() noexcept
{
  for (const auto &tile : wanted)
    if (!IsShown(tile, block_frame))
      return tile;

  return {};
}

} // anonymous namespace

void
RadarDownloadGlue::BeginShutdown() noexcept
{
  task.BeginShutdown();
  complete_notify.ClearNotification();
  path = nullptr;
  completion_error = {};
  progress_shown = false;
  BackgroundDownloadProgress::Get().ForceHide();
}

void
RadarDownloadGlue::Start(std::string _url, const GeoBitmap::TileData &_base,
                         const GeoBitmap::TileData &_tile,
                         const BrokenDateTime &_frame_time,
                         unsigned _slot) noexcept
{
  /* every request parameter is set here, after the guards: an early
     return must not leave the next completion labelled with a request
     that was never made */
  if (task.IsShuttingDown() || task.IsRunning())
    return;

  if (_url.empty() || !_base.IsValid() || !_tile.IsValid() ||
      !_frame_time.IsPlausible() || _slot >= slots.size())
    return;

  url = std::move(_url);
  base_tile = _base;
  tile = _tile;
  frame_time = _frame_time;
  slot = _slot;
  drawn = false;
  path = nullptr;
  completion_error = {};

  /* one file per slot, so the cache cannot grow with the flight */
  path = AllocatedPath::Build(MakeCacheDirectory("opera"),
                              fmt::format("rain-{}.png", slot).c_str());

  /* hold the slot now: the next tile must not be handed the same one
     while this download is in flight */
  slots[slot].tile = tile;
  slots[slot].frame_time = frame_time;
  slots[slot].reserved = true;

  BeginProgress();
  task.Start(RunDownload(), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
RadarDownloadGlue::RunDownload()
{
  /* one range request for the directory; a no-op once the frame is
     open, which is every tile after the first */
  co_await composite.Open(url, curl);

  const auto bounds = GeoBitmap::GetBounds(tile);
  const auto &index = composite.GetIndex();

  /* the overview that matches what the block is drawn at, taken from
     its centre tile so that every tile of it uses the same one:
     asking for a finer level only costs bandwidth and decode time,
     and asking for different ones across the block would fetch the
     same ground twice */
  const auto level_index =
    OPERA::SelectLevel(index,
                       OPERA::TileMetresPerPixel(GeoBitmap::GetBounds(base_tile),
                                                 OPERA::TILE_PIXELS));

  for (const auto source : OPERA::CoveringSourceTiles(index.levels[level_index],
                                                      bounds))
    co_await composite.FetchTile(level_index, source, curl);

  drawn = composite.Render(level_index, bounds,
                           OPERA::TILE_PIXELS, OPERA::TILE_PIXELS, path);
}

void
RadarDownloadGlue::OnCompletion(std::exception_ptr error) noexcept
{
  completion_error = std::move(error);
  complete_notify.SendNotification();
}

void
RadarDownloadGlue::OnCompleteNotify() noexcept
{
  if (task.IsShuttingDown())
    return;

  if (completion_error) {
    LogError(std::exchange(completion_error, {}), "Radar download");

    ReleaseSlot(slot);

    /* send the tile that failed to the back of the queue, so one the
       server will not serve does not stand in front of the rest of
       the block for the whole flight */
    if (const auto i = std::find_if(wanted.begin(), wanted.end(),
                                    [this](const auto &t){
                                      return OPERA::IsSameTile(t, tile);
                                    });
        i != wanted.end())
      std::rotate(i, i + 1, wanted.end());

    ++consecutive_failures;
    EndProgress();

    /* a lost tile on its own is not worth a dialog; it only matters
       once it means the map has no radar left to show */
    if (std::exchange(stale_removed, false))
      WarnStale();

    return;
  }

  consecutive_failures = 0;

  /* The block can move while a tile is in flight, and then this
     answer is to a question nobody is asking any more.  Two ways:
     a newer frame fell due, or the aircraft crossed into another tile
     and the block was rebuilt around it.  The frame check alone
     misses the second -- the frame is unchanged there, but
     ReleaseUnwantedSlots() has already given this slot back, and
     installing now would drop a tile from outside the block onto the
     map and hold a slot the block wants for something else. */
  const bool still_wanted = frame_time == block_frame &&
    std::any_of(wanted.begin(), wanted.end(),
                [this](const auto &t){
                  return OPERA::IsSameTile(t, tile);
                });

  if (!still_wanted)
    ReleaseSlot(slot);
  else if (drawn)
    InstallTile(slot, path, tile, frame_time);
  else
    /* clear sky here: the slot stays reserved for this tile so the
       block does not ask for it again, but nothing is drawn */
    slots[slot].reserved = true;

  /* straight on to the next tile, so the block fills as fast as the
     link allows rather than one tile per timer tick */
  OPERA::ActivatePageOverlay();
}

void
RadarDownloadGlue::ScheduleAgeCheck() noexcept
{
  /* the composite changes every five minutes, so checking once a
     minute is often enough to catch a new one and cheap enough to run
     for as long as the page is open */
  age_timer.Schedule(std::chrono::minutes{1});
}

void
RadarDownloadGlue::CancelAgeCheck() noexcept
{
  age_timer.Cancel();
}

void
RadarDownloadGlue::OnAgeTimer() noexcept
{
  if (!active)
    return;

  const auto age = BlockAge();
  if (age >= std::chrono::system_clock::duration::zero() &&
      age > std::chrono::minutes{OPERA::MAX_AGE_MINUTES}) {
    /* down it comes before anything is fetched: an echo this old is
       worse than none, and a refresh that hangs or fails must not be
       able to leave it standing */
    OPERA::ClearMapOverlay();
    stale_removed = true;
  }

  /* a new minute is a new chance for whatever the link dropped */
  consecutive_failures = 0;

  OPERA::ActivatePageOverlay();
}

void
OPERA::ActivatePageOverlay() noexcept
{
  auto *glue = GetRadarDownloadGlue();
  const auto *map = UIGlobals::GetMap();
  if (glue == nullptr || map == nullptr)
    return;

  active = true;

  /* arm the watchdog first: it has to keep running even on the turns
     where there is nothing new to fetch, or the picture would never
     age out and a dropped tile would never be retried */
  glue->ScheduleAgeCheck();

  const auto &projection = map->VisibleProjection();
  if (!projection.IsValid())
    return;

  const auto now = BrokenDateTime::NowUTC();
  auto url = MakeCompositeURL(now);
  const auto frame_time = CompositeTime(now);
  if (url.empty() || !frame_time.IsPlausible())
    /* without a clock we cannot tell which composite is the current
       one, and guessing would show yesterday's weather */
    return;

  /* One step finer than the map's own scale, so the block covers the
     screen with a margin to pan into.  The geometric choice is asked
     for one step coarser than the grid we want, so that adding the
     step lands inside [MIN_TILE_ZOOM, MAX_TILE_ZOOM] rather than
     saturating one short of either end. */
  const auto map_tile = GeoBitmap::GetTile(projection,
                                           MIN_TILE_ZOOM - TILE_ZOOM_STEP,
                                           MAX_TILE_ZOOM - TILE_ZOOM_STEP);
  const auto zoom = uint16_t(map_tile.zoom + TILE_ZOOM_STEP);

  const auto &basic = CommonInterface::Basic();
  const auto base = basic.location_available
    ? GetAircraftTile(basic.location, zoom)
    /* before the first fix there is no aircraft to centre on, so the
       block goes where the pilot is looking instead */
    : GeoBitmap::GetTile(projection.GetScreenBounds(), zoom);
  if (!base.IsValid())
    return;

  /* recompute the block when the aircraft has crossed into another
     tile, when the map scale changed the grid, or when a newer frame
     is due.  Everything is keyed on the tile grid, so a few
     kilometres of flight change nothing and the tiles already on the
     map stay there. */
  if (!IsSameTile(base, block_base) || !(frame_time == block_frame)) {
    block_base = base;
    block_frame = frame_time;
    wanted = CollectTiles(base);
    ReleaseUnwantedSlots();
    consecutive_failures = 0;
  }

  if (glue->IsRunning() || wanted.empty())
    return;

  if (consecutive_failures >= wanted.size()) {
    /* the whole block failed in a row; wait for the timer rather than
       spin through it again */
    EndProgress();
    return;
  }

  const auto next = NextMissingTile();
  if (!next.IsValid()) {
    /* the block is complete */
    EndProgress();
    return;
  }

  const auto slot = FindFreeSlot();
  if (slot < 0)
    return;

  glue->Start(std::move(url), base, next, frame_time, unsigned(slot));
}

void
OPERA::ClearMapOverlay() noexcept
{
  for (unsigned i = 0; i < slots.size(); ++i)
    ReleaseSlot(i);

  block_frame = BrokenDateTime::Invalid();
  block_base = {};
  wanted.clear();
  consecutive_failures = 0;
  EndProgress();
}

void
OPERA::DeactivatePageOverlay() noexcept
{
  if (suspended_for_pan)
    /* the pilot is dragging the map, not leaving the radar behind */
    return;

  if (auto *glue = GetRadarDownloadGlue(); glue != nullptr)
    glue->CancelAgeCheck();

  active = false;
  stale_removed = false;
  ClearMapOverlay();
}

void
OPERA::SuspendForPan() noexcept
{
  suspended_for_pan = true;
}

void
OPERA::ResumeAfterPan() noexcept
{
  suspended_for_pan = false;
}
