// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogLoader.hpp"
#include "RasterTileCache.hpp"
#include "RasterTraits.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"
#include "util/ByteOrder.hxx"
#include "LogFile.hpp"

#ifdef USE_LIBTIFF
#include <tiffio.h>
#endif

#ifdef USE_GEOTIFF
#include "Geo/GeoTIFFHeaders.hpp"
#endif

#ifdef HAVE_HTTP
#include "lib/curl/Global.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include <zlib.h>
#include <map>
#endif

#ifdef _WIN32
#include "system/UTF8Win32.hpp"
#endif

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cmath>

static constexpr uint16_t COG_TIFF_LITTLE_ENDIAN = 0x4949;
static constexpr uint16_t COG_TIFF_BIG_ENDIAN = 0x4D4D;
static constexpr uint16_t COG_TIFF_MAGIC = 42;

static constexpr uint16_t COG_TAG_IMAGE_WIDTH = 256;
static constexpr uint16_t COG_TAG_IMAGE_LENGTH = 257;
static constexpr uint16_t COG_TAG_BITS_PER_SAMPLE = 258;
static constexpr uint16_t COG_TAG_COMPRESSION = 259;
static constexpr uint16_t COG_TAG_SAMPLE_FORMAT = 339;
static constexpr uint16_t COG_TAG_TILE_WIDTH = 322;
static constexpr uint16_t COG_TAG_TILE_LENGTH = 323;
static constexpr uint16_t COG_TAG_TILE_OFFSETS = 324;
static constexpr uint16_t COG_TAG_TILE_BYTE_COUNTS = 325;
static constexpr uint16_t COG_TAG_MODEL_TIEPOINT = 33922;
static constexpr uint16_t COG_TAG_MODEL_PIXEL_SCALE = 33550;

static constexpr uint16_t COG_COMPRESSION_NONE = 1;
static constexpr uint16_t COG_COMPRESSION_DEFLATE = 8;
static constexpr uint16_t COG_COMPRESSION_ZLIB = 32946;

static constexpr uint16_t COG_TYPE_SHORT = 3;
static constexpr uint16_t COG_TYPE_LONG = 4;
static constexpr uint16_t COG_TYPE_DOUBLE = 12;
static constexpr uint16_t COG_TYPE_LONG8 = 16;

struct TileCacheKey {
  unsigned level;
  unsigned tx;
  unsigned ty;
  
  bool operator<(const TileCacheKey &other) const noexcept {
    if (level != other.level) return level < other.level;
    if (ty != other.ty) return ty < other.ty;
    return tx < other.tx;
  }
};

struct CogLoader::Impl {
#ifdef USE_LIBTIFF
  TIFF *tiff = nullptr;
#endif

  GeoBounds bounds;
  std::vector<CogLevel> levels;

#ifdef HAVE_HTTP
  CurlGlobal *curl = nullptr;
  std::string url;
  bool is_http = false;
  bool little_endian = true;
  uint16_t compression = COG_COMPRESSION_NONE;
  uint16_t bits_per_sample = 16;
  uint16_t sample_format = 1;
  
  mutable std::map<TileCacheKey, std::vector<TerrainHeight>> tile_cache;
  static constexpr size_t MAX_CACHED_TILES = 32;
#endif

  ~Impl() noexcept {
    Close();
  }

  void Close() noexcept {
#ifdef USE_LIBTIFF
    if (tiff != nullptr) {
      TIFFClose(tiff);
      tiff = nullptr;
    }
#endif
    levels.clear();
    bounds.SetInvalid();
#ifdef HAVE_HTTP
    curl = nullptr;
    url.clear();
    is_http = false;
#endif
  }

#ifdef HAVE_HTTP
  uint16_t ReadU16(const uint8_t *data) const noexcept {
    uint16_t v;
    std::memcpy(&v, data, 2);
    return little_endian ? FromLE16(v) : FromBE16(v);
  }

  uint32_t ReadU32(const uint8_t *data) const noexcept {
    uint32_t v;
    std::memcpy(&v, data, 4);
    return little_endian ? FromLE32(v) : FromBE32(v);
  }

