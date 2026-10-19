// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageOverlay.hpp"

#include "Asset.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "Look/Look.hpp"
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
#include <cmath>
#include <cstring>
#include <exception>
#include <vector>

using std::string_view_literals::operator""sv;

namespace {

constexpr unsigned TILE_SLOTS = 24;

/** A frame older than this is taken down. Same bound as the radar. */
constexpr uint32_t kMaxShownAgeSeconds = 20 * 60;
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

/** Squares the current view wants. An old frame stays until replaced. */
struct Square {
  uint16_t zoom = 0;
  uint32_t x = 0;
  uint32_t y = 0;
};

std::array<Square, TILE_SLOTS> view{};
unsigned view_count = 0;

bool
InView(const GeoBitmap::TileData &tile) noexcept
{
  for (unsigned i = 0; i < view_count; ++i)
    if (view[i].zoom == tile.zoom && view[i].x == tile.x &&
        view[i].y == tile.y)
      return true;
  return false;
}

struct Requested {
  std::string layer;
  uint32_t time = 0;
  GeoBitmap::TileData tile{};
};

Requested requested;

/** A tile that did not come back as a PNG. Skip it until retry_at. */
struct TileSkip {
  std::string layer;
  uint32_t time = 0;
  GeoBitmap::TileData tile{};
  uint32_t retry_at = 0;
};

std::array<TileSkip, TILE_SLOTS> skipped{};
unsigned skipped_count = 0;

bool
TileCooling(const GeoBitmap::TileData &tile, uint32_t time,
            uint32_t now) noexcept
{
  for (unsigned i = 0; i < skipped_count; ++i) {
    const auto &skip = skipped[i];
    if (skip.layer == selected && skip.time == time &&
        skip.tile.zoom == tile.zoom && skip.tile.x == tile.x &&
        skip.tile.y == tile.y && now < skip.retry_at)
      return true;
  }
  return false;
}

void
CoolTile(const Requested &item, uint32_t now) noexcept
{
  for (unsigned i = 0; i < skipped_count; ++i) {
    auto &skip = skipped[i];
    if (skip.layer == item.layer && skip.time == item.time &&
        skip.tile.zoom == item.tile.zoom && skip.tile.x == item.tile.x &&
        skip.tile.y == item.tile.y) {
      skip.retry_at = now + 30;
      return;
    }
  }

  unsigned slot = skipped_count;
  if (slot == TILE_SLOTS) {
    slot = 0;
    for (unsigned i = 1; i < TILE_SLOTS; ++i)
      if (skipped[i].retry_at < skipped[slot].retry_at)
        slot = i;
  } else {
    ++skipped_count;
  }
  skipped[slot] = {item.layer, item.time, item.tile, now + 30};
}

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
  if (text == nullptr)
    return;
  try {
    const auto message = fmt::format("{}: {}", _("WeGlide"), text);
    Message::AddMessage(message.c_str());
  } catch (...) {
    LogError(std::current_exception(), "WeGlide overlay");
  }
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

  /* Time, zoom, x and y are the last four fields. The layer id may
     itself contain hyphens. */
  const auto y_at = name.rfind('-');
  if (y_at == name.npos || y_at == 0)
    return false;
  const auto x_at = name.rfind('-', y_at - 1);
  if (x_at == name.npos || x_at == 0)
    return false;
  const auto zoom_at = name.rfind('-', x_at - 1);
  if (zoom_at == name.npos || zoom_at == 0)
    return false;
  const auto time_at = name.rfind('-', zoom_at - 1);
  if (time_at == name.npos || time_at == 0)
    return false;

  const auto layer = name.substr(0, time_at);
  unsigned long time = 0;
  unsigned zoom = 0, x = 0, y = 0;
  const auto rest = std::string{name.substr(time_at + 1)};
  if (sscanf(rest.c_str(), "%lu-%u-%u-%u", &time, &zoom, &x, &y) != 4)
    return false;
  if (time == 0 || time > 0xfffffffful || zoom == 0 ||
      zoom > GeoBitmap::MAX_TILE_ZOOM ||
      x >= (1u << zoom) || y >= (1u << zoom))
    return false;
  tile.layer_id = std::string{layer};
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

std::string
StopLabel(double value, std::string_view units)
{
  const auto number = std::fabs(value - std::round(value)) < 0.05
    ? fmt::format("{:.0f}", value)
    : fmt::format("{:.1f}", value);
  if (units.empty())
    return number;
  return fmt::format("{} {}", number, units);
}

class LegendOverlay final : public MapOverlay {
  std::vector<WeGlide::LegendStop> stops;
  std::string label;
  std::string units;

public:
  LegendOverlay(std::vector<WeGlide::LegendStop> _stops,
                std::string _label, std::string _units) noexcept
    :stops(std::move(_stops)), label(std::move(_label)),
     units(std::move(_units)) {}

  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint) const noexcept override {
    return false;
  }

