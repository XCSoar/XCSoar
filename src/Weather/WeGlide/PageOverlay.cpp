// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageOverlay.hpp"

#include "Dialogs/Settings/WeGlideDeviceSignIn.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "MapWindow/OverlayLimits.hpp"
#include "Message.hpp"
#include "UIGlobals.hpp"
#include "io/FileOutputStream.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Global.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "co/Task.hxx"
#include "net/client/WeGlide/Auth.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "net/client/auth/DeviceFlow.hpp"
#include "net/http/Init.hpp"
#include "system/FileUtil.hpp"
#include "util/Exception.hxx"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "MapWindow/Overlay.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/custom/GeoBitmap.hpp"
#include "ui/dim/Size.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"

#include <array>
#include <cstdio>

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <vector>

using std::string_view_literals::operator""sv;

namespace {

constexpr unsigned TILE_SLOTS = 24;
constexpr unsigned LEGEND_SLOT = 24;
constexpr float TILE_ALPHA = 0.55f;

static_assert(LEGEND_SLOT < MapWindowOverlay::MAX_MAP_OVERLAYS);

WeGlideDownloadGlue *glue_pointer = nullptr;

bool active = false;
bool suspended = false;
bool catalog_valid = false;
bool query_valid = false;
unsigned failures = 0;
bool renew_attempted = false;
uint32_t wait_until = 0;
uint32_t latest_fetched_at = 0;

std::string selected{"thermal"};
int64_t requested_time = 0;
WeGlide::TileQuery query;
std::vector<WeGlide::LayerInfo> catalog;
std::vector<WeGlide::LayerTick> latest;

struct Shown {
  bool used = false;
  std::string layer;
  uint32_t time = 0;
  GeoBitmap::TileData tile{};
};

std::array<Shown, TILE_SLOTS> shown{};

struct Requested {
  std::string layer;
  uint32_t time = 0;
  GeoBitmap::TileData tile{};
};

Requested requested;

enum class Notice : uint8_t {
  NONE,
  DISABLED,
  SIGNED_OUT,
  UNAVAILABLE,
};

Notice notice = Notice::NONE;

uint32_t
UnixNow() noexcept
{
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  if (seconds <= 0)
    return 0;
  if (static_cast<uint64_t>(seconds) > 0xffffffffu)
    return 0xffffffffu;
  return static_cast<uint32_t>(seconds);
}

void
Note(Notice next, const char *text) noexcept
{
  if (notice == next)
    return;
  notice = next;
  if (text != nullptr)
    Message::AddMessage(text);
}

bool
IsPng(std::string_view body) noexcept
{
  static constexpr unsigned char sig[] = {
    137, 80, 78, 71, 13, 10, 26, 10,
  };
  return body.size() >= sizeof(sig) &&
         std::memcmp(body.data(), sig, sizeof(sig)) == 0;
}

std::string
Bearer(const WeGlide::AccountToken &account)
{
  return std::string{"Authorization: Bearer "} + account.access_token;
}

const char *
KindName(WeGlideDownloadGlue::Kind kind) noexcept
{
  switch (kind) {
  case WeGlideDownloadGlue::Kind::REFRESH:
    return "refresh";
  case WeGlideDownloadGlue::Kind::TOKEN:
    return "token";
  case WeGlideDownloadGlue::Kind::CATALOG:
    return "catalog";
  case WeGlideDownloadGlue::Kind::LATEST:
    return "latest";
  case WeGlideDownloadGlue::Kind::TILE:
    return "tile";
  }
  return "?";
}

const char *
RefreshName(WeGlide::RefreshOutcome outcome) noexcept
{
  switch (outcome) {
  case WeGlide::RefreshOutcome::UNCHANGED:
    return "unchanged";
  case WeGlide::RefreshOutcome::UPDATED:
    return "updated";
  case WeGlide::RefreshOutcome::FAILED:
    return "failed";
  }
  return "?";
}

/** Drop the query string. It carries the tile token. */
std::string_view
UrlPath(std::string_view url) noexcept
{
  const auto cut = url.find('?');
  return cut == url.npos ? url : url.substr(0, cut);
}

std::string
SafeError(const std::exception_ptr &error)
{
  if (!error)
    return {};
  auto text = GetFullMessage(error);
  const auto cut = text.find('?');
  if (cut != text.npos)
    text.resize(cut);
  return text;
}

/** Log a repeated idle reason once, until a request starts. */
void
LogState(const char *reason) noexcept
{
  static const char *previous = nullptr;
  if (reason == nullptr) {
    previous = nullptr;
    return;
  }
  if (previous == reason)
    return;
  previous = reason;
  LogDebug("WeGlide: {}", reason);
}

std::string
CacheName(std::string_view layer, uint32_t time,
          const GeoBitmap::TileData &tile)
{
  return fmt::format("{}-{}-{}-{}-{}.png",
                     layer, time, tile.zoom, tile.x, tile.y);
}

bool
ParseCacheName(std::string_view name, WeGlide::StoredTile &tile) noexcept
{
  if (!name.ends_with(".png"))
    return false;
  name.remove_suffix(4);

  const auto first = name.find('-');
  if (first == name.npos || first == 0)
    return false;

  tile.layer_id = std::string{name.substr(0, first)};
  unsigned long time = 0;
  unsigned zoom = 0, x = 0, y = 0;
  const auto rest = name.substr(first + 1);
  if (sscanf(std::string{rest}.c_str(), "%lu-%u-%u-%u",
             &time, &zoom, &x, &y) != 4)
    return false;
  if (time == 0 || time > 0xfffffffful || zoom == 0 ||
      zoom > GeoBitmap::MAX_TILE_ZOOM ||
      x >= (1u << zoom) || y >= (1u << zoom))
    return false;
  tile.time = static_cast<uint32_t>(time);
  return true;
}

void
ClearTiles() noexcept
{
  auto *map = UIGlobals::GetMapIfActive();
  for (unsigned i = 0; i < TILE_SLOTS; ++i) {
    shown[i] = {};
    if (map != nullptr)
      map->SetOverlay(i, nullptr);
  }
}

void
ClearLegend() noexcept
{
  if (auto *map = UIGlobals::GetMapIfActive(); map != nullptr)
    map->SetOverlay(LEGEND_SLOT, nullptr);
}

const WeGlide::LayerTick *
TickFor(std::string_view layer_id) noexcept
{
  for (const auto &tick : latest)
    if (tick.layer_id == layer_id && WeGlide::IsUsableTick(tick))
      return &tick;
  return nullptr;
}

uint32_t
FrameFor(const WeGlide::LayerTick &tick, uint32_t now) noexcept
{
  if (requested_time <= 0 ||
      uint64_t(requested_time) > 0xffffffffu)
    return WeGlide::CurrentTickTime(tick, now);
  return WeGlide::AlignTickTime(tick, uint32_t(requested_time), now);
}

const WeGlide::LayerInfo *
InfoFor(std::string_view layer_id) noexcept
{
  for (const auto &layer : catalog)
    if (layer.id == layer_id)
      return &layer;
  return nullptr;
}

class LegendOverlay final : public MapOverlay {
  std::vector<WeGlide::LegendStop> stops;
  std::string label;

public:
  LegendOverlay(std::vector<WeGlide::LegendStop> _stops,
                std::string _label) noexcept
    :stops(std::move(_stops)), label(std::move(_label)) {}

  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint) const noexcept override {
    return false;
  }

