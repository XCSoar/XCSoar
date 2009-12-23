#ifndef RASTERTILE_H
#define RASTERTILE_H

#include <stdlib.h>

class RasterTile {
public:
  RasterTile() {
    xstart = ystart = xend = yend = 0;
    width = height = 0;
    ImageBuffer = NULL;
  }

  unsigned int xstart, ystart, xend, yend;
  unsigned int width, height;
  bool request;
  short *ImageBuffer;

  int CheckTileVisibility(int view_x, int view_y);
  bool SetEdgeIfInRange(unsigned int x, unsigned int y, short val);

 public:
  void Disable();
  void Enable();
  inline  bool IsEnabled() {
    return (ImageBuffer != NULL);
  }
  inline  bool IsDisabled() {
    return (ImageBuffer == NULL);
  }
  bool GetField(unsigned int x, unsigned int y,
                short *theight);
  inline short* GetImageBuffer() {
    return ImageBuffer;
  }
  bool VisibilityChanged(int view_x, int view_y);
};

#define MAX_RTC_TILES 1024
#define MAX_ACTIVE_TILES 16
#define RTC_SUBSAMPLING 16U
#define RTC_SUBSAMPLING_FINE (16*256)

class RasterTileCache {
public:

  RasterTileCache() {
    Overview = NULL;
    scan_overview = true;
    Reset();
  };
  ~RasterTileCache() {
    Reset();
  }

private:
  bool initialised;
  bool loaded_one;
private:
  int view_x;
  int view_y;
  RasterTile tiles[MAX_RTC_TILES];
  int tile_last;
  int ActiveTiles[MAX_ACTIVE_TILES];
  short* Overview;
  bool scan_overview;
  unsigned int width, height;
public:
  bool GetScanType(void);
  short GetField(unsigned int lx,
                 unsigned int ly);
  void LoadJPG2000(char* jp2_filename);
  bool GetInitialised(void);
  void Reset();
  void SetInitialised(bool val);

  bool TileRequest(int index);
  short* GetOverview(void);
  void SetSize(int width, int height);
  short* GetImageBuffer(int index);
  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max);
  void SetTile(int index, int xstart, int ystart, int xend, int yend);
  bool PollTiles(int x, int y);
  short GetMaxElevation(void);

  double lat_min, lat_max, lon_min, lon_max;
  unsigned int GetWidth() { return width; }
  unsigned int GetHeight() { return height; }
 private:
  unsigned int overview_width, overview_height;
  unsigned int overview_width_fine, overview_height_fine;

  short GetOverviewField(unsigned int lx,
                         unsigned int ly);

  void StitchTiles(void);
 private:
  void StitchTile(unsigned int i);

};

#endif