  uint64_t ReadU64(const uint8_t *data) const noexcept {
    uint64_t v;
    std::memcpy(&v, data, 8);
    return little_endian ? FromLE64(v) : FromBE64(v);
  }

  double ReadDouble(const uint8_t *data) const noexcept {
    double v;
    std::memcpy(&v, data, 8);
    return v;
  }
#endif
};

CogLoader::CogLoader() noexcept
  :impl(std::make_unique<Impl>()) {}

CogLoader::~CogLoader() noexcept = default;

bool
CogLoader::IsCogFile(const char *path) noexcept
{
  return StringEndsWithIgnoreCase(path, ".tif") ||
         StringEndsWithIgnoreCase(path, ".tiff");
}

bool
CogLoader::IsHttpUrl(const char *path) noexcept
{
  return StringStartsWithIgnoreCase(path, "http://") ||
         StringStartsWithIgnoreCase(path, "https://");
}

#ifdef USE_LIBTIFF

static TIFF *
OpenTiff(Path path)
{
#ifdef USE_GEOTIFF
  XTIFFInitialize();
#endif

#ifdef _WIN32
  return TIFFOpenW(UTF8ToWide(path.c_str()).c_str(), "r");
#else
  return TIFFOpen(path.c_str(), "r");
#endif
}

#ifdef USE_GEOTIFF
static GeoBounds
ReadGeoBounds(TIFF *tiff)
{
  auto gtif = GTIFNew(tiff);
  if (gtif == nullptr)
    throw std::runtime_error("Not a GeoTIFF file");

  AtScopeExit(gtif) { GTIFFree(gtif); };

  GTIFDefn defn;
  if (!GTIFGetDefn(gtif, &defn))
    throw std::runtime_error("Failed to parse GeoTIFF metadata");

  uint32_t width, height;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);

  auto ToGeoPoint = [&](double x, double y) -> GeoPoint {
    if (!GTIFImageToPCS(gtif, &x, &y))
      return GeoPoint::Invalid();

    if (defn.Model != ModelTypeGeographic &&
        !GTIFProj4ToLatLong(&defn, 1, &x, &y))
      return GeoPoint::Invalid();

    return GeoPoint(Angle::Degrees(x), Angle::Degrees(y));
  };

  const GeoPoint top_left = ToGeoPoint(0, 0);
  const GeoPoint bottom_right = ToGeoPoint(width, height);

  if (!top_left.IsValid() || !bottom_right.IsValid())
    throw std::runtime_error("Invalid GeoTIFF bounds");

  return GeoBounds(top_left, bottom_right);
}
#endif

static CogLevel
ReadLevelInfo(TIFF *tiff, unsigned scale)
{
  CogLevel level;

  uint32_t width, height;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);

  level.width = width;
  level.height = height;
  level.scale = scale;

  if (TIFFIsTiled(tiff)) {
    uint32_t tw, th;
    TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tiff, TIFFTAG_TILELENGTH, &th);
    level.tile_width = tw;
    level.tile_height = th;
  } else {
    level.tile_width = width;
    level.tile_height = 1;
  }

  level.tiles_x = (level.width + level.tile_width - 1) / level.tile_width;
  level.tiles_y = (level.height + level.tile_height - 1) / level.tile_height;

  return level;
}

bool
CogLoader::Open(Path path)
{
  impl->Close();

  impl->tiff = OpenTiff(path);
  if (impl->tiff == nullptr)
    return false;

  try {
#ifdef USE_GEOTIFF
    impl->bounds = ReadGeoBounds(impl->tiff);
#else
    impl->bounds.SetInvalid();
#endif

    unsigned scale = 1;
    do {
      impl->levels.push_back(ReadLevelInfo(impl->tiff, scale));
      scale *= 2;
    } while (TIFFReadDirectory(impl->tiff));

    TIFFSetDirectory(impl->tiff, 0);

    if (impl->levels.empty()) {
      impl->Close();
      return false;
    }

    return true;
  } catch (...) {
    impl->Close();
    throw;
  }
}

