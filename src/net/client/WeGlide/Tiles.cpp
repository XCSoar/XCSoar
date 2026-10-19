// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Tiles.hpp"

#include <boost/json.hpp>

#include <cmath>
#include <fmt/format.h>

namespace WeGlide {

static bool
IsLayerId(std::string_view id) noexcept
{
  if (id.empty() || id.size() > 32)
    return false;

  for (const unsigned char c : id) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!ok)
      return false;
  }
  return true;
}

static std::optional<std::string>
ReadString(const boost::json::object &object, std::string_view key)
{
  const auto *value = object.if_contains(key);
  if (value == nullptr || !value->is_string())
    return std::nullopt;
  return std::string{value->as_string()};
}

static std::optional<double>
ReadNumber(const boost::json::value &value)
{
  if (const auto i = value.if_int64())
    return static_cast<double>(*i);
  if (const auto u = value.if_uint64())
    return static_cast<double>(*u);
  if (const auto d = value.if_double())
    return *d;
  return std::nullopt;
}

static std::optional<uint32_t>
ReadUint32(const boost::json::value &value)
{
  if (const auto u = value.if_uint64(); u && *u <= 0xffffffffu)
    return static_cast<uint32_t>(*u);
  if (const auto i = value.if_int64();
      i && *i >= 0 && static_cast<uint64_t>(*i) <= 0xffffffffu)
    return static_cast<uint32_t>(*i);
  if (const auto d = value.if_double();
      d && *d >= 0 && *d <= 4294967295.0 && std::floor(*d) == *d)
    return static_cast<uint32_t>(*d);
  return std::nullopt;
}

static std::optional<Rgb>
ReadColor(const boost::json::value &value)
{
  const auto *array = value.if_array();
  if (array == nullptr || array->size() != 3)
    return std::nullopt;

  Rgb color;
  uint8_t *channels[] = {&color.r, &color.g, &color.b};
  for (std::size_t i = 0; i < 3; ++i) {
    const auto channel = ReadNumber((*array)[i]);
    if (!channel || *channel < 0 || *channel > 255 ||
        std::floor(*channel) != *channel)
      return std::nullopt;
    *channels[i] = static_cast<uint8_t>(*channel);
  }
  return color;
}

std::optional<TileQuery>
ParseTileQuery(std::string_view json)
{
  try {
    const auto body = boost::json::parse(json);
    if (!body.is_object())
      return std::nullopt;

    const auto &object = body.as_object();
    auto base_url = ReadString(object, "base_url");
    auto query = ReadString(object, "query");
    const auto *expires = object.if_contains("expires");
    if (!base_url || base_url->empty() || !query || query->empty() ||
        expires == nullptr)
      return std::nullopt;

    const auto expires_at = ReadUint32(*expires);
    if (!expires_at)
      return std::nullopt;

    TileQuery token;
    token.base_url = std::move(*base_url);
    token.query = std::move(*query);
    token.expires = *expires_at;
    return token;
  } catch (...) {
    return std::nullopt;
  }
}

static std::optional<LayerInfo>
ReadLayer(const boost::json::object &object)
{
  auto id = ReadString(object, "id");
  auto name = ReadString(object, "name");
  if (!id || !IsLayerId(*id) || !name)
    return std::nullopt;

  LayerInfo layer;
  layer.id = std::move(*id);
  layer.name = std::move(*name);
  if (auto units = ReadString(object, "units"))
    layer.units = std::move(*units);

  const auto *legend = object.if_contains("legend");
  if (legend != nullptr && legend->is_object()) {
    const auto *colors = legend->as_object().if_contains("colors");
    if (colors != nullptr && colors->is_array()) {
      for (const auto &stop_value : colors->as_array()) {
        if (!stop_value.is_object())
          continue;
        const auto &stop = stop_value.as_object();
        const auto *value = stop.if_contains("value");
        const auto *color = stop.if_contains("color");
        if (value == nullptr || color == nullptr)
          continue;
        const auto number = ReadNumber(*value);
        const auto rgb = ReadColor(*color);
        if (!number || !rgb)
          continue;
        layer.legend.push_back({*number, *rgb});
      }
    }
  }
  return layer;
}

