// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MVT.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace XCThermMVT {

namespace {

enum class WireType : uint8_t {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimited = 2,
  Fixed32 = 5,
};

struct Reader {
  const uint8_t *ptr;
  const uint8_t *end;

  bool Empty() const noexcept { return ptr >= end; }

  bool ReadVarint(uint64_t &out) {
    out = 0;
    unsigned shift = 0;

    while (ptr < end && shift <= 63) {
      const uint8_t byte = *ptr++;
      out |= uint64_t(byte & 0x7fu) << shift;
      if ((byte & 0x80u) == 0)
        return true;
      shift += 7;
    }

    return false;
  }

  bool ReadTag(uint32_t &field, WireType &wire) {
    uint64_t key = 0;
    if (!ReadVarint(key))
      return false;

    field = uint32_t(key >> 3);
    wire = static_cast<WireType>(key & 0x7u);
    return true;
  }

  bool ReadLengthDelimited(Reader &sub) {
    uint64_t length = 0;
    if (!ReadVarint(length))
      return false;

    if (length > uint64_t(end - ptr))
      return false;

    sub.ptr = ptr;
    sub.end = ptr + size_t(length);
    ptr += size_t(length);
    return true;
  }

  bool ReadString(std::string &out) {
    Reader sub{};
    if (!ReadLengthDelimited(sub))
      return false;

    out.assign(reinterpret_cast<const char *>(sub.ptr),
               reinterpret_cast<const char *>(sub.end));
    return true;
  }

  bool ReadFixed32(uint32_t &out) {
    if (end - ptr < 4)
      return false;

    out = uint32_t(ptr[0]) |
          (uint32_t(ptr[1]) << 8) |
          (uint32_t(ptr[2]) << 16) |
          (uint32_t(ptr[3]) << 24);
    ptr += 4;
    return true;
  }

  bool ReadFixed64(uint64_t &out) {
    if (end - ptr < 8)
      return false;

    out = uint64_t(ptr[0]) |
          (uint64_t(ptr[1]) << 8) |
          (uint64_t(ptr[2]) << 16) |
          (uint64_t(ptr[3]) << 24) |
          (uint64_t(ptr[4]) << 32) |
          (uint64_t(ptr[5]) << 40) |
          (uint64_t(ptr[6]) << 48) |
          (uint64_t(ptr[7]) << 56);
    ptr += 8;
    return true;
  }

  bool Skip(WireType wire) {
    switch (wire) {
    case WireType::Varint: {
      uint64_t dummy = 0;
      return ReadVarint(dummy);
    }
    case WireType::Fixed64:
      if (end - ptr < 8)
        return false;
      ptr += 8;
      return true;
    case WireType::LengthDelimited: {
      Reader sub{};
      return ReadLengthDelimited(sub);
    }
    case WireType::Fixed32:
      if (end - ptr < 4)
        return false;
      ptr += 4;
      return true;
    }

    return false;
  }
};

static inline int32_t ZigZagDecode(uint64_t value) {
  return int32_t((value >> 1) ^ (~(value & 1) + 1));
}

struct ProtoValue {
  enum class Type {
    Missing,
    Number,
    String,
    Bool,
  } type = Type::Missing;