#else

bool
CogLoader::Open([[maybe_unused]] Path path)
{
  return false;
}

#endif

bool
CogLoader::Open([[maybe_unused]] struct zzip_dir *dir, const char *path)
{
  return Open(Path(path));
}

#ifdef HAVE_HTTP

static std::string
HttpGetRange([[maybe_unused]] CurlGlobal &curl, const char *url,
             uint64_t offset, size_t length)
{
  CurlEasy easy{url};
  Curl::Setup(easy);
  easy.SetOption(CURLOPT_FOLLOWLOCATION, 1L);
  easy.SetOption(CURLOPT_MAXREDIRS, 10L);
  easy.SetFailOnError();
  easy.SetTimeout(30);

  char range_header[64];
  snprintf(range_header, sizeof(range_header), "%lu-%lu",
           (unsigned long)offset, (unsigned long)(offset + length - 1));
  easy.SetOption(CURLOPT_RANGE, range_header);

  easy.SetOption(CURLOPT_MAXFILESIZE_LARGE, curl_off_t(length + 4096));

  std::string response_body;
  easy.SetWriteFunction([](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
    auto *body = static_cast<std::string *>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
  }, &response_body);

  easy.Perform();

  long response_code = 0;
  easy.GetInfo(CURLINFO_RESPONSE_CODE, &response_code);
  if (response_code != 206 && response_code != 200)
    throw std::runtime_error("HTTP range request failed");

  return response_body;
}

