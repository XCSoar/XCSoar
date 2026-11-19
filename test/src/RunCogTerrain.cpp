// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * Test program for Cloud-Optimized GeoTIFF (COG) terrain loading.
 *
 * Usage:
 *   RunCogTerrain <lat> <lon>
 *
 * This fetches the OpenTopography SRTM COG tile containing the given
 * coordinate and prints elevation data for the surrounding area.
 */

#include "CoInstance.hpp"
#include "net/http/CoGetRange.hpp"
#include "net/http/Init.hpp"
#include "system/Args.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/PrintException.hxx"
#include "util/ByteOrder.hxx"

#include <fmt/format.h>
#include <zlib.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <stdexcept>

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

static constexpr const char *OPENTOPO_BASE =
  "https://opentopography.s3.sdsc.edu/raster/SRTM_GL1/SRTM_GL1_srtm/";

struct TiffHeader {
  uint16_t byte_order;
  uint16_t magic;
  uint32_t ifd_offset;
};

struct TiffTag {
  uint16_t tag;
  uint16_t type;
  uint32_t count;
  uint32_t value_offset;
};

static constexpr uint16_t TIFF_LITTLE_ENDIAN = 0x4949;
static constexpr uint16_t TIFF_BIG_ENDIAN = 0x4D4D;
static constexpr uint16_t TIFF_MAGIC = 42;

static constexpr uint16_t TAG_IMAGE_WIDTH = 256;
static constexpr uint16_t TAG_IMAGE_LENGTH = 257;
static constexpr uint16_t TAG_BITS_PER_SAMPLE = 258;
static constexpr uint16_t TAG_COMPRESSION = 259;
static constexpr uint16_t TAG_SAMPLE_FORMAT = 339;
static constexpr uint16_t TAG_TILE_WIDTH = 322;
static constexpr uint16_t TAG_TILE_LENGTH = 323;
static constexpr uint16_t TAG_TILE_OFFSETS = 324;
static constexpr uint16_t TAG_TILE_BYTE_COUNTS = 325;
static constexpr uint16_t TAG_MODEL_TIEPOINT = 33922;
static constexpr uint16_t TAG_MODEL_PIXEL_SCALE = 33550;

static constexpr uint16_t COMPRESSION_NONE = 1;
static constexpr uint16_t COMPRESSION_DEFLATE = 8;
static constexpr uint16_t COMPRESSION_ZLIB = 32946;

class TiffReader {
  bool little_endian = true;
  std::string data;

public:
  explicit TiffReader(std::string &&d) : data(std::move(d)) {}

  [[nodiscard]] size_t Size() const noexcept { return data.size(); }
  [[nodiscard]] const uint8_t *Data() const noexcept {
    return reinterpret_cast<const uint8_t *>(data.data());
  }

  void Append(std::string &&more) {
    data += std::move(more);
  }

  void SetByteOrder(bool le) noexcept { little_endian = le; }

  [[nodiscard]] uint16_t ReadU16(size_t offset) const {
    if (offset + 2 > data.size())
      throw std::runtime_error("Read past end of data");
    uint16_t v;
    std::memcpy(&v, data.data() + offset, 2);
    return little_endian ? FromLE16(v) : FromBE16(v);
  }

  [[nodiscard]] uint32_t ReadU32(size_t offset) const {
    if (offset + 4 > data.size())
      throw std::runtime_error("Read past end of data");
    uint32_t v;
    std::memcpy(&v, data.data() + offset, 4);
    return little_endian ? FromLE32(v) : FromBE32(v);
  }

  [[nodiscard]] int16_t ReadS16(size_t offset) const {
    return static_cast<int16_t>(ReadU16(offset));
  }

  [[nodiscard]] double ReadDouble(size_t offset) const {
    if (offset + 8 > data.size())
      throw std::runtime_error("Read past end of data");
    double v;
    std::memcpy(&v, data.data() + offset, 8);
    return v;
  }
};

struct CogInfo {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t tile_width = 0;
  uint32_t tile_height = 0;
  uint16_t bits_per_sample = 0;
  uint16_t compression = 0;
  uint16_t sample_format = 1;

  std::vector<uint64_t> tile_offsets;
  std::vector<uint64_t> tile_byte_counts;

  double pixel_scale_x = 0;
  double pixel_scale_y = 0;
  double tie_point_x = 0;
  double tie_point_y = 0;