  void Draw(Canvas &canvas, const WindowProjection &) noexcept override {
    if (stops.empty())
      return;

    const int width = Layout::Scale(12);
    const int height = Layout::Scale(140);
    const int left = Layout::Scale(6);
    const int bottom = int(canvas.GetSize().height) - Layout::Scale(8);
    const int top = bottom - height;
    canvas.DrawFilledRectangle({left - 1, top - 1, left + width + 1,
                                bottom + 1},
                               COLOR_BLACK);

    const int count = int(stops.size());
    for (int i = 0; i < count; ++i) {
      const auto &stop = stops[size_t(count - 1 - i)];
      const int y0 = top + height * i / count;
      const int y1 = top + height * (i + 1) / count;
      canvas.DrawFilledRectangle(
        {left, y0, left + width, std::max(y1, y0 + 1)},
        Color(stop.color.r, stop.color.g, stop.color.b));
    }
  }
};

void
InstallLegend() noexcept
{
  auto *map = UIGlobals::GetMapIfActive();
  if (map == nullptr)
    return;

  const auto *info = InfoFor(selected);
  if (info == nullptr || info->legend.empty()) {
    map->SetOverlay(LEGEND_SLOT, nullptr);
    return;
  }

  map->SetOverlay(LEGEND_SLOT,
                  std::make_unique<LegendOverlay>(info->legend, info->name));
}