bool
CogLoader::OpenHttp(CurlGlobal &curl, const char *url)
{
  impl->Close();
  impl->curl = &curl;
  impl->url = url;
  impl->is_http = true;

  try {
    auto header_data = HttpGetRange(curl, url, 0, 32768);
    if (header_data.size() < 8)
      throw std::runtime_error("TIFF header too short");

    const uint8_t *data = reinterpret_cast<const uint8_t *>(header_data.data());

    uint16_t byte_order;
    std::memcpy(&byte_order, data, 2);
    if (byte_order == COG_TIFF_LITTLE_ENDIAN) {
      impl->little_endian = true;
    } else if (byte_order == COG_TIFF_BIG_ENDIAN) {
      impl->little_endian = false;
    } else {
      throw std::runtime_error("Invalid TIFF byte order");
    }

    uint16_t magic = impl->ReadU16(data + 2);
    if (magic != COG_TIFF_MAGIC)
      throw std::runtime_error("Invalid TIFF magic");

    uint32_t ifd_offset = impl->ReadU32(data + 4);

    size_t current_size = header_data.size();
    while (ifd_offset > 0) {
      if (ifd_offset + 2 > current_size) {
        auto more = HttpGetRange(curl, url, current_size,
                                 ifd_offset + 4096 - current_size);
        header_data += more;
        data = reinterpret_cast<const uint8_t *>(header_data.data());
        current_size = header_data.size();
      }

      if (ifd_offset + 2 > current_size)
        break;

      uint16_t num_tags = impl->ReadU16(data + ifd_offset);
      size_t ifd_end = ifd_offset + 2 + num_tags * 12 + 4;

      if (ifd_end > current_size) {
        auto more = HttpGetRange(curl, url, current_size,
                                 ifd_end + 4096 - current_size);
        header_data += more;
        data = reinterpret_cast<const uint8_t *>(header_data.data());
        current_size = header_data.size();
      }

      CogLevel level;
      level.scale = impl->levels.empty() ? 1 : impl->levels.back().scale * 2;

      size_t tag_base = ifd_offset + 2;
      uint32_t tile_offsets_offset = 0;
      uint32_t tile_offsets_count = 0;
      uint16_t tile_offsets_type = COG_TYPE_LONG;
      uint32_t tile_counts_offset = 0;
      uint32_t tile_counts_count = 0;
      uint16_t tile_counts_type = COG_TYPE_LONG;
      double pixel_scale_x = 0, pixel_scale_y = 0;
      double tie_x = 0, tie_y = 0;

      for (unsigned i = 0; i < num_tags; ++i) {
        size_t tag_offset = tag_base + i * 12;
        if (tag_offset + 12 > current_size)
          break;

        uint16_t tag = impl->ReadU16(data + tag_offset);
        uint16_t type = impl->ReadU16(data + tag_offset + 2);
        uint32_t count = impl->ReadU32(data + tag_offset + 4);
        uint32_t value_or_offset = impl->ReadU32(data + tag_offset + 8);

        auto get_value = [&]() -> uint32_t {
          if (type == COG_TYPE_SHORT)
            return value_or_offset & 0xFFFF;
          return value_or_offset;
        };

        switch (tag) {
        case COG_TAG_IMAGE_WIDTH:
          level.width = get_value();
          break;
        case COG_TAG_IMAGE_LENGTH:
          level.height = get_value();
          break;
        case COG_TAG_TILE_WIDTH:
          level.tile_width = get_value();
          break;
        case COG_TAG_TILE_LENGTH:
          level.tile_height = get_value();
          break;
        case COG_TAG_BITS_PER_SAMPLE:
          impl->bits_per_sample = get_value();
          break;
        case COG_TAG_COMPRESSION:
          impl->compression = get_value();
          break;
        case COG_TAG_SAMPLE_FORMAT:
          impl->sample_format = get_value();
          break;
        case COG_TAG_TILE_OFFSETS:
          tile_offsets_offset = value_or_offset;
          tile_offsets_count = count;
          tile_offsets_type = type;
          break;
        case COG_TAG_TILE_BYTE_COUNTS:
          tile_counts_offset = value_or_offset;
          tile_counts_count = count;
          tile_counts_type = type;
          break;
        case COG_TAG_MODEL_PIXEL_SCALE:
          if (count >= 2 && value_or_offset + 16 <= current_size) {
            pixel_scale_x = impl->ReadDouble(data + value_or_offset);
            pixel_scale_y = impl->ReadDouble(data + value_or_offset + 8);
          }
          break;
        case COG_TAG_MODEL_TIEPOINT:
          if (count >= 6 && value_or_offset + 48 <= current_size) {
            tie_x = impl->ReadDouble(data + value_or_offset + 24);
            tie_y = impl->ReadDouble(data + value_or_offset + 32);
          }
          break;
        }
      }

      if (level.tile_width == 0)
        level.tile_width = level.width;
      if (level.tile_height == 0)
        level.tile_height = 1;

      level.tiles_x = level.tile_width > 0
        ? (level.width + level.tile_width - 1) / level.tile_width
        : 0;
      level.tiles_y = level.tile_height > 0
        ? (level.height + level.tile_height - 1) / level.tile_height
        : 0;

      unsigned num_tiles = level.tiles_x * level.tiles_y;

      if (tile_offsets_count == num_tiles && tile_offsets_offset > 0) {
        size_t elem_size = (tile_offsets_type == COG_TYPE_LONG8) ? 8 : 4;
        size_t offsets_end = tile_offsets_offset + num_tiles * elem_size;
        if (offsets_end > current_size) {
          auto more = HttpGetRange(curl, url, current_size,
                                   offsets_end + 256 - current_size);
          header_data += more;
          data = reinterpret_cast<const uint8_t *>(header_data.data());
          current_size = header_data.size();
        }

        level.tile_offsets.resize(num_tiles);
        for (unsigned j = 0; j < num_tiles && tile_offsets_offset + j * elem_size + elem_size <= current_size; ++j) {
          if (elem_size == 8)
            level.tile_offsets[j] = impl->ReadU64(data + tile_offsets_offset + j * 8);
          else
            level.tile_offsets[j] = impl->ReadU32(data + tile_offsets_offset + j * 4);
        }
      }

      if (tile_counts_count == num_tiles && tile_counts_offset > 0) {
        size_t elem_size = (tile_counts_type == COG_TYPE_LONG8) ? 8 : 4;
        size_t counts_end = tile_counts_offset + num_tiles * elem_size;
        if (counts_end > current_size) {
          auto more = HttpGetRange(curl, url, current_size,
                                   counts_end + 256 - current_size);
          header_data += more;
          data = reinterpret_cast<const uint8_t *>(header_data.data());
          current_size = header_data.size();
        }

        level.tile_byte_counts.resize(num_tiles);
        for (unsigned j = 0; j < num_tiles && tile_counts_offset + j * elem_size + elem_size <= current_size; ++j) {
          if (elem_size == 8)
            level.tile_byte_counts[j] = impl->ReadU64(data + tile_counts_offset + j * 8);
          else
            level.tile_byte_counts[j] = impl->ReadU32(data + tile_counts_offset + j * 4);
        }
      }

      if (impl->levels.empty() && pixel_scale_x > 0 && pixel_scale_y > 0) {
        GeoPoint top_left(Angle::Degrees(tie_x), Angle::Degrees(tie_y));
        GeoPoint bottom_right(
          Angle::Degrees(tie_x + level.width * pixel_scale_x),
          Angle::Degrees(tie_y - level.height * pixel_scale_y));
        impl->bounds = GeoBounds(top_left, bottom_right);
      }

      impl->levels.push_back(std::move(level));

      uint32_t next_ifd = impl->ReadU32(data + ifd_offset + 2 + num_tags * 12);
      if (next_ifd == 0 || next_ifd <= ifd_offset)
        break;
      ifd_offset = next_ifd;
    }

    if (impl->levels.empty()) {
      impl->Close();
      return false;
    }

    LogFmt("COG HTTP: {}x{}, {} levels, tile {}x{}, compression={}",
           impl->levels[0].width, impl->levels[0].height,
           impl->levels.size(),
           impl->levels[0].tile_width, impl->levels[0].tile_height,
           impl->compression);

    if (impl->levels.size() > 1) {
      unsigned overview_level = impl->levels.size() - 1;
      const CogLevel &lev = impl->levels[overview_level];
      const unsigned tile_pixels = lev.tile_width * lev.tile_height;

      for (unsigned ty = 0; ty < lev.tiles_y && ty < 4; ++ty) {
        for (unsigned tx = 0; tx < lev.tiles_x && tx < 4; ++tx) {
          std::vector<TerrainHeight> tile_buffer(tile_pixels);
          if (const_cast<CogLoader *>(this)->ReadTile(overview_level, tx, ty, tile_buffer.data())) {
            TileCacheKey key{overview_level, tx, ty};
            impl->tile_cache[key] = std::move(tile_buffer);
          }
        }
      }
    }

    return true;
  } catch (...) {
    impl->Close();
    throw;
  }
}