  double number = 0;
  std::string text;
  bool boolean = false;
};

struct RawFeature {
  uint32_t geom_type = 0;
  std::vector<uint32_t> tags;
  std::vector<uint32_t> geometry_cmds;
};

static bool ParseValue(Reader &reader, ProtoValue &value) {
  while (!reader.Empty()) {
    uint32_t field = 0;
    WireType wire{};
    if (!reader.ReadTag(field, wire))
      return false;

    switch (field) {
    case 1: {
      std::string text;
      if (wire != WireType::LengthDelimited || !reader.ReadString(text))
        return false;
      value.type = ProtoValue::Type::String;
      value.text = std::move(text);
      break;
    }
    case 2: {
      uint32_t raw = 0;
      if (wire != WireType::Fixed32 || !reader.ReadFixed32(raw))
        return false;
      float f;
      static_assert(sizeof(float) == sizeof(uint32_t), "float size mismatch");
      std::memcpy(&f, &raw, sizeof(float));
      value.type = ProtoValue::Type::Number;
      value.number = f;
      break;
    }
    case 3: {
      uint64_t raw = 0;
      if (wire != WireType::Fixed64 || !reader.ReadFixed64(raw))
        return false;
      double d;
      static_assert(sizeof(double) == sizeof(uint64_t), "double size mismatch");
      std::memcpy(&d, &raw, sizeof(double));
      value.type = ProtoValue::Type::Number;
      value.number = d;
      break;
    }
    case 4:
    case 5:
    case 6: {
      uint64_t v = 0;
      if (wire != WireType::Varint || !reader.ReadVarint(v))
        return false;
      value.type = ProtoValue::Type::Number;
      value.number = (field == 6) ? ZigZagDecode(v) : static_cast<double>(v);
      break;
    }
    case 7: {
      uint64_t v = 0;
      if (wire != WireType::Varint || !reader.ReadVarint(v))
        return false;
      value.type = ProtoValue::Type::Bool;
      value.boolean = (v != 0);
      break;
    }
    default:
      if (!reader.Skip(wire))
        return false;
      break;
    }
  }

  return true;
}

static bool ParseFeature(Reader &reader, RawFeature &feature) {
  while (!reader.Empty()) {
    uint32_t field = 0;
    WireType wire{};
    if (!reader.ReadTag(field, wire))
      return false;

    switch (field) {
    case 2: {
      if (wire != WireType::LengthDelimited)
        return false;
      Reader sub{};
      if (!reader.ReadLengthDelimited(sub))
        return false;
      while (!sub.Empty()) {
        uint64_t v = 0;
        if (!sub.ReadVarint(v))
          return false;
        feature.tags.push_back(static_cast<uint32_t>(v));
      }
      break;
    }
    case 3: {
      uint64_t v = 0;
      if (wire != WireType::Varint || !reader.ReadVarint(v))
        return false;
      feature.geom_type = static_cast<uint32_t>(v);
      break;
    }
    case 4: {
      if (wire != WireType::LengthDelimited)
        return false;
      Reader sub{};
      if (!reader.ReadLengthDelimited(sub))
        return false;
      while (!sub.Empty()) {
        uint64_t v = 0;
        if (!sub.ReadVarint(v))
          return false;
        feature.geometry_cmds.push_back(static_cast<uint32_t>(v));
      }
      break;
    }
    default:
      if (!reader.Skip(wire))
        return false;
      break;
    }
  }

  return true;
}

struct DecodedLayer {
  std::string name;
  uint32_t extent = 4096;
  std::vector<std::string> keys;
  std::vector<ProtoValue> values;
  std::vector<RawFeature> features;
};

static bool ParseLayer(Reader &reader, DecodedLayer &layer) {
  while (!reader.Empty()) {
    uint32_t field = 0;
    WireType wire{};
    if (!reader.ReadTag(field, wire))
      return false;

    switch (field) {
    case 1: {
      std::string name;
      if (wire != WireType::LengthDelimited || !reader.ReadString(name))
        return false;
      layer.name = std::move(name);
      break;
    }
    case 2: {
      if (wire != WireType::LengthDelimited)
        return false;
      Reader sub{};
      if (!reader.ReadLengthDelimited(sub))
        return false;
      RawFeature feature;
      if (!ParseFeature(sub, feature))
        return false;
      layer.features.push_back(std::move(feature));
      break;
    }
    case 3: {
      std::string key;
      if (wire != WireType::LengthDelimited || !reader.ReadString(key))
        return false;
      layer.keys.push_back(std::move(key));
      break;
    }
    case 4: {
      if (wire != WireType::LengthDelimited)
        return false;
      Reader sub{};
      if (!reader.ReadLengthDelimited(sub))
        return false;
      ProtoValue value;
      if (!ParseValue(sub, value))
        return false;
      layer.values.push_back(std::move(value));
      break;
    }
    case 5: {
      uint64_t extent = 0;
      if (wire != WireType::Varint || !reader.ReadVarint(extent))
        return false;
      layer.extent = static_cast<uint32_t>(extent);
      break;
    }
    default:
      if (!reader.Skip(wire))
        return false;
      break;
    }
  }

  return true;
}

static std::unordered_map<std::string, ProtoValue>
BuildProperties(const DecodedLayer &layer, const RawFeature &feature) {
  std::unordered_map<std::string, ProtoValue> properties;

  for (size_t i = 0; i + 1 < feature.tags.size(); i += 2) {
    const uint32_t key_idx = feature.tags[i];
    const uint32_t value_idx = feature.tags[i + 1];
    if (key_idx >= layer.keys.size() || value_idx >= layer.values.size())
      continue;

    properties.emplace(layer.keys[key_idx], layer.values[value_idx]);
  }

  return properties;
}

static bool DecodePoints(const RawFeature &feature,
                         std::vector<IntPoint> &out_points) {
  if (feature.geom_type != 1)
    return true;

  int x = 0;
  int y = 0;

  size_t i = 0;
  uint32_t command = 0;
  uint32_t repeat = 0;

  while (i < feature.geometry_cmds.size()) {
    if (repeat == 0) {
      const uint32_t cmd = feature.geometry_cmds[i++];
      command = cmd & 0x7;
      repeat = cmd >> 3;
      if (repeat == 0)
        return false;
    }

    --repeat;

    if (command == 1 || command == 2) {
      if (i + 1 >= feature.geometry_cmds.size())
        return false;

      const int dx = ZigZagDecode(feature.geometry_cmds[i++]);
      const int dy = ZigZagDecode(feature.geometry_cmds[i++]);
      x += dx;
      y += dy;

      if (command == 1)
        out_points.push_back({x, y});
    } else if (command == 7) {
      continue;
    } else {
      return false;
    }
  }

  return true;
}

static bool DecodePolygonRings(const RawFeature &feature,
                               std::vector<std::vector<IntPoint>> &out_rings) {
  if (feature.geom_type != 3)
    return true;

  int x = 0;
  int y = 0;

  std::vector<IntPoint> current_ring;

  size_t i = 0;
  uint32_t command = 0;
  uint32_t repeat = 0;

  while (i < feature.geometry_cmds.size()) {
    if (repeat == 0) {
      const uint32_t cmd = feature.geometry_cmds[i++];
      command = cmd & 0x7;
      repeat = cmd >> 3;
      if (repeat == 0)
        return false;
    }

    --repeat;

    if (command == 1 || command == 2) {
      if (i + 1 >= feature.geometry_cmds.size())
        return false;

      const int dx = ZigZagDecode(feature.geometry_cmds[i++]);
      const int dy = ZigZagDecode(feature.geometry_cmds[i++]);
      x += dx;
      y += dy;

      if (command == 1) {
        if (!current_ring.empty()) {
          out_rings.push_back(std::move(current_ring));
          current_ring.clear();
        }
      }

      current_ring.push_back({x, y});
    } else if (command == 7) {
      if (!current_ring.empty() &&
          (current_ring.front().x != current_ring.back().x ||
           current_ring.front().y != current_ring.back().y)) {
        current_ring.push_back(current_ring.front());
      }
    } else {
      return false;
    }
  }

  if (!current_ring.empty())
    out_rings.push_back(std::move(current_ring));

  return true;
}

} // namespace