std::optional<std::vector<LayerInfo>>
ParseCatalog(std::string_view json)
{
  try {
    const auto body = boost::json::parse(json);
    if (!body.is_array())
      return std::nullopt;

    std::vector<LayerInfo> layers;
    for (const auto &value : body.as_array()) {
      if (!value.is_object())
        continue;
      if (auto layer = ReadLayer(value.as_object()))
        layers.push_back(std::move(*layer));
    }
    return layers;
  } catch (...) {
    return std::nullopt;
  }
}

static std::optional<LayerTick>
ReadTick(const boost::json::object &object)
{
  auto layer_id = ReadString(object, "layer_id");
  const auto *time = object.if_contains("time");
  if (!layer_id || !IsLayerId(*layer_id) || time == nullptr)
    return std::nullopt;

  const auto tick_time = ReadUint32(*time);
  if (!tick_time || *tick_time == 0)
    return std::nullopt;

  LayerTick tick;
  tick.layer_id = std::move(*layer_id);
  tick.time = *tick_time;
  if (const auto *interval = object.if_contains("interval"))
    if (const auto seconds = ReadUint32(*interval))
      tick.interval_seconds = *seconds;
  if (const auto *zoom = object.if_contains("max_zoom"))
    if (const auto level = ReadUint32(*zoom))
      tick.max_zoom = *level;
  return tick;
}

std::optional<std::vector<LayerTick>>
ParseLatest(std::string_view json)
{
  try {
    const auto body = boost::json::parse(json);
    if (!body.is_array())
      return std::nullopt;

    std::vector<LayerTick> ticks;
    for (const auto &value : body.as_array()) {
      if (!value.is_object())
        continue;
      if (auto tick = ReadTick(value.as_object()))
        ticks.push_back(std::move(*tick));
    }
    return ticks;
  } catch (...) {
    return std::nullopt;
  }
}

std::string
GridUrl(const TileQuery &token, std::string_view path)
{
  if (token.base_url.empty() || token.query.empty() || path.empty())
    return {};

  std::string url = token.base_url;
  while (!url.empty() && url.back() == '/')
    url.pop_back();
  url.append("/grid/");
  url.append(path);
  url.push_back('?');
  url.append(token.query);
  return url;
}

std::string
TileRelativePath(std::string_view layer_id, uint32_t time,
                 unsigned z, unsigned x, unsigned y)
{
  if (!IsLayerId(layer_id) || time == 0)
    return {};
  return fmt::format("{}/{}/{}/{}/{}.png", layer_id, time, z, x, y);
}

TileQueryAction
NextTileQuery(uint32_t now, uint32_t expires,
              uint32_t wait_until) noexcept
{
  if (wait_until != 0 && now < wait_until)
    return TileQueryAction::WAIT;

  const uint64_t renew_at = uint64_t{now} + kTileTokenRenewSeconds;
  if (expires == 0 || uint64_t{expires} <= renew_at)
    return TileQueryAction::RENEW;
  return TileQueryAction::USE;
}

bool
AllowTileRetry(unsigned consecutive_failures) noexcept
{
  return consecutive_failures == 0;
}

uint32_t
SubscriptionWaitUntil(uint32_t now) noexcept
{
  const uint64_t sum = uint64_t{now} + kSubscriptionWaitSeconds;
  return sum > 0xffffffffu ? 0xffffffffu : static_cast<uint32_t>(sum);
}

bool
IsUsableTick(const LayerTick &tick) noexcept
{
  return IsLayerId(tick.layer_id) && tick.time != 0 &&
         tick.interval_seconds != 0 && tick.max_zoom != 0;
}

uint32_t
CurrentTickTime(const LayerTick &tick, uint32_t now) noexcept
{
  if (tick.interval_seconds == 0 || now <= tick.time)
    return tick.time;

  const uint32_t elapsed = now - tick.time;
  const uint32_t step = tick.interval_seconds;
  return tick.time + elapsed - elapsed % step;
}