#else

bool
CogLoader::OpenHttp([[maybe_unused]] CurlGlobal &curl,
                    [[maybe_unused]] const char *url)
{
  return false;
}

#endif

void
CogLoader::Close() noexcept
{
  impl->Close();
}

bool
CogLoader::IsOpen() const noexcept
{
#ifdef USE_LIBTIFF
  if (impl->tiff != nullptr)
    return true;
#endif
#ifdef HAVE_HTTP
  if (impl->is_http && !impl->levels.empty())
    return true;
#endif
  return false;
}

bool
CogLoader::IsHttp() const noexcept
{
#ifdef HAVE_HTTP
  return impl->is_http;
#else
  return false;
#endif
}

const GeoBounds &
CogLoader::GetBounds() const noexcept
{
  return impl->bounds;
}

UnsignedPoint2D
CogLoader::GetSize() const noexcept
{
  if (impl->levels.empty())
    return {0, 0};
  return {impl->levels[0].width, impl->levels[0].height};
}

unsigned
CogLoader::GetLevelCount() const noexcept
{
  return impl->levels.size();
}

const CogLevel &
CogLoader::GetLevel(unsigned level) const noexcept
{
  return impl->levels[level];
}

unsigned
CogLoader::SelectLevel(double meters_per_pixel) const noexcept
{
  if (impl->levels.empty())
    return 0;

  if (!impl->bounds.IsValid())
    return 0;

  const double terrain_meters_per_pixel =
    impl->bounds.GetHeight().IsPositive()
    ? impl->bounds.GetHeight().Degrees() * 111000.0 / impl->levels[0].height
    : 90.0;

  const double target_ratio = meters_per_pixel / terrain_meters_per_pixel;

  for (unsigned i = 0; i < impl->levels.size(); ++i) {
    if (impl->levels[i].scale >= target_ratio)
      return i;
  }

  return impl->levels.size() - 1;
}

