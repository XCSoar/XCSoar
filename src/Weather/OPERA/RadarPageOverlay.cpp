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
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "lib/curl/Global.hxx"
#include "time/BrokenDateTime.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "util/BindMethod.hxx"

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

/** the composite the running download is fetching */
std::string pending_url;

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
    return;
  }

  InstallOverlay(path, bounds);
}

void
OPERA::ActivatePageOverlay() noexcept
{
  auto *glue = GetRadarDownloadGlue();
  const auto *map = UIGlobals::GetMap();
  if (glue == nullptr || map == nullptr)
    return;

  const auto &projection = map->VisibleProjection();
  if (!projection.IsValid())
    return;

  const auto &bounds = projection.GetScreenBounds();
  const auto url = OPERA::MakeCompositeURL(BrokenDateTime::NowUTC());
  if (url.empty() || IsStillCurrent(url.c_str(), bounds))
    return;

  const auto size = projection.GetScreenSize();
  pending_url = url;
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
  installed_url.clear();
}