bool
IsShown(const GeoBitmap::TileData &tile, uint32_t time) noexcept
{
  for (const auto &slot : shown)
    if (slot.used && slot.layer == selected && slot.time == time &&
        slot.tile.zoom == tile.zoom && slot.tile.x == tile.x &&
        slot.tile.y == tile.y)
      return true;
  return false;
}

bool
InstallTile(Path path, const Requested &item) noexcept
{
  auto *map = UIGlobals::GetMapIfActive();
  if (map == nullptr || !active)
    return true;

  int slot = -1;
  for (unsigned i = 0; i < TILE_SLOTS; ++i) {
    if (!shown[i].used) {
      slot = int(i);
      break;
    }
  }
  if (slot < 0)
    return true;

  Bitmap bitmap;
  if (!bitmap.LoadFile(path))
    return false;

  auto overlay = std::make_unique<MapOverlayBitmap>(
    std::move(bitmap), GeoBitmap::GetGeoQuadrilateral(item.tile), "");
  overlay->SetAlpha(TILE_ALPHA);
  map->SetOverlay(unsigned(slot), std::move(overlay));
  shown[slot].used = true;
  shown[slot].layer = item.layer;
  shown[slot].time = item.time;
  shown[slot].tile = item.tile;
  return true;
}

struct Candidate {
  GeoBitmap::TileData tile;
  unsigned distance = 0;
};

std::vector<Candidate>
TilesInView(unsigned zoom, GeoPoint aircraft) noexcept
{
  std::vector<Candidate> tiles;
  auto *map = UIGlobals::GetMapIfActive();
  if (map == nullptr || zoom == 0 || zoom >= 31)
    return tiles;

  const auto &projection = map->VisibleProjection();
  GeoBitmap::TileData north_west{};
  GeoBitmap::TileData south_east{};
  if (projection.IsValid()) {
    const auto bounds = projection.GetScreenBounds();
    if (bounds.IsValid()) {
      north_west = GeoBitmap::GetTile(GeoBounds{bounds.GetNorthWest()},
                                      uint16_t(zoom));
      south_east = GeoBitmap::GetTile(GeoBounds{bounds.GetSouthEast()},
                                      uint16_t(zoom));
    }
  }

  if (!north_west.IsValid() || !south_east.IsValid()) {
    if (!aircraft.IsValid())
      return tiles;
    north_west = south_east =
      GeoBitmap::GetTile(GeoBounds{aircraft}, uint16_t(zoom));
  }

  const auto aircraft_tile = aircraft.IsValid()
    ? GeoBitmap::GetTile(GeoBounds{aircraft}, uint16_t(zoom))
    : north_west;
  const int span = 1 << zoom;

  const auto wrap = [span](int value) {
    int result = value % span;
    if (result < 0)
      result += span;
    return result;
  };

  const int x0 = int(north_west.x);
  const int x1 = int(south_east.x);
  const int y0 = int(std::min(north_west.y, south_east.y));
  const int y1 = int(std::max(north_west.y, south_east.y));

  std::vector<int> columns;
  if (x0 <= x1) {
    for (int x = x0; x <= x1; ++x)
      columns.push_back(wrap(x));
  } else {
    for (int x = x0; x < span; ++x)
      columns.push_back(x);
    for (int x = 0; x <= x1; ++x)
      columns.push_back(x);
  }

  for (int y = y0; y <= y1; ++y) {
    if (y < 0 || y >= span)
      continue;
    for (const int x : columns) {
      int dx = x - int(aircraft_tile.x);
      if (dx > span / 2)
        dx -= span;
      if (dx < -span / 2)
        dx += span;
      const int dy = y - int(aircraft_tile.y);
      tiles.push_back({
        {uint16_t(zoom), uint32_t(x), uint32_t(y)},
        unsigned(dx * dx + dy * dy),
      });
    }
  }

  std::sort(tiles.begin(), tiles.end(),
            [](const Candidate &a, const Candidate &b) {
              return a.distance < b.distance;
            });
  if (tiles.size() > TILE_SLOTS)
    tiles.resize(TILE_SLOTS);
  return tiles;
}