UnsignedPoint2D
CogLoader::GetTileSize(unsigned level) const noexcept
{
  if (level >= impl->levels.size())
    return {0, 0};
  return {impl->levels[level].tile_width, impl->levels[level].tile_height};
}

const std::string &
CogLoader::GetUrl() const noexcept
{
#ifdef HAVE_HTTP
  return impl->url;
#else
  static const std::string empty;
  return empty;
#endif
}

bool
CogLoader::ReadTile(unsigned level, unsigned tx, unsigned ty,
                    TerrainHeight *buffer) noexcept
{
  if (!IsOpen() || level >= impl->levels.size())
    return false;

  const CogLevel &lev = impl->levels[level];
  if (tx >= lev.tiles_x || ty >= lev.tiles_y)
    return false;

#ifdef USE_LIBTIFF
  if (impl->tiff != nullptr) {
    if (TIFFSetDirectory(impl->tiff, level) == 0)
      return false;

    if (!TIFFIsTiled(impl->tiff))
      return false;

    uint16_t bits_per_sample = 0;
    uint16_t sample_format = SAMPLEFORMAT_UINT;
    TIFFGetField(impl->tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    TIFFGetField(impl->tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);

    const tmsize_t tile_size = TIFFTileSize(impl->tiff);
    if (tile_size <= 0)
      return false;

    std::vector<uint8_t> raw_buffer(tile_size);

    const ttile_t tile_index = TIFFComputeTile(impl->tiff,
                                               tx * lev.tile_width,
                                               ty * lev.tile_height,
                                               0, 0);

    if (TIFFReadEncodedTile(impl->tiff, tile_index,
                            raw_buffer.data(), tile_size) < 0)
      return false;

    const unsigned tile_pixels = lev.tile_width * lev.tile_height;

    if (bits_per_sample == 16) {
      const int16_t *src = reinterpret_cast<const int16_t *>(raw_buffer.data());
      for (unsigned i = 0; i < tile_pixels; ++i) {
        int16_t value = src[i];
        if (value == -32768)
          buffer[i] = TerrainHeight::Invalid();
        else
          buffer[i] = TerrainHeight(value);
      }
    } else if (bits_per_sample == 32 && sample_format == SAMPLEFORMAT_IEEEFP) {
      const float *src = reinterpret_cast<const float *>(raw_buffer.data());
      for (unsigned i = 0; i < tile_pixels; ++i) {
        float value = src[i];
        if (std::isnan(value) || value < -32767 || value > 32767)
          buffer[i] = TerrainHeight::Invalid();
        else
          buffer[i] = TerrainHeight(static_cast<int16_t>(value));
      }
    } else {
      std::fill_n(buffer, tile_pixels, TerrainHeight::Invalid());
      return false;
    }

    return true;
  }
#endif

#ifdef HAVE_HTTP
  if (impl->is_http && impl->curl != nullptr) {
    unsigned tile_index = ty * lev.tiles_x + tx;
    if (tile_index >= lev.tile_offsets.size() ||
        tile_index >= lev.tile_byte_counts.size())
      return false;

    uint64_t offset = lev.tile_offsets[tile_index];
    uint64_t size = lev.tile_byte_counts[tile_index];

    if (offset == 0 || size == 0 || size > 10 * 1024 * 1024)
      return false;

    try {
      auto tile_data = HttpGetRange(*impl->curl, impl->url.c_str(), offset, size);

      const unsigned tile_pixels = lev.tile_width * lev.tile_height;
      std::string decompressed;

      if (impl->compression == COG_COMPRESSION_DEFLATE ||
          impl->compression == COG_COMPRESSION_ZLIB) {
        size_t expected_size = tile_pixels * (impl->bits_per_sample / 8);
        decompressed.resize(expected_size);

        z_stream strm{};
        strm.next_in = reinterpret_cast<Bytef *>(tile_data.data());
        strm.avail_in = tile_data.size();
        strm.next_out = reinterpret_cast<Bytef *>(decompressed.data());
        strm.avail_out = decompressed.size();

        if (inflateInit(&strm) != Z_OK)
          return false;

        int ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);

        if (ret != Z_STREAM_END)
          return false;

        tile_data = std::move(decompressed);
      } else if (impl->compression != COG_COMPRESSION_NONE) {
        return false;
      }

      if (impl->bits_per_sample == 16) {
        if (tile_data.size() < tile_pixels * 2)
          return false;
        const int16_t *src = reinterpret_cast<const int16_t *>(tile_data.data());
        for (unsigned i = 0; i < tile_pixels; ++i) {
          int16_t value = src[i];
          if (value == -32768)
            buffer[i] = TerrainHeight::Invalid();
          else
            buffer[i] = TerrainHeight(value);
        }
      } else if (impl->bits_per_sample == 32 && impl->sample_format == 3) {
        if (tile_data.size() < tile_pixels * 4)
          return false;
        const float *src = reinterpret_cast<const float *>(tile_data.data());
        for (unsigned i = 0; i < tile_pixels; ++i) {
          float value = src[i];
          if (std::isnan(value) || value < -32767 || value > 32767)
            buffer[i] = TerrainHeight::Invalid();
          else
            buffer[i] = TerrainHeight(static_cast<int16_t>(value));
        }
      } else {
        std::fill_n(buffer, tile_pixels, TerrainHeight::Invalid());
        return false;
      }

      return true;
    } catch (...) {
      return false;
    }
  }
#endif

  return false;
}