  [[nodiscard]] unsigned TilesX() const noexcept {
    return tile_width > 0 ? (width + tile_width - 1) / tile_width : 0;
  }

  [[nodiscard]] unsigned TilesY() const noexcept {
    return tile_height > 0 ? (height + tile_height - 1) / tile_height : 0;
  }

  [[nodiscard]] GeoPoint GetTopLeft() const noexcept {
    return GeoPoint(Angle::Degrees(tie_point_x),
                    Angle::Degrees(tie_point_y));
  }

  [[nodiscard]] GeoPoint PixelToGeo(unsigned px, unsigned py) const noexcept {
    return GeoPoint(
      Angle::Degrees(tie_point_x + px * pixel_scale_x),
      Angle::Degrees(tie_point_y - py * pixel_scale_y));
  }

  [[nodiscard]] std::pair<unsigned, unsigned> GeoToPixel(GeoPoint p) const noexcept {
    double px = (p.longitude.Degrees() - tie_point_x) / pixel_scale_x;
    double py = (tie_point_y - p.latitude.Degrees()) / pixel_scale_y;
    return {static_cast<unsigned>(std::max(0.0, px)),
            static_cast<unsigned>(std::max(0.0, py))};
  }

  [[nodiscard]] std::pair<unsigned, unsigned> PixelToTile(unsigned px, unsigned py) const noexcept {
    return {px / tile_width, py / tile_height};
  }
};

static std::string
MakeTileUrl(double lat, double lon)
{
  int lat_i = static_cast<int>(std::floor(lat));
  int lon_i = static_cast<int>(std::floor(lon));

  char ns = lat_i >= 0 ? 'N' : 'S';
  char ew = lon_i >= 0 ? 'E' : 'W';

  return fmt::format("{}{}{:02d}{}{:03d}.tif",
                     OPENTOPO_BASE, ns, std::abs(lat_i), ew, std::abs(lon_i));
}

