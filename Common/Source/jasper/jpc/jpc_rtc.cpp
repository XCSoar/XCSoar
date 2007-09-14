#include "jasper/jpc_rtc.h"
#include "jasper/RasterTile.h"

RasterTileCache *raster_tile_current = 0;

extern void StepProgressDialog(void);

extern "C" {

  void jas_rtc_stepprogress(void) {
    StepProgressDialog();
  }

  void jas_rtc_SetTile(int index, 
                       int xstart, int ystart, 
                       int xend, int yend) {
    raster_tile_current->SetTile(index, xstart, ystart, xend, yend);
  }
  
  bool jas_rtc_TileRequest(int index) {
    return raster_tile_current->TileRequest(index);
  }

  /*
  bool jas_rtc_PollTiles(int view_x, int view_y) {
    return raster_tile_current->PollTiles(view_x, view_y);
  }
  */
  
  short* jas_rtc_GetImageBuffer(int index) {
    return raster_tile_current->GetImageBuffer(index);
  }

  void jas_rtc_SetLatLonBounds(double lon_min, double lon_max, 
                               double lat_min, double lat_max) {
    raster_tile_current->SetLatLonBounds(lon_min, lon_max, lat_min, lat_max);
  }

  void jas_rtc_SetSize(int width, 
                       int height) {
    raster_tile_current->SetSize(width, height);
  }

  void jas_rtc_SetInitialised(bool val) {
    raster_tile_current->SetInitialised(val);
  }

  /*
  bool jas_rtc_GetInitialised(void) {
    return raster_tile_current->GetInitialised();
  }
  */

  int jas_rtc_GetScanType(void) {
    return raster_tile_current->GetScanType();
  }

  short* jas_rtc_GetOverview(void) {
    return raster_tile_current->GetOverview();
  }
};