bool Parse(const std::vector<std::uint8_t> &buffer, Tile &out) {
  out = {};

  Reader root{buffer.data(), buffer.data() + buffer.size()};
  while (!root.Empty()) {
    uint32_t field = 0;
    WireType wire{};
    if (!root.ReadTag(field, wire))
      return false;

    if (field != 3 || wire != WireType::LengthDelimited) {
      if (!root.Skip(wire))
        return false;
      continue;
    }

    Reader layer_reader{};
    if (!root.ReadLengthDelimited(layer_reader))
      return false;

    DecodedLayer layer;
    if (!ParseLayer(layer_reader, layer))
      return false;

    if (layer.name == "points") {
      out.extent = layer.extent;

      for (const auto &feature : layer.features) {
        auto props = BuildProperties(layer, feature);
        const auto dir_it = props.find("dir");
        const auto v_it = props.find("v");
        if (dir_it == props.end() || v_it == props.end())
          continue;
        if (dir_it->second.type != ProtoValue::Type::Number ||
            v_it->second.type != ProtoValue::Type::Number)
          continue;

        std::vector<IntPoint> points;
        if (!DecodePoints(feature, points))
          continue;

        for (const auto &p : points) {
          PointFeature point;
          point.x = p.x;
          point.y = p.y;
          point.direction_deg = static_cast<float>(dir_it->second.number);
          point.vertical_speed = static_cast<float>(v_it->second.number);
          out.points.push_back(point);
        }
      }
    } else if (layer.name == "polygons") {
      out.extent = layer.extent;

      for (const auto &feature : layer.features) {
        auto props = BuildProperties(layer, feature);
        const auto min_it = props.find("min");
        const auto max_it = props.find("max");
        if (min_it == props.end() || max_it == props.end())
          continue;
        if (min_it->second.type != ProtoValue::Type::Number ||
            max_it->second.type != ProtoValue::Type::Number)
          continue;

        PolygonFeature polygon;
        polygon.min_value = static_cast<float>(min_it->second.number);
        polygon.max_value = static_cast<float>(max_it->second.number);

        if (!DecodePolygonRings(feature, polygon.rings))
          continue;

        if (!polygon.rings.empty())
          out.polygons.push_back(std::move(polygon));
      }
    }
  }

  return true;
}

} // namespace XCThermMVT