uint32_t
AlignTickTime(const LayerTick &tick, uint32_t requested,
              uint32_t now) noexcept
{
  const uint32_t live = CurrentTickTime(tick, now);
  if (tick.interval_seconds == 0 || requested >= live)
    return live;
  if (requested <= tick.time)
    return tick.time;

  const uint32_t elapsed = requested - tick.time;
  const uint32_t step = tick.interval_seconds;
  return tick.time + elapsed - elapsed % step;
}

static constexpr uint32_t kCursorStepSeconds = 15 * 60;

static uint32_t
CursorStep(const LayerTick &tick) noexcept
{
  const uint32_t interval = tick.interval_seconds;
  if (interval == 0 || kCursorStepSeconds % interval == 0)
    return kCursorStepSeconds;

  const uint32_t count = (kCursorStepSeconds + interval - 1) / interval;
  return count * interval;
}

uint32_t
StepTickTime(const LayerTick &tick, uint32_t frame, int delta,
             uint32_t now) noexcept
{
  const uint32_t live = CurrentTickTime(tick, now);
  if (tick.interval_seconds == 0 || delta == 0)
    return frame == 0 ? live : AlignTickTime(tick, frame, now);

  const uint32_t aligned = frame == 0
    ? live : AlignTickTime(tick, frame, now);
  const uint32_t step = CursorStep(tick);
  if (delta > 0) {
    const uint64_t next = uint64_t{aligned} +
      uint64_t{step} * uint64_t(delta);
    return next >= live ? live : uint32_t(next);
  }

  const uint64_t retreat = uint64_t{step} * uint64_t(-int64_t(delta));
  const uint64_t span = uint64_t{aligned} - tick.time;
  if (retreat >= span)
    return tick.time;
  return aligned - uint32_t(retreat);
}

unsigned
SmallestInterval(std::span<const LayerTick> ticks) noexcept
{
  unsigned smallest = 0;
  for (const auto &tick : ticks) {
    if (!IsUsableTick(tick))
      continue;
    if (smallest == 0 || tick.interval_seconds < smallest)
      smallest = tick.interval_seconds;
  }
  return smallest;
}

static const LayerTick *
LatestFor(std::span<const LayerTick> latest, std::string_view layer_id)
{
  const LayerTick *found = nullptr;
  for (const auto &tick : latest) {
    if (!IsUsableTick(tick) || tick.layer_id != layer_id)
      continue;
    found = &tick;
  }
  return found;
}

static bool
OnSeries(const LayerTick &tick, uint32_t time, uint32_t now) noexcept
{
  const uint32_t live = CurrentTickTime(tick, now);
  if (time < tick.time || time > live || tick.interval_seconds == 0)
    return false;
  return (time - tick.time) % tick.interval_seconds == 0;
}

std::vector<std::size_t>
TilesToDrop(std::span<const StoredTile> tiles,
            std::span<const LayerTick> latest,
            uint32_t now,
            uint64_t cap_bytes)
{
  std::vector<std::size_t> dropped;
  std::vector<bool> gone(tiles.size(), false);
  uint64_t kept = 0;

  for (std::size_t i = 0; i < tiles.size(); ++i) {
    const auto *tick = LatestFor(latest, tiles[i].layer_id);
    if (tick != nullptr && !OnSeries(*tick, tiles[i].time, now)) {
      gone[i] = true;
      dropped.push_back(i);
    } else {
      kept += tiles[i].bytes;
    }
  }

  if (cap_bytes == 0 || kept <= cap_bytes)
    return dropped;

  while (kept > cap_bytes) {
    std::size_t oldest = tiles.size();
    for (std::size_t i = 0; i < tiles.size(); ++i) {
      if (gone[i])
        continue;
      if (oldest == tiles.size() ||
          tiles[i].mtime < tiles[oldest].mtime ||
          (tiles[i].mtime == tiles[oldest].mtime && i < oldest))
        oldest = i;
    }
    if (oldest == tiles.size())
      break;

    gone[oldest] = true;
    if (tiles[oldest].bytes >= kept)
      kept = 0;
    else
      kept -= tiles[oldest].bytes;
    dropped.push_back(oldest);
  }
  return dropped;
}

} // namespace WeGlide