bool
IsCogTerrainFile(const char *filename) noexcept
{
  return CogLoader::IsCogFile(filename);
}

void
LoadCogTerrain(Path path, RasterTileCache &cache,
               OperationEnvironment &env)
{
  CogLoader loader;
  if (!loader.Open(path))
    throw std::runtime_error("Failed to open COG terrain file");

  env.SetProgressRange(100);
  env.SetProgressPosition(10);

  const UnsignedPoint2D size = loader.GetSize();
  if (size.x == 0 || size.y == 0)
    throw std::runtime_error("COG terrain has zero size");

  const GeoBounds &bounds = loader.GetBounds();
  if (!bounds.IsValid())
    throw std::runtime_error("COG terrain has no valid bounds");
  cache.SetBounds(bounds);

  const unsigned level_count = loader.GetLevelCount();
  if (level_count == 0)
    throw std::runtime_error("COG has no pyramid levels");

  const UnsignedPoint2D tile_size = loader.GetTileSize(0);
  const CogLevel &full_level = loader.GetLevel(0);

  cache.SetSize({size.x, size.y},
                {static_cast<uint_least16_t>(tile_size.x),
                 static_cast<uint_least16_t>(tile_size.y)},
                {full_level.tiles_x, full_level.tiles_y});

  env.SetProgressPosition(50);

  LogFmt("COG terrain: {}x{}, {} levels, tile {}x{}, {} tiles",
         size.x, size.y, level_count,
         tile_size.x, tile_size.y,
         full_level.tiles_x * full_level.tiles_y);

  env.SetProgressPosition(100);
}

void
LoadCogTerrain(struct zzip_dir *dir, const char *path,
               RasterTileCache &cache,
               OperationEnvironment &env)
{
  (void)dir;
  LoadCogTerrain(Path(path), cache, env);
}