void
PruneCache() noexcept
{
  const auto cache = MakeCacheDirectory("weglide");
  if (cache == nullptr)
    return;

  struct Item {
    WeGlide::StoredTile tile;
    AllocatedPath path;
  };
  std::vector<Item> items;

  struct Visitor final : File::Visitor {
    std::vector<Item> &items;
    explicit Visitor(std::vector<Item> &_items) :items(_items) {}

    void Visit(Path path, Path filename) override {
      WeGlide::StoredTile tile;
      if (!ParseCacheName(filename.c_str(), tile))
        return;
      tile.bytes = File::GetSize(path);
      const auto modified = File::GetLastModification(path);
      tile.mtime = std::chrono::duration_cast<std::chrono::seconds>(
        modified.time_since_epoch()).count();
      items.push_back({std::move(tile), AllocatedPath{path}});
    }
  } visitor{items};

  Directory::VisitFiles(cache, visitor);

  std::vector<WeGlide::StoredTile> stored;
  stored.reserve(items.size());
  for (const auto &item : items)
    stored.push_back(item.tile);

  for (const std::size_t index :
       WeGlide::TilesToDrop(stored, latest, UnixNow(),
                            WeGlide::kTileCacheCapBytes)) {
    if (index < items.size())
      File::Delete(items[index].path);
  }
}

AllocatedPath
TileFile(std::string_view layer, uint32_t time,
         const GeoBitmap::TileData &tile)
{
  const auto cache = MakeCacheDirectory("weglide");
  if (cache == nullptr)
    return nullptr;
  return AllocatedPath::Build(cache, CacheName(layer, time, tile).c_str());
}

} // namespace

WeGlideDownloadGlue *
GetWeGlideDownloadGlue() noexcept
{
  return glue_pointer;
}

WeGlideDownloadGlue::WeGlideDownloadGlue(CurlGlobal &_curl) noexcept
  :curl(_curl), task(_curl.GetEventLoop())
{
  glue_pointer = this;
}

void
WeGlideDownloadGlue::BeginShutdown() noexcept
{
  timer.Cancel();
  task.BeginShutdown();
  if (glue_pointer == this)
    glue_pointer = nullptr;
}

void
WeGlideDownloadGlue::Cancel() noexcept
{
  timer.Cancel();
  task.Cancel();
}

void
WeGlideDownloadGlue::Schedule(std::chrono::seconds delay) noexcept
{
  if (delay.count() < 1)
    delay = std::chrono::seconds{1};
  timer.Schedule(delay);
}

void
WeGlideDownloadGlue::StartRefresh(WeGlide::AccountToken _account) noexcept
{
  if (IsRunning())
    return;
  kind = Kind::REFRESH;
  account = std::move(_account);
  url.clear();
  form.clear();
  authorization.clear();
  path = nullptr;
  status = 0;
  body.clear();
  error = nullptr;
  wrote_file = false;
  refresh_outcome = WeGlide::RefreshOutcome::UNCHANGED;
  LogState(nullptr);
  LogDebug("WeGlide: refresh access_len={}",
           account.access_token.size());
  task.Start(Run(), BIND_THIS_METHOD(OnCompletion));
}