static Co::InvokeTask
FetchAndParseCog(CurlGlobal &curl, const char *url, GeoPoint location)
{
  printf("Fetching COG header from: %s\n", url);

  auto header_data = co_await Net::CoGetRange(curl, url, 0, 16384);
  printf("Received %zu bytes of header data\n", header_data.size());

  TiffReader reader(std::move(header_data));

  uint16_t byte_order = reader.ReadU16(0);
  if (byte_order == TIFF_LITTLE_ENDIAN) {
    reader.SetByteOrder(true);
    printf("Byte order: Little endian\n");
  } else if (byte_order == TIFF_BIG_ENDIAN) {
    reader.SetByteOrder(false);
    printf("Byte order: Big endian\n");
  } else {
    throw std::runtime_error("Invalid TIFF byte order");
  }

  uint16_t magic = reader.ReadU16(2);
  if (magic != TIFF_MAGIC)
    throw std::runtime_error("Invalid TIFF magic number");

  uint32_t ifd_offset = reader.ReadU32(4);
  printf("First IFD at offset: %u\n", ifd_offset);

  if (ifd_offset + 2 > reader.Size()) {
    printf("Need more header data, fetching...\n");
    auto more = co_await Net::CoGetRange(curl, url, reader.Size(), 32768);
    reader.Append(std::move(more));
  }

  uint16_t num_tags = reader.ReadU16(ifd_offset);
  printf("Number of tags: %u\n", num_tags);

  CogInfo info;
  size_t tag_base = ifd_offset + 2;

  std::vector<std::pair<uint32_t, uint32_t>> deferred_reads;

  for (unsigned i = 0; i < num_tags; ++i) {
    size_t tag_offset = tag_base + i * 12;
    if (tag_offset + 12 > reader.Size()) {
      auto more = co_await Net::CoGetRange(curl, url, reader.Size(),
                                           tag_offset + 12 - reader.Size() + 4096);
      reader.Append(std::move(more));
    }

    uint16_t tag = reader.ReadU16(tag_offset);
    uint16_t type = reader.ReadU16(tag_offset + 2);
    uint32_t count = reader.ReadU32(tag_offset + 4);
    uint32_t value_or_offset = reader.ReadU32(tag_offset + 8);

    auto get_value = [&]() -> uint32_t {
      if (type == 3)
        return value_or_offset & 0xFFFF;
      return value_or_offset;
    };

    switch (tag) {
    case TAG_IMAGE_WIDTH:
      info.width = get_value();
      printf("  Image width: %u\n", info.width);
      break;
    case TAG_IMAGE_LENGTH:
      info.height = get_value();
      printf("  Image height: %u\n", info.height);
      break;
    case TAG_TILE_WIDTH:
      info.tile_width = get_value();
      printf("  Tile width: %u\n", info.tile_width);
      break;
    case TAG_TILE_LENGTH:
      info.tile_height = get_value();
      printf("  Tile height: %u\n", info.tile_height);
      break;
    case TAG_BITS_PER_SAMPLE:
      info.bits_per_sample = get_value();
      printf("  Bits per sample: %u\n", info.bits_per_sample);
      break;
    case TAG_COMPRESSION:
      info.compression = get_value();
      printf("  Compression: %u (%s)\n", info.compression,
             info.compression == COMPRESSION_NONE ? "none" :
             info.compression == COMPRESSION_DEFLATE ? "deflate" :
             info.compression == COMPRESSION_ZLIB ? "zlib" : "unknown");
      break;
    case TAG_SAMPLE_FORMAT:
      info.sample_format = get_value();
      printf("  Sample format: %u\n", info.sample_format);
      break;
    case TAG_TILE_OFFSETS:
      printf("  Tile offsets: %u entries at offset %u\n", count, value_or_offset);
      deferred_reads.emplace_back(value_or_offset, count * (type == 16 ? 8 : 4));
      info.tile_offsets.resize(count);
      for (unsigned j = 0; j < count && value_or_offset + j * 4 + 4 <= reader.Size(); ++j) {
        info.tile_offsets[j] = reader.ReadU32(value_or_offset + j * 4);
      }
      break;
    case TAG_TILE_BYTE_COUNTS:
      printf("  Tile byte counts: %u entries at offset %u\n", count, value_or_offset);
      deferred_reads.emplace_back(value_or_offset, count * (type == 16 ? 8 : 4));
      info.tile_byte_counts.resize(count);
      for (unsigned j = 0; j < count && value_or_offset + j * 4 + 4 <= reader.Size(); ++j) {
        info.tile_byte_counts[j] = reader.ReadU32(value_or_offset + j * 4);
      }
      break;
    case TAG_MODEL_PIXEL_SCALE:
      if (count >= 2) {
        if (value_or_offset + 16 <= reader.Size()) {
          info.pixel_scale_x = reader.ReadDouble(value_or_offset);
          info.pixel_scale_y = reader.ReadDouble(value_or_offset + 8);
          printf("  Pixel scale: %.9f x %.9f degrees\n",
                 info.pixel_scale_x, info.pixel_scale_y);
        }
      }
      break;
    case TAG_MODEL_TIEPOINT:
      if (count >= 6) {
        if (value_or_offset + 48 <= reader.Size()) {
          info.tie_point_x = reader.ReadDouble(value_or_offset + 24);
          info.tie_point_y = reader.ReadDouble(value_or_offset + 32);
          printf("  Tie point (top-left): %.6f, %.6f\n",
                 info.tie_point_x, info.tie_point_y);
        }
      }
      break;
    }
  }

  printf("\nCOG Summary:\n");
  printf("  Size: %u x %u pixels\n", info.width, info.height);
  printf("  Tiles: %u x %u = %u total\n",
         info.TilesX(), info.TilesY(), info.TilesX() * info.TilesY());
  printf("  Tile size: %u x %u pixels\n", info.tile_width, info.tile_height);

  GeoPoint top_left = info.GetTopLeft();
  printf("  Geographic bounds: %.6f,%.6f to %.6f,%.6f\n",
         top_left.longitude.Degrees(), top_left.latitude.Degrees(),
         top_left.longitude.Degrees() + info.width * info.pixel_scale_x,
         top_left.latitude.Degrees() - info.height * info.pixel_scale_y);

  auto [px, py] = info.GeoToPixel(location);
  auto [tx, ty] = info.PixelToTile(px, py);
  printf("\nLocation %.6f, %.6f -> pixel (%u, %u) -> tile (%u, %u)\n",
         location.latitude.Degrees(), location.longitude.Degrees(),
         px, py, tx, ty);

  if (tx >= info.TilesX() || ty >= info.TilesY()) {
    printf("Error: Location is outside tile bounds\n");
    co_return;
  }

  unsigned tile_index = ty * info.TilesX() + tx;
  if (tile_index >= info.tile_offsets.size() ||
      tile_index >= info.tile_byte_counts.size()) {
    printf("Error: Tile index %u out of range\n", tile_index);
    co_return;
  }

  uint64_t tile_offset = info.tile_offsets[tile_index];
  uint64_t tile_size = info.tile_byte_counts[tile_index];

  printf("\nFetching tile %u at offset %lu, size %lu bytes\n",
         tile_index, (unsigned long)tile_offset, (unsigned long)tile_size);

  if (tile_size == 0 || tile_size > 10 * 1024 * 1024) {
    printf("Error: Invalid tile size\n");
    co_return;
  }

  auto tile_data = co_await Net::CoGetRange(curl, url, tile_offset, tile_size);
  printf("Received %zu bytes of tile data\n", tile_data.size());

  std::string decompressed;

  if (info.compression == COMPRESSION_DEFLATE ||
      info.compression == COMPRESSION_ZLIB) {
    size_t expected_size = info.tile_width * info.tile_height * 2;
    decompressed.resize(expected_size);

    z_stream strm{};
    strm.next_in = reinterpret_cast<Bytef *>(tile_data.data());
    strm.avail_in = tile_data.size();
    strm.next_out = reinterpret_cast<Bytef *>(decompressed.data());
    strm.avail_out = decompressed.size();

    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
      printf("Error: inflateInit failed: %d\n", ret);
      co_return;
    }

    ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
      printf("Error: inflate failed: %d (processed %lu of %zu bytes)\n",
             ret, strm.total_in, tile_data.size());
      co_return;
    }

    printf("Decompressed %zu bytes to %lu bytes\n",
           tile_data.size(), strm.total_out);
    tile_data = std::move(decompressed);
  } else if (info.compression != COMPRESSION_NONE) {
    printf("Error: Unsupported compression: %u\n", info.compression);
    co_return;
  }

  unsigned px_in_tile = px % info.tile_width;
  unsigned py_in_tile = py % info.tile_height;
  unsigned pixel_offset = (py_in_tile * info.tile_width + px_in_tile) * 2;

  if (pixel_offset + 2 <= tile_data.size()) {
    int16_t elevation;
    std::memcpy(&elevation, tile_data.data() + pixel_offset, 2);
    printf("\nElevation at location: %d meters\n", elevation);

    printf("\nElevation grid around location (11x11, ~330m spacing):\n");
    for (int dy = -5; dy <= 5; ++dy) {
      printf("  ");
      for (int dx = -5; dx <= 5; ++dx) {
        int ppx = static_cast<int>(px_in_tile) + dx;
        int ppy = static_cast<int>(py_in_tile) + dy;
        if (ppx >= 0 && ppx < static_cast<int>(info.tile_width) &&
            ppy >= 0 && ppy < static_cast<int>(info.tile_height)) {
          unsigned off = (ppy * info.tile_width + ppx) * 2;
          if (off + 2 <= tile_data.size()) {
            int16_t elev;
            std::memcpy(&elev, tile_data.data() + off, 2);
            printf("%5d ", elev);
          } else {
            printf("  --- ");
          }
        } else {
          printf("  --- ");
        }
      }
      printf("\n");
    }
  } else {
    printf("Error: Pixel offset %u out of range (tile size %zu)\n",
           pixel_offset, tile_data.size());
  }

  printf("\nSuccess! COG terrain loading via HTTP range requests works.\n");
}

int
main(int argc, char **argv) noexcept
try {
  Args args(argc, argv, "LAT LON");
  const double lat = args.ExpectNextDouble();
  const double lon = args.ExpectNextDouble();
  args.ExpectEnd();

  if (lat < -56 || lat > 60) {
    fprintf(stderr, "Error: Latitude must be between -56 and 60 (SRTM coverage)\n");
    return EXIT_FAILURE;
  }

  printf("Testing COG terrain at %.6f, %.6f\n\n", lat, lon);

  std::string url = MakeTileUrl(lat, lon);
  GeoPoint location(Angle::Degrees(lon), Angle::Degrees(lat));

  Instance instance;
  instance.Run(FetchAndParseCog(*Net::curl, url.c_str(), location));

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
