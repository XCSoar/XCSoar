// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadarPageOverlay.hpp"
#include "Radar.hpp"
#include "co/Task.hxx"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "UIGlobals.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "lib/curl/Global.hxx"
#include "time/BrokenDateTime.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "util/BindMethod.hxx"

#include <chrono>
#include <memory>
#include <string>
#include <utility>

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

void
RadarDownloadGlue::BeginShutdown() noexcept
{
  task.BeginShutdown();
  complete_notify.ClearNotification();
  path = nullptr;
  completion_error = {};
  BackgroundDownloadProgress::Get().ForceHide();
}

void
RadarDownloadGlue::Start(const GeoBounds &_bounds,
                         unsigned _width, unsigned _height) noexcept
{
  if (task.IsShuttingDown() || task.IsRunning())
    return;

  if (!_bounds.IsValid() || _width == 0 || _height == 0)
    return;

  bounds = _bounds;
  width = _width;
  height = _height;
  path = nullptr;
  completion_error = {};

  BackgroundDownloadProgress::Get().Begin(_("Downloading radar..."));
  BackgroundDownloadProgress::Get().SetProgressRange(100);

  task.Start(RunDownload(), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
RadarDownloadGlue::RunDownload()
{
  path = co_await OPERA::DownloadRadar(bounds, width, height, curl,
                                       BackgroundDownloadProgress::Get());
}

void
RadarDownloadGlue::OnCompletion(std::exception_ptr error) noexcept
{
  completion_error = std::move(error);
  complete_notify.SendNotification();
}

namespace {

/**
 * The overlay we installed, so that we never remove one somebody else
 * put there.
 */
const MapOverlay *installed_overlay = nullptr;

/**
 * What #installed_overlay shows: the composite it came from, and the
 * area it was fetched for.
 */
std::string installed_url;
GeoBounds installed_bounds = GeoBounds::Invalid();

/** the time #installed_overlay depicts */
BrokenDateTime installed_time = BrokenDateTime::Invalid();

/** the composite the running download is fetching */
std::string pending_url;
BrokenDateTime pending_time = BrokenDateTime::Invalid();

/**
 * Set when a frame was taken off the map for age and the refresh that
 * should replace it has not succeeded yet.  If that refresh fails,
 * the pilot is told the radar is gone rather than left wondering.
 */
bool stale_removed = false;

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
     the choice to the profile, and the Network configuration page can
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
OverlayAge() noexcept
{
  const auto now = BrokenDateTime::NowUTC();
  if (installed_overlay == nullptr ||
      !installed_time.IsPlausible() || !now.IsPlausible())
    return std::chrono::system_clock::duration{-1};

  return now - installed_time;
}

void
InstallOverlay(Path path, const GeoBounds &bounds) noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr || path == nullptr || !bounds.IsValid())
    return;

  Bitmap bitmap;
  try {
    if (!bitmap.LoadFile(path))
      return;
  } catch (...) {
    LogError(std::current_exception(), "Radar overlay");
    return;
  }

  auto bmp = std::make_unique<MapOverlayBitmap>(std::move(bitmap),
                                                GeoQuadrilateral{
                                                  bounds.GetNorthWest(),
                                                  bounds.GetNorthEast(),
                                                  bounds.GetSouthWest(),
                                                  bounds.GetSouthEast(),
                                                },
                                                _("Radar"));
  bmp->SetAlpha(0.6);
  installed_overlay = bmp.get();
  installed_bounds = bounds;
  installed_url = pending_url;
  installed_time = pending_time;
  stale_removed = false;
  map->SetOverlay(std::move(bmp));
}

/**
 * Is the overlay we already installed still the right one?  The
 * composite is published every five minutes, so re-fetching the same
 * one costs bandwidth and shows nothing new; and an image fetched for
 * a wider area still covers a map that has not been panned out of it.
 */
bool
IsStillCurrent(const char *url, const GeoBounds &bounds) noexcept
{
  const auto *map = UIGlobals::GetMap();
  return map != nullptr && installed_overlay != nullptr &&
    map->GetOverlay() == installed_overlay &&
    installed_url == url &&
    installed_bounds.IsValid() && installed_bounds.IsInside(bounds);
}

} // anonymous namespace

void
RadarDownloadGlue::OnCompleteNotify() noexcept
{
  if (task.IsShuttingDown())
    return;

  if (!task.IsRunning())
    BackgroundDownloadProgress::Get().End();

  if (completion_error) {
    LogError(std::exchange(completion_error, {}), "Radar download");

    /* a lost request on its own is not worth a dialog; it only
       matters once it means the map has no radar left to show */
    if (std::exchange(stale_removed, false))
      WarnStale();

    return;
  }

  InstallOverlay(path, bounds);
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
  const auto age = OverlayAge();
  if (age < std::chrono::system_clock::duration::zero())
    /* nothing on the map to keep fresh */
    return;

  if (age > std::chrono::minutes{OPERA::MAX_AGE_MINUTES}) {
    /* down it comes before anything is fetched: an echo this old is
       worse than none, and a refresh that hangs or fails must not be
       able to leave it standing */
    OPERA::ClearMapOverlay();
    stale_removed = true;
  }

  /* ActivatePageOverlay() is a no-op while the frame we hold is still
     the newest one, so this only fetches when there is something new */
  OPERA::ActivatePageOverlay();
}

void
OPERA::ActivatePageOverlay() noexcept
{
  auto *glue = GetRadarDownloadGlue();
  const auto *map = UIGlobals::GetMap();
  if (glue == nullptr || map == nullptr)
    return;

  /* arm the watchdog first: it has to keep running even on the turns
     where there is nothing new to fetch, or the picture would never
     age out */
  glue->ScheduleAgeCheck();

  const auto &projection = map->VisibleProjection();
  if (!projection.IsValid())
    return;

  const auto &bounds = projection.GetScreenBounds();
  const auto now = BrokenDateTime::NowUTC();
  const auto url = OPERA::MakeCompositeURL(now);
  if (url.empty() || IsStillCurrent(url.c_str(), bounds))
    return;

  const auto size = projection.GetScreenSize();
  pending_url = url;
  pending_time = OPERA::CompositeTime(now);
  glue->Start(bounds, size.width, size.height);
}

void
OPERA::ClearMapOverlay() noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr || installed_overlay == nullptr)
    return;

  if (map->GetOverlay() == installed_overlay)
    map->SetOverlay(nullptr);

  installed_overlay = nullptr;
  installed_bounds = GeoBounds::Invalid();
  installed_time = BrokenDateTime::Invalid();
  installed_url.clear();
}

void
OPERA::DeactivatePageOverlay() noexcept
{
  if (auto *glue = GetRadarDownloadGlue(); glue != nullptr)
    glue->CancelAgeCheck();

  stale_removed = false;
  ClearMapOverlay();
}