void
WeGlideDownloadGlue::StartGet(Kind _kind, std::string _url,
                              std::string _authorization,
                              AllocatedPath _path) noexcept
{
  if (IsRunning())
    return;
  kind = _kind;
  url = std::move(_url);
  authorization = std::move(_authorization);
  path = std::move(_path);
  form.clear();
  status = 0;
  body.clear();
  error = nullptr;
  wrote_file = false;
  LogState(nullptr);
  LogDebug("WeGlide: get {} {}", KindName(kind), UrlPath(url));
  task.Start(Run(), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
WeGlideDownloadGlue::Run()
{
  try {
    if (kind == Kind::REFRESH) {
      auto refresh = WeGlide::RefreshAccount(curl, account);
      refresh_outcome = co_await refresh;
      co_return;
    }

    CurlEasy easy{url.c_str()};
    Curl::Setup(easy);
    easy.SetTimeout(30);

    CurlSlist headers;
    if (!authorization.empty()) {
      headers.Append(authorization.c_str());
      easy.SetRequestHeaders(headers.Get());
    }

    /* A temporary CoRequest is destroyed when this coroutine
       suspends, so the transfer never completes. */
    Curl::CoRequest request{curl, std::move(easy)};
    auto response = co_await request;
    status = response.status;
    if (kind == Kind::TILE && response.status == 200 && IsPng(response.body) &&
        path != nullptr) {
      FileOutputStream out{path};
      const auto bytes = std::as_bytes(std::span{response.body.data(),
                                                 response.body.size()});
      out.Write(bytes);
      out.Commit();
      wrote_file = true;
    } else {
      body = std::move(response.body);
    }
  } catch (...) {
    error = std::current_exception();
  }
}

void
WeGlideDownloadGlue::OnCompletion(std::exception_ptr _error) noexcept
{
  if (_error)
    error = std::move(_error);
  complete_notify.SendNotification();
}

void
WeGlideDownloadGlue::OnCompleteNotify() noexcept
{
  const auto bytes = wrote_file && path != nullptr
    ? File::GetSize(path) : body.size();
  LogDebug("WeGlide: {} done status={} bytes={} file={} error={}",
           KindName(kind), status, bytes, wrote_file, SafeError(error));

  if (kind == Kind::REFRESH) {
    LogDebug("WeGlide: refresh {}", RefreshName(refresh_outcome));
    WeGlide::SaveAccountToken(account);
    if (refresh_outcome == WeGlide::RefreshOutcome::FAILED &&
        account.access_token.empty())
      Note(Notice::SIGNED_OUT, _("Not connected"));
  } else if (error) {
    if (WeGlide::AllowTileRetry(failures))
      failures = 1;
    else if (kind == Kind::TOKEN)
      wait_until = UnixNow() + 60;
    else if (!renew_attempted) {
      query_valid = false;
      renew_attempted = true;
      failures = 0;
    } else {
      wait_until = WeGlide::SubscriptionWaitUntil(UnixNow());
    }
  } else if (kind == Kind::TOKEN) {
    if (status == 403) {
      if (WeGlide::AllowTileRetry(failures)) {
        failures = 1;
      } else {
        failures = 0;
        wait_until = WeGlide::SubscriptionWaitUntil(UnixNow());
        Note(Notice::UNAVAILABLE, _("Unavailable"));
      }
    } else if (auto parsed = WeGlide::ParseTileQuery(body)) {
      query = std::move(*parsed);
      query_valid = true;
      catalog_valid = false;
      failures = 0;
      renew_attempted = false;
      notice = Notice::NONE;
      LogDebug("WeGlide: token expires={}", query.expires);
    } else if (WeGlide::AllowTileRetry(failures)) {
      failures = 1;
    } else {
      wait_until = UnixNow() + 60;
      failures = 0;
    }
  } else if (status == 403 || status == 0) {
    if (WeGlide::AllowTileRetry(failures))
      failures = 1;
    else if (!renew_attempted) {
      query_valid = false;
      renew_attempted = true;
      failures = 0;
    } else {
      wait_until = WeGlide::SubscriptionWaitUntil(UnixNow());
    }
  } else if (kind == Kind::CATALOG) {
    if (auto parsed = WeGlide::ParseCatalog(body)) {
      catalog = std::move(*parsed);
      catalog_valid = true;
      failures = 0;
      if (InfoFor(selected) == nullptr && !catalog.empty())
        selected = catalog.front().id;
      LogDebug("WeGlide: catalog layers={} selected={}",
               catalog.size(), selected);
      InstallLegend();
    }
  } else if (kind == Kind::LATEST) {
    if (auto parsed = WeGlide::ParseLatest(body)) {
      const uint32_t now = UnixNow();
      const auto *previous = TickFor(selected);
      const uint32_t previous_time = previous != nullptr
        ? FrameFor(*previous, now) : 0;
      latest = std::move(*parsed);
      latest_fetched_at = now;
      failures = 0;
      const auto *next = TickFor(selected);
      const uint32_t next_time = next != nullptr
        ? FrameFor(*next, now) : 0;
      if (next != nullptr && next_time != previous_time)
        ClearTiles();
      for (const auto &tick : latest)
        LogDebug("WeGlide: latest {} origin={} step={} zoom={} frame={}",
                 tick.layer_id, tick.time, tick.interval_seconds,
                 tick.max_zoom, WeGlide::CurrentTickTime(tick, now));
      PruneCache();
    }
  } else if (kind == Kind::TILE && wrote_file) {
    failures = 0;
    InstallTile(path, requested);
    PruneCache();
  } else if (kind == Kind::TILE) {
    Schedule(std::chrono::seconds{5});
    return;
  }

  Poll();
}

void
WeGlideDownloadGlue::Poll() noexcept
{
  if (!active || suspended || IsRunning())
    return;

  const auto &settings = CommonInterface::GetComputerSettings().weglide;
  if (!settings.enabled) {
    Note(Notice::DISABLED, _("Disabled"));
    LogState("disabled");
    Schedule(std::chrono::seconds{5});
    return;
  }

  const uint32_t now = UnixNow();
  if (wait_until != 0 && now < wait_until) {
    LogDebug("WeGlide: waiting {}s", wait_until - now);
    Schedule(std::chrono::seconds{std::min<uint32_t>(wait_until - now, 30)});
    return;
  }
  wait_until = 0;

  auto account_now = WeGlide::LoadAccountToken();
  Net::Client::Auth::DeviceFlow flow;
  flow.token.access_token = account_now.access_token;
  flow.token.refresh_token = account_now.refresh_token;
  flow.access_expires_at = account_now.access_expires_at;
  const auto account_action =
    flow.Next(now, WeGlide::kAccessRefreshMargin);

  using Net::Client::Auth::DeviceFlowAction;
  if (account_action == DeviceFlowAction::REFRESH) {
    if (IsWeGlideSignInOpen()) {
      LogState("sign-in open");
      Schedule(std::chrono::seconds{5});
      return;
    }
    StartRefresh(std::move(account_now));
    return;
  }
  if (account_now.access_token.empty()) {
    Note(Notice::SIGNED_OUT, _("Not connected"));
    LogState("not connected");
    Schedule(std::chrono::seconds{15});
    return;
  }
  if (notice == Notice::SIGNED_OUT || notice == Notice::DISABLED)
    notice = Notice::NONE;

  const auto query_action =
    WeGlide::NextTileQuery(now, query_valid ? query.expires : 0, 0);
  if (!query_valid || query_action == WeGlide::TileQueryAction::RENEW) {
    StartGet(Kind::TOKEN,
             std::string{WeGlideSettings::default_url} +
             "/copilot/tiles/token",
             Bearer(account_now));
    return;
  }

  if (!catalog_valid) {
    StartGet(Kind::CATALOG, WeGlide::GridUrl(query, "catalog"), {});
    return;
  }

  const unsigned interval = WeGlide::SmallestInterval(latest);
  const bool latest_due = latest_fetched_at == 0 || interval == 0 ||
    now >= latest_fetched_at + interval;
  if (latest_due) {
    StartGet(Kind::LATEST, WeGlide::GridUrl(query, "latest"), {});
    return;
  }

  const auto *tick = TickFor(selected);
  if (tick == nullptr) {
    LogState("no tick");
    Schedule(std::chrono::seconds{interval == 0 ? 20 : interval});
    return;
  }

  const uint32_t frame = FrameFor(*tick, now);
  const auto &basic = CommonInterface::Basic();
  unsigned zoom = tick->max_zoom;
  if (auto *map = UIGlobals::GetMapIfActive();
      map != nullptr && map->VisibleProjection().IsValid()) {
    const auto picked = GeoBitmap::GetTile(map->VisibleProjection(), 1,
                                           uint16_t(std::min(tick->max_zoom,
                                                             20u)));
    if (picked.IsValid())
      zoom = picked.zoom;
  }

  static uint32_t logged_frame = 0;
  static unsigned logged_zoom = 0;
  if (frame != logged_frame || zoom != logged_zoom) {
    logged_frame = frame;
    logged_zoom = zoom;
    LogDebug("WeGlide: draw {} frame={} zoom={} origin={}",
             selected, frame, zoom, tick->time);
  }

  for (const auto &slot : shown) {
    if (slot.used && (slot.tile.zoom != zoom || slot.time != frame ||
                      slot.layer != selected)) {
      ClearTiles();
      break;
    }
  }

  const auto candidates = TilesInView(zoom, basic.location);
  if (candidates.empty())
    LogState("no tiles in view");

  for (const auto &candidate : candidates) {
    if (IsShown(candidate.tile, frame))
      continue;
    auto file = TileFile(selected, frame, candidate.tile);
    if (file == nullptr)
      break;
    if (File::Exists(file)) {
      requested = {selected, frame, candidate.tile};
      if (!InstallTile(file, requested)) {
        LogDebug("WeGlide: cache install failed {}",
                 CacheName(selected, frame, candidate.tile));
        File::Delete(file);
      }
      continue;
    }
    const auto relative = WeGlide::TileRelativePath(selected, frame,
                                                   candidate.tile.zoom,
                                                   candidate.tile.x,
                                                   candidate.tile.y);
    requested = {selected, frame, candidate.tile};
    StartGet(Kind::TILE, WeGlide::GridUrl(query, relative), {},
             std::move(file));
    return;
  }

  LogState("view ready");
  Schedule(std::chrono::seconds{interval == 0 ? 20 : interval});
}

namespace WeGlide {

void
ActivatePageOverlay(std::string_view layer_id, bool first_enter,
                    int64_t frame_time) noexcept
{
  if (layer_id.empty())
    layer_id = "thermal"sv;
  if (frame_time < 0)
    frame_time = 0;

  const bool changed = selected != layer_id;
  const bool time_changed = requested_time != frame_time;
  selected = std::string{layer_id};
  requested_time = frame_time;
  active = true;
  suspended = false;
  if (changed || time_changed)
    ClearTiles();
  if (first_enter)
    catalog_valid = false;

  LogDebug("WeGlide: show layer={} first={}", selected, first_enter);
  if (auto *glue = GetWeGlideDownloadGlue(); glue != nullptr)
    glue->Poll();
  else
    LogDebug("WeGlide: no download glue");
}

void
DeactivatePageOverlay() noexcept
{
  if (suspended)
    return;

  LogDebug("WeGlide: hide");
  active = false;
  requested_time = 0;
  if (auto *glue = GetWeGlideDownloadGlue(); glue != nullptr)
    glue->Cancel();
  ClearTiles();
  ClearLegend();
}

void
SuspendForPan() noexcept
{
  LogDebug("WeGlide: suspend");
  suspended = true;
  if (auto *glue = GetWeGlideDownloadGlue(); glue != nullptr)
    glue->Cancel();
}

void
ResumeAfterPan() noexcept
{
  LogDebug("WeGlide: resume");
  suspended = false;
}

void
OnProjectionModified() noexcept
{
  if (!active || suspended)
    return;
  if (auto *glue = GetWeGlideDownloadGlue(); glue != nullptr)
    glue->Poll();
}

void
SelectLayer(std::string_view layer_id) noexcept
{
  ActivatePageOverlay(layer_id, false, requested_time);
}

void
SetFrameTime(int64_t frame_time) noexcept
{
  if (frame_time < 0)
    frame_time = 0;
  if (requested_time == frame_time)
    return;

  const auto *tick = TickFor(selected);
  const uint32_t now = UnixNow();
  const uint32_t before = tick != nullptr ? FrameFor(*tick, now) : 0;
  requested_time = frame_time;
  const uint32_t after = tick != nullptr ? FrameFor(*tick, now) : 0;
  LogDebug("WeGlide: frame request {} shows {}", frame_time, after);
  if (before != after && active)
    ClearTiles();
  if (auto *glue = GetWeGlideDownloadGlue(); glue != nullptr)
    glue->Poll();
}

std::string_view
SelectedLayer() noexcept
{
  return selected;
}

const LayerTick *
SelectedTick() noexcept
{
  return TickFor(selected);
}

const std::vector<LayerInfo> &
Catalog() noexcept
{
  return catalog;
}

} // namespace WeGlide
