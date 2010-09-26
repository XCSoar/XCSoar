#ifndef XCSOAR_RASTERTILE_HPP
#define XCSOAR_RASTERTILE_HPP

#include "Terrain/RasterBuffer.hpp"
#include "Geo/BoundsRectangle.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/ActiveList.hpp"
#include "Util/StaticArray.hpp"

#include <tchar.h>
#include <stddef.h>
#include <stdio.h>

class RasterTile : private NonCopyable {
  struct MetaData {
    unsigned int xstart, ystart, xend, yend;
  };

  unsigned int xstart, ystart, xend, yend;
  unsigned int width, height;
  bool request;

  RasterBuffer buffer;

public:
  RasterTile()
    :xstart(0), ystart(0), xend(0), yend(0),
     width(0), height(0) {}
  ~RasterTile() {
    Disable();
  }

  void set(unsigned _xstart, unsigned _ystart,
           unsigned _xend, unsigned _yend) {
    xstart = _xstart;
    ystart = _ystart;
    xend = _xend;
    yend = _yend;
    width = xend - xstart;
    height = yend - ystart;
  }

  bool defined() const {
    return width > 0 && height > 0;
  }

  bool is_requested() const {
    return request;
  }

  bool SaveCache(FILE *file) const;
  bool LoadCache(FILE *file);

  bool CheckTileVisibility(const int view_x, const int view_y);

  void Disable() {
    buffer.reset();
  }

  void Enable();
  bool IsEnabled() const {
    return buffer.defined();
  }
  bool IsDisabled() const {
    return !buffer.defined();
  }

  gcc_pure
  short GetField(unsigned x, unsigned y, unsigned ix, unsigned iy) const;

  inline short* GetImageBuffer() {
    return buffer.get_data();
  }

  bool VisibilityChanged(int view_x, int view_y);
};

#define MAX_ACTIVE_TILES 16

class RasterTileCache : private NonCopyable {
  static const unsigned MAX_RTC_TILES = 4096;
  static const unsigned RTC_SUBSAMPLING = 16;

  struct MarkerSegmentInfo {
    MarkerSegmentInfo() {}
    MarkerSegmentInfo(long _file_offset, int _tile=-1)
      :file_offset(_file_offset), tile(_tile) {}

    /**
     * The position of this marker segment within the file.
     */
    long file_offset;

    /**
     * The associated tile number.  -1 if this segment does not belong
     * to a tile.
     */
    int tile;
  };

  struct CacheHeader {
    enum { VERSION = 0x1 };
    unsigned version, width, height, num_marker_segments;
    BoundsRectangle bounds;
  };

  StaticArray<MarkerSegmentInfo, 8192> segments;

public:
  RasterTileCache()
    :scan_overview(true) {
    Reset();
  }

private:
  bool initialised;
  RasterTile tiles[MAX_RTC_TILES];
  mutable ActiveList<const RasterTile, MAX_ACTIVE_TILES> ActiveTiles;
  RasterBuffer Overview;
  bool scan_overview;
  unsigned int width, height;
  BoundsRectangle bounds;

public:
  gcc_pure
  short GetField(unsigned int lx,
                 unsigned int ly) const;

protected:
  void LoadJPG2000(const char *path);

  /**
   * Load a world file (*.tfw or *.j2w).
   */
  bool LoadWorldFile(const TCHAR *path);

public:
  bool LoadOverview(const char *path, const TCHAR *world_file);

  bool SaveCache(FILE *file) const;
  bool LoadCache(FILE *file);

  void UpdateTiles(const char *path, int x, int y);

  bool GetInitialised() const {
    return initialised;
  }

  void Reset();

  const BoundsRectangle &GetBounds() const {
    return bounds;
  }

private:
  gcc_pure
  const MarkerSegmentInfo *
  FindMarkerSegment(long file_offset) const;

public:
  /* callback methods for libjasper (via jas_rtc.cpp) */

  long SkipMarkerSegment(long file_offset) const;
  void MarkerSegment(long file_offset, unsigned id);

  bool TileRequest(unsigned index);

  short *GetOverview() {
    return Overview.get_data();
  }

  void SetSize(unsigned width, unsigned height);
  short* GetImageBuffer(unsigned index);
  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max);
  void SetTile(unsigned index, int xstart, int ystart, int xend, int yend);

  void SetInitialised(bool val) {
    initialised = val;
  }

protected:
  bool PollTiles(int x, int y);

public:
  short GetMaxElevation() const {
    return Overview.get_max();
  }

  unsigned int GetWidth() const { return width; }
  unsigned int GetHeight() const { return height; }

private:
  unsigned int overview_width_fine, overview_height_fine;
};

#endif