TerrainHeight
CogLoader::GetHeight(const GeoPoint &location,
                     unsigned quantisation) const noexcept
{
  if (!IsOpen() || impl->levels.empty() || !impl->bounds.IsValid())
    return TerrainHeight::Invalid();

  if (!impl->bounds.IsInside(location))
    return TerrainHeight::Invalid();

  unsigned level = 0;
  if (quantisation > 1 && impl->levels.size() > 1) {
    for (unsigned i = 1; i < impl->levels.size(); ++i) {
      if (impl->levels[i].scale <= quantisation)
        level = i;
      else
        break;
    }
  }
  const CogLevel &lev = impl->levels[level];

  const double lat_ratio = (impl->bounds.GetNorth().Degrees() - location.latitude.Degrees()) /
                           impl->bounds.GetHeight().Degrees();
  const double lon_ratio = (location.longitude.Degrees() - impl->bounds.GetWest().Degrees()) /
                           impl->bounds.GetWidth().Degrees();

  const unsigned px = static_cast<unsigned>(lon_ratio * lev.width);
  const unsigned py = static_cast<unsigned>(lat_ratio * lev.height);

  if (px >= lev.width || py >= lev.height)
    return TerrainHeight::Invalid();

  const unsigned tx = px / lev.tile_width;
  const unsigned ty = py / lev.tile_height;

#ifdef HAVE_HTTP
  if (impl->is_http) {
    TileCacheKey key{level, tx, ty};
    auto it = impl->tile_cache.find(key);
    if (it != impl->tile_cache.end()) {
      const unsigned local_x = px % lev.tile_width;
      const unsigned local_y = py % lev.tile_height;
      const unsigned idx = local_y * lev.tile_width + local_x;
      return it->second[idx];
    }

    return TerrainHeight::Invalid();
  }
#endif

  const unsigned tile_pixels = lev.tile_width * lev.tile_height;
  std::vector<TerrainHeight> tile_buffer(tile_pixels);

  if (!const_cast<CogLoader *>(this)->ReadTile(level, tx, ty, tile_buffer.data()))
    return TerrainHeight::Invalid();

  const unsigned local_x = px % lev.tile_width;
  const unsigned local_y = py % lev.tile_height;
  const unsigned idx = local_y * lev.tile_width + local_x;

  return tile_buffer[idx];
}

unsigned
CogLoader::PreCacheTiles([[maybe_unused]] const GeoBounds &bounds,
                         [[maybe_unused]] unsigned quantisation) noexcept
{
  return 0;
}

bool
CogLoader::IsInternalTileCached([[maybe_unused]] unsigned level,
                                [[maybe_unused]] unsigned tx,
                                [[maybe_unused]] unsigned ty) const noexcept
{
#ifdef HAVE_HTTP
  if (impl->is_http) {
    TileCacheKey key{level, tx, ty};
    return impl->tile_cache.find(key) != impl->tile_cache.end();
  }
#endif
  return true;
}

bool
CogLoader::LoadInternalTile([[maybe_unused]] unsigned level,
                            [[maybe_unused]] unsigned tx,
                            [[maybe_unused]] unsigned ty) noexcept
{
#ifdef HAVE_HTTP
  if (!impl->is_http || level >= impl->levels.size())
    return false;

  const CogLevel &lev = impl->levels[level];
  if (tx >= lev.tiles_x || ty >= lev.tiles_y)
    return false;

  TileCacheKey key{level, tx, ty};
  if (impl->tile_cache.find(key) != impl->tile_cache.end())
    return true;

  const unsigned tile_pixels = lev.tile_width * lev.tile_height;
  std::vector<TerrainHeight> tile_buffer(tile_pixels);

  if (!ReadTile(level, tx, ty, tile_buffer.data()))
    return false;

  if (impl->tile_cache.size() >= Impl::MAX_CACHED_TILES)
    impl->tile_cache.erase(impl->tile_cache.begin());

  impl->tile_cache[key] = std::move(tile_buffer);
  return true;
#else
  return false;
#endif
}

void
UpdateCogTiles([[maybe_unused]] struct zzip_dir *dir,
               [[maybe_unused]] const char *path,
               [[maybe_unused]] RasterTileCache &cache,
               [[maybe_unused]] SharedMutex &mutex,
               [[maybe_unused]] SignedRasterLocation center,
               [[maybe_unused]] unsigned radius)
{
}
