// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace WeGlide {

struct Rgb {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

struct LegendStop {
  double value = 0;
  Rgb color;
};

/** One layer from `GET …/grid/catalog`. */
struct LayerInfo {
  std::string id;
  std::string name;
  std::string units;
  std::vector<LegendStop> legend;
};

/** One row from `GET …/grid/latest`. */
struct LayerTick {
  std::string layer_id;
  uint32_t time = 0;
  unsigned interval_seconds = 0;
  unsigned max_zoom = 0;
};

/** `GET /v1/copilot/tiles/token`. `query` is appended unchanged. */
struct TileQuery {
  std::string base_url;
  std::string query;
  uint32_t expires = 0;
};

enum class TileQueryAction : uint8_t {
  /** `query` is still inside the renew margin. */
  USE,
  /** Fetch a new tile token. */
  RENEW,
  /** A subscription 403 is still inside its backoff. */
  WAIT,
};

/** Renew when less than this remains before `expires`. */
inline constexpr unsigned kTileTokenRenewSeconds = 30u * 60u;

/** Wait after `GET /v1/copilot/tiles/token` answers 403. */
inline constexpr unsigned kSubscriptionWaitSeconds = 10u * 60u;

/**
 * Backstop once cached frames fill the directory. A live set is a
 * few kilobytes; the cap drops the oldest files.
 */
inline constexpr uint64_t kTileCacheCapBytes = 8ull * 1024ull * 1024ull;

[[nodiscard]] std::optional<TileQuery>
ParseTileQuery(std::string_view json);

[[nodiscard]] std::optional<std::vector<LayerInfo>>
ParseCatalog(std::string_view json);

[[nodiscard]] std::optional<std::vector<LayerTick>>
ParseLatest(std::string_view json);

/**
 * `{base_url}/grid/{path}?{query}`.
 *
 * @return empty when the token or @p path is empty
 */
[[nodiscard]] std::string
GridUrl(const TileQuery &token, std::string_view path);

/**
 * `{layer_id}/{time}/{z}/{x}/{y}.png` for a slippy tile.
 *
 * @return empty when @p layer_id is not a safe path segment
 */
[[nodiscard]] std::string
TileRelativePath(std::string_view layer_id, uint32_t time,
                 unsigned z, unsigned x, unsigned y);

[[nodiscard]] TileQueryAction
NextTileQuery(uint32_t now, uint32_t expires,
              uint32_t wait_until) noexcept;

/**
 * One extra attempt after a CDN 403 or a connection error.
 *
 * @param consecutive_failures how many failures are already recorded
 */
[[nodiscard]] bool
AllowTileRetry(unsigned consecutive_failures) noexcept;

[[nodiscard]] uint32_t
SubscriptionWaitUntil(uint32_t now) noexcept;

[[nodiscard]] bool
IsUsableTick(const LayerTick &tick) noexcept;

/**
 * Newest frame at or before @p now.
 *
 * `latest.time` is the series origin. Step it by `interval` rather
 * than copying it: on 2026-10-19 the origin stayed hours behind the
 * clock while those later URLs held the painted tiles.
 */
[[nodiscard]] uint32_t
CurrentTickTime(const LayerTick &tick, uint32_t now) noexcept;

/**
 * Snap @p requested onto the series grid, not after the live frame
 * and not before the series origin.
 */
[[nodiscard]] uint32_t
AlignTickTime(const LayerTick &tick, uint32_t requested,
              uint32_t now) noexcept;

/**
 * Move @p frame by @p delta cursor steps of 15 minutes, snapped to
 * the layer interval. A positive delta stops at the live frame. A
 * negative delta stops at the series origin.
 */
[[nodiscard]] uint32_t
StepTickTime(const LayerTick &tick, uint32_t frame, int delta,
             uint32_t now) noexcept;

/** The smaller positive interval, or 0 when none is usable. */
[[nodiscard]] unsigned
SmallestInterval(std::span<const LayerTick> ticks) noexcept;

/** One cached PNG, for #TilesToDrop. */
struct StoredTile {
  std::string layer_id;
  uint32_t time = 0;
  uint64_t bytes = 0;
  int64_t mtime = 0;
};

/**
 * Indexes to delete, oldest byte-cap victims last.
 *
 * A layer in @p latest keeps every time on its grid from the series
 * origin through the live frame at @p now. Times the series has left
 * behind are dropped, so stepping the cursor reuses cached tiles. A
 * cap of 0 does not apply the byte limit.
 */
[[nodiscard]] std::vector<std::size_t>
TilesToDrop(std::span<const StoredTile> tiles,
            std::span<const LayerTick> latest,
            uint32_t now,
            uint64_t cap_bytes);

} // namespace WeGlide