  void Draw(Canvas &canvas, const WindowProjection &) noexcept override {
    auto *map = UIGlobals::GetMapIfActive();
    if (map == nullptr || stops.empty())
      return;

    /* Above the scale, below the thermal band, inside the HUD. */
    const auto layout = map->GetHudLayout();
    const int gap = Layout::Scale(4);
    const int width = Layout::Scale(12);
    const int left = layout.bottom.left;
    const int bottom = layout.bottom.bottom -
      int(layout.scale_title_clearance) - gap;
    const int top_limit = layout.content.top +
      layout.content.GetHeight() / 5 + gap;
    int top = bottom - Layout::Scale(140);
    if (top < top_limit)
      top = top_limit;
    const int height = bottom - top;
    if (height < Layout::Scale(24))
      return;

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

    const auto *font = UIGlobals::GetLook().map.overlay.overlay_font;
    if (font == nullptr)
      return;

    const auto ends = std::minmax_element(
      stops.begin(), stops.end(),
      [](const auto &a, const auto &b) { return a.value < b.value; });
    const auto high = StopLabel(ends.second->value, units);
    const auto low = StopLabel(ends.first->value, units);
    const int text_x = left + width + gap;
    canvas.Select(*font);
    canvas.SetBackgroundTransparent();
    const Color ink = HasColors() ? COLOR_WHITE : COLOR_BLACK;
    const auto paint = [&](PixelPoint at, std::string_view text) {
      if (HasColors()) {
        canvas.SetTextColor(COLOR_BLACK);
        canvas.DrawText({at.x + 1, at.y + 1}, text);
      }
      canvas.SetTextColor(ink);
      canvas.DrawText(at, text);
    };
    paint(PixelPoint(text_x, top), high);
    paint(PixelPoint(text_x, bottom - int(font->GetHeight())), low);
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

  try {
    map->SetOverlay(LEGEND_SLOT,
                    std::make_unique<LegendOverlay>(info->legend, info->name,
                                                    info->units));
  } catch (...) {
    LogError(std::current_exception(), "WeGlide overlay");
  }
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

/**
 * Where to put a tile. The same square is reused, so a newer frame
 * replaces that picture. A square the view has left is reused next,
 * so the old frame is not taken down before the new one is drawn.
 */
int
SlotFor(const Requested &item) noexcept
{
  for (unsigned i = 0; i < TILE_SLOTS; ++i) {
    const auto &slot = shown[i];
    if (slot.used && slot.layer == item.layer &&
        slot.tile.zoom == item.tile.zoom &&
        slot.tile.x == item.tile.x && slot.tile.y == item.tile.y)
      return int(i);
  }

  for (unsigned i = 0; i < TILE_SLOTS; ++i)
    if (shown[i].used && !InView(shown[i].tile))
      return int(i);

  for (unsigned i = 0; i < TILE_SLOTS; ++i)
    if (!shown[i].used)
      return int(i);

  return -1;
}

bool
InstallTile(Path path, const Requested &item) noexcept
{
  auto *map = UIGlobals::GetMapIfActive();
  if (map == nullptr || !active)
    return true;

  const int slot = SlotFor(item);
  /* The file is fine. There is no slot that is not already showing
     the current view. */
  if (slot < 0)
    return true;

  Bitmap bitmap;
  if (!bitmap.LoadFile(path))
    return false;

  /* Allocating under a noexcept caller is std::terminate, not a
     missed tile. */
  try {
    auto overlay = std::make_unique<MapOverlayBitmap>(
      std::move(bitmap), GeoBitmap::GetGeoQuadrilateral(item.tile), "");
    overlay->SetAlpha(TILE_ALPHA);
    map->SetOverlay(unsigned(slot), std::move(overlay));
    shown[slot].layer = item.layer;
    shown[slot].time = item.time;
    shown[slot].tile = item.tile;
    shown[slot].used = true;
  } catch (...) {
    LogError(std::current_exception(), "WeGlide overlay");
    return false;
  }
  return true;
}

struct Candidate {
  GeoBitmap::TileData tile;
  unsigned distance = 0;
};

/** Columns from x0 to x1. When the view crosses the date line, x0 > x1. */
std::vector<int>
TileColumns(int x0, int x1, int span) noexcept
{
  const auto wrap = [span](int value) {
    int result = value % span;
    if (result < 0)
      result += span;
    return result;
  };

  std::vector<int> columns;
  if (x0 <= x1) {
    for (int x = x0; x <= x1; ++x)
      columns.push_back(wrap(x));
    return columns;
  }

  for (int x = x0; x < span; ++x)
    columns.push_back(x);
  for (int x = 0; x <= x1; ++x)
    columns.push_back(x);
  return columns;
}

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
  const int x0 = int(north_west.x);
  const int x1 = int(south_east.x);
  const int y0 = int(std::min(north_west.y, south_east.y));
  const int y1 = int(std::max(north_west.y, south_east.y));
  const auto columns = TileColumns(x0, x1, span);

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
try {
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
} catch (...) {
  LogError(std::current_exception(), "WeGlide overlay");
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
  complete_notify.ClearNotification();
  path = nullptr;
  error = nullptr;
  if (glue_pointer == this)
    glue_pointer = nullptr;
}

void
WeGlideDownloadGlue::Cancel() noexcept
{
  timer.Cancel();
  task.Cancel();
  /* A completion queued before the cancel must not run against the
     next request. */
  complete_notify.ClearNotification();
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
  complete_notify.ClearNotification();
  try {
    kind = Kind::REFRESH;
    refresh_at_start = _account.refresh_token;
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
  } catch (...) {
    LogError(std::current_exception(), "WeGlide overlay");
  }
}

void
WeGlideDownloadGlue::StartGet(Kind _kind, std::string _url,
                              std::string _authorization,
                              AllocatedPath _path) noexcept
{
  if (IsRunning())
    return;
  complete_notify.ClearNotification();
  try {
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
  } catch (...) {
    LogError(std::current_exception(), "WeGlide overlay");
  }
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

/** One immediate retry. Further failures take the longer wait. */
static bool
TakeRetry() noexcept
{
  if (!WeGlide::AllowTileRetry(failures))
    return false;
  failures = 1;
  return true;
}

/** The tile query was rejected. Fetch a new one once, then back off. */
static void
RenewOrWait() noexcept
{
  if (TakeRetry())
    return;

  if (!renew_attempted) {
    query_valid = false;
    renew_attempted = true;
    failures = 0;
    return;
  }

  wait_until = WeGlide::SubscriptionWaitUntil(UnixNow());
}

/** Transport error on the token request. Retry once, then wait a minute. */
static void
WaitMinute() noexcept
{
  if (TakeRetry())
    return;
  wait_until = UnixNow() + 60;
}

/** One more try after a few seconds, then wait a minute. */
static void
HoldOff() noexcept
{
  if (TakeRetry())
    return;
  failures = 0;
  wait_until = UnixNow() + 60;
}

static void
DenySubscription() noexcept
{
  if (TakeRetry())
    return;
  failures = 0;
  wait_until = WeGlide::SubscriptionWaitUntil(UnixNow());
  Note(Notice::UNAVAILABLE, _("Unavailable"));
}

static void
FinishRefresh(const WeGlide::AccountToken &account,
              WeGlide::RefreshOutcome outcome,
              std::string_view refresh_at_start) noexcept
{
  LogDebug("WeGlide: refresh {}", RefreshName(outcome));
  /* Disconnect or a newer sign-in replaced this session. */
  const auto profile = WeGlide::LoadAccountToken().refresh_token;
  if (!WeGlide::ShouldSaveRefreshedAccount(outcome, profile,
                                           refresh_at_start))
    return;
  WeGlide::SaveAccountToken(account);
  if (outcome == WeGlide::RefreshOutcome::FAILED &&
      account.access_token.empty())
    Note(Notice::SIGNED_OUT, _("Not connected"));
}

static void
FinishToken(std::string_view body, unsigned status) noexcept
{
  if (status == 403) {
    DenySubscription();
    return;
  }

  auto parsed = WeGlide::ParseTileQuery(body);
  if (!parsed) {
    HoldOff();
    return;
  }

  query = std::move(*parsed);
  query_valid = true;
  catalog_valid = false;
  failures = 0;
  renew_attempted = false;
  notice = Notice::NONE;
  LogDebug("WeGlide: token expires={}", query.expires);
}

static bool
FinishCatalog(std::string_view body) noexcept
{
  auto parsed = WeGlide::ParseCatalog(body);
  if (!parsed)
    return false;

  catalog = std::move(*parsed);
  catalog_valid = true;
  failures = 0;
  if (InfoFor(selected) == nullptr && !catalog.empty())
    selected = catalog.front().id;
  LogDebug("WeGlide: catalog layers={} selected={}",
           catalog.size(), selected);
  InstallLegend();
  return true;
}

static bool
FinishLatest(std::string_view body) noexcept
{
  auto parsed = WeGlide::ParseLatest(body);
  if (!parsed)
    return false;

  const uint32_t now = UnixNow();
  latest = std::move(*parsed);
  latest_fetched_at = now;
  failures = 0;
  for (const auto &tick : latest)
    LogDebug("WeGlide: latest {} origin={} step={} zoom={} frame={}",
             tick.layer_id, tick.time, tick.interval_seconds,
             tick.max_zoom, WeGlide::CurrentTickTime(tick, now));
  PruneCache();
  return true;
}

void
WeGlideDownloadGlue::OnCompleteNotify() noexcept
try {
  const auto bytes = wrote_file && path != nullptr
    ? File::GetSize(path) : body.size();
  LogDebug("WeGlide: {} done status={} bytes={} file={} error={}",
           KindName(kind), status, bytes, wrote_file, SafeError(error));

  if (kind == Kind::REFRESH) {
    FinishRefresh(account, refresh_outcome, refresh_at_start);
    /* A thrown refresh leaves the outcome unchanged. Wait anyway,
       or the next Poll() starts another POST at once. */
    if (error ||
        refresh_outcome == WeGlide::RefreshOutcome::FAILED) {
      HoldOff();
      Schedule(std::chrono::seconds{5});
      return;
    }
  } else if (error) {
    if (kind == Kind::TOKEN)
      WaitMinute();
    else
      RenewOrWait();
  } else if (kind == Kind::TOKEN) {
    FinishToken(body, status);
  } else if (status == 403 || status == 0) {
    RenewOrWait();
  } else if (kind == Kind::CATALOG) {
    if (!FinishCatalog(body)) {
      HoldOff();
      Schedule(std::chrono::seconds{5});
      return;
    }
  } else if (kind == Kind::LATEST) {
    if (!FinishLatest(body)) {
      HoldOff();
      Schedule(std::chrono::seconds{5});
      return;
    }
  } else if (kind == Kind::TILE && wrote_file) {
    failures = 0;
    InstallTile(path, requested);
  } else if (kind == Kind::TILE) {
    CoolTile(requested, UnixNow());
    Schedule(std::chrono::seconds{5});
    return;
  }

  Poll();
} catch (...) {
  LogError(std::current_exception(), "WeGlide overlay");
}

bool
WeGlideDownloadGlue::RefreshOrSignIn(const WeGlide::AccountToken &account,
                                    uint32_t now) noexcept
{
  Net::Client::Auth::DeviceFlow flow;
  flow.token.access_token = account.access_token;
  flow.token.refresh_token = account.refresh_token;
  flow.access_expires_at = account.access_expires_at;

  using Net::Client::Auth::DeviceFlowAction;
  if (flow.Next(now, WeGlide::kAccessRefreshMargin) ==
      DeviceFlowAction::REFRESH) {
    if (WeGlide::IsSignInOpen()) {
      LogState("sign-in open");
      Schedule(std::chrono::seconds{5});
      return true;
    }
    StartRefresh(account);
    return true;
  }

  if (account.access_token.empty()) {
    Note(Notice::SIGNED_OUT, _("Not connected"));
    LogState("not connected");
    Schedule(std::chrono::seconds{15});
    return true;
  }

  if (notice == Notice::SIGNED_OUT || notice == Notice::DISABLED)
    notice = Notice::NONE;
  return false;
}

static unsigned
ZoomFor(const WeGlide::LayerTick &tick) noexcept
{
  unsigned zoom = tick.max_zoom;
  const auto *map = UIGlobals::GetMapIfActive();
  if (map == nullptr || !map->VisibleProjection().IsValid())
    return zoom;

  const auto limit = uint16_t(std::min(tick.max_zoom, 20u));
  const auto picked = GeoBitmap::GetTile(map->VisibleProjection(), 1, limit);
  return picked.IsValid() ? picked.zoom : zoom;
}

void
WeGlideDownloadGlue::ShowTiles(uint32_t now) noexcept
try {
  const unsigned interval = WeGlide::SmallestInterval(latest);
  const auto delay = std::chrono::seconds{interval == 0 ? 20 : interval};
  const auto *tick = TickFor(selected);
  if (tick == nullptr) {
    LogState("no tick");
    Schedule(delay);
    return;
  }

  const uint32_t frame = FrameFor(*tick, now);
  const unsigned zoom = ZoomFor(*tick);
  static uint32_t logged_frame = 0;
  static unsigned logged_zoom = 0;
  if (frame != logged_frame || zoom != logged_zoom) {
    logged_frame = frame;
    logged_zoom = zoom;
    LogDebug("WeGlide: draw {} frame={} zoom={} origin={}",
             selected, frame, zoom, tick->time);
  }

  const auto candidates =
    TilesInView(zoom, CommonInterface::Basic().location);
  view_count = 0;
  for (const auto &candidate : candidates) {
    if (view_count == TILE_SLOTS)
      break;
    const auto &tile = candidate.tile;
    view[view_count++] = {tile.zoom, tile.x, tile.y};
  }
  if (candidates.empty())
    LogState("no tiles in view");

  for (const auto &candidate : candidates) {
    if (IsShown(candidate.tile, frame) ||
        TileCooling(candidate.tile, frame, now))
      continue;

    auto file = TileFile(selected, frame, candidate.tile);
    if (file == nullptr)
      break;

    requested = {selected, frame, candidate.tile};
    if (File::Exists(file)) {
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
    StartGet(Kind::TILE, WeGlide::GridUrl(query, relative), {},
             std::move(file));
    return;
  }

  LogState("view ready");
  Schedule(delay);
} catch (...) {
  LogError(std::current_exception(), "WeGlide overlay");
}

/** Take the picture down once the frame on the map is too old. */
static void
RetireShown(uint32_t now) noexcept
{
  uint32_t newest = 0;
  bool any = false;
  for (const auto &slot : shown) {
    if (!slot.used)
      continue;
    any = true;
    if (slot.time > newest)
      newest = slot.time;
  }
  if (!any || now <= newest)
    return;
  if (now - newest > kMaxShownAgeSeconds)
    ClearTiles();
}

void
WeGlideDownloadGlue::Poll() noexcept
{
  if (!active)
    return;

  RetireShown(UnixNow());

  if (suspended || IsRunning())
    return;

  if (!CommonInterface::GetComputerSettings().weglide.enabled) {
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

  try {
    const auto account = WeGlide::LoadAccountToken();
    if (RefreshOrSignIn(account, now))
      return;

    const bool renew = !query_valid ||
      WeGlide::NextTileQuery(now, query.expires, 0) ==
      WeGlide::TileQueryAction::RENEW;
    if (renew) {
      StartGet(Kind::TOKEN,
               std::string{WeGlideSettings::default_url} +
               "/copilot/tiles/token",
               Bearer(account));
      return;
    }

    if (!catalog_valid) {
      StartGet(Kind::CATALOG, WeGlide::GridUrl(query, "catalog"), {});
      return;
    }

    const unsigned interval = WeGlide::SmallestInterval(latest);
    if (latest_fetched_at == 0 || interval == 0 ||
        now >= latest_fetched_at + interval) {
      StartGet(Kind::LATEST, WeGlide::GridUrl(query, "latest"), {});
      return;
    }

    ShowTiles(now);
  } catch (...) {
    LogError(std::current_exception(), "WeGlide overlay");
  }
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
  selected = std::string{layer_id};
  requested_time = frame_time;
  active = true;
  suspended = false;
  /* A different layer is a different picture. A new frame keeps the
     old one up until its squares are replaced. */
  if (changed)
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

  requested_time = frame_time;
  const auto *tick = TickFor(selected);
  const uint32_t after = tick != nullptr
    ? FrameFor(*tick, UnixNow()) : 0;
  LogDebug("WeGlide: frame request {} shows {}", frame_time, after);
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

uint32_t
DrawnFrameTime() noexcept
{
  uint32_t oldest = 0;
  bool any = false;
  for (const auto &slot : shown) {
    if (!slot.used || slot.layer != selected || !InView(slot.tile))
      continue;
    if (!any || slot.time < oldest)
      oldest = slot.time;
    any = true;
  }
  return any ? oldest : 0;
}

const std::vector<LayerInfo> &
Catalog() noexcept
{
  return catalog;
}

} // namespace WeGlide
