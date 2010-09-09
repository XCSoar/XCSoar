#ifndef XCSOAR_RASTERTILE_HPP
#define XCSOAR_RASTERTILE_HPP

#include "Terrain/RasterBuffer.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/ActiveList.hpp"

#include <stddef.h>

class RasterTile : private NonCopyable {
  RasterBuffer buffer;

public:
  RasterTile()
    :xstart(0), ystart(0), xend(0), yend(0),
     width(0), height(0) {}
  ~RasterTile() {
    Disable();
  }

  unsigned int xstart, ystart, xend, yend;
  unsigned int width, height;
  bool request;

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
  const short* GetImageBuffer() const {
    return buffer.get_data();
  }
  bool VisibilityChanged(int view_x, int view_y);
};

#define MAX_RTC_TILES 1024
#define MAX_ACTIVE_TILES 16
#define RTC_SUBSAMPLING 16U
#define RTC_SUBSAMPLING_FINE (16*256)

class RasterTileCache : private NonCopyable {
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

public:
  gcc_pure
  short GetField(unsigned int lx,
                 unsigned int ly) const;

  void LoadJPG2000(const char *path);

  bool GetInitialised() const {
    return initialised;
  }

  void Reset();
  void SetInitialised(bool val);

  bool TileRequest(int index);

  short *GetOverview() {
    return Overview.get_data();
  }

  void SetSize(int width, int height);
  short* GetImageBuffer(int index);
  const short* GetImageBuffer(int index) const;
  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max);
  void SetTile(int index, int xstart, int ystart, int xend, int yend);
  bool PollTiles(int x, int y);

  short GetMaxElevation() const {
    return Overview.get_max();
  }

  double lat_min, lat_max, lon_min, lon_max;
  unsigned int GetWidth() const { return width; }
  unsigned int GetHeight() const { return height; }

private:
  unsigned int overview_width_fine, overview_height_fine;
};

#endif
