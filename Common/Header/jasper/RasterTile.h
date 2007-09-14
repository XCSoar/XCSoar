#ifndef RASTERTILE_H
#define RASTERTILE_H

class RasterTile {
public:
  RasterTile() {
    xstart = ystart = xend = yend = 0;
    width = height = 0;
    ImageBuffer = 0;
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
    return (ImageBuffer != 0);
  }
  inline  bool IsDisabled() {
    return (ImageBuffer == 0);
  }
  bool GetField(unsigned int x, unsigned int y,
                short *theight);
  short* GetImageBuffer();
  bool VisibilityChanged(int view_x, int view_y);
};

#define MAX_RTC_TILES 1024
#define MAX_ACTIVE_TILES 16
#define RTC_SUBSAMPLING 16U
#define RTC_SUBSAMPLING_FINE (16*256)

class RasterTileCache {
public:

  RasterTileCache() {
    Overview = 0;
    scan_overview = true;
    Reset();
  };
  ~RasterTileCache() {
    Reset();
  }

private:
  int view_x;
  int view_y;
  bool initialised;
  bool scan_overview;
  bool loaded_one;
private:
  RasterTile tiles[MAX_RTC_TILES];
  int tile_last;
  int ActiveTiles[MAX_ACTIVE_TILES];
  short* Overview;
public:
  bool GetScanType(void);
  short* GetOverview(void);
  double lat_min, lat_max, lon_min, lon_max;
  unsigned int width, height;
  unsigned int overview_width, overview_height;
  unsigned int overview_width_fine, overview_height_fine;
  // public only for debugging!

  void Reset();
  short* GetImageBuffer(int index);
  bool PollTiles(int x, int y);
  void SetTile(int index, int xstart, int ystart, int xend, int yend);
  bool TileRequest(int index);

  short GetField(unsigned int lx,
                 unsigned int ly);
  short GetOverviewField(unsigned int lx,
                         unsigned int ly);
  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max);
  void SetSize(int width, int height);
  void SetInitialised(bool val);
  bool GetInitialised(void);
  short GetMaxElevation(void);

  void StitchTiles(void);
  void LoadJPG2000(char* jp2_filename);
 private:
  void StitchTile(unsigned int i);

};

#endif
