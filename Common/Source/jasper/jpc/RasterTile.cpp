#include "jasper/jasper.h"
#include "jasper/RasterTile.h"
#include "Sizes.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef __MINGW32__
#define min(x, y) \
        (((x) < (y)) ? (x) : (y))

/* Compute the maximum of two values. */
#define max(x, y) \
        (((x) > (y)) ? (x) : (y))
#endif

inline unsigned int CombinedDivAndMod(unsigned int &lx) {
  unsigned int ox = lx & 0xff;
  // JMW no need to check max since overflow will result in 
  // beyond max dimensions
  lx = lx>>8;
  return ox;
}


void RasterTile::Disable() {
  if (ImageBuffer) {
    free(ImageBuffer);
    ImageBuffer = NULL;    
  }
}

void RasterTile::Enable() {
  if (!width || !height) {
    Disable();
  } else {
    ImageBuffer = (short*)malloc((width+1)*(height+1)*sizeof(short));    
  }
}


bool RasterTile::SetEdgeIfInRange(unsigned int x, unsigned int y, 
                                  short val)
{
  if (!ImageBuffer)
    return false;
  if ((x-= xstart)>width) {
    return false;
  }
  if ((y-= ystart)>height) {
    return false;
  }
  ImageBuffer[y*(width+1)+x]= val;
  return true;
}


bool RasterTile::GetField(unsigned int lx, 
                          unsigned int ly, 
                          short *theight) {

  // we want to exit out of this function as soon as possible
  // if we have the wrong tile

  if (IsDisabled()) 
    return false;

  // check x in range, and decompose fraction part
  int ix = CombinedDivAndMod(lx);
  if ((lx-= xstart)>=width) 
    return false;

  // check y in range, and decompose fraction part
  int iy = CombinedDivAndMod(ly);
  if ((ly-= ystart)>=height) 
    return false;

  // perform piecewise linear interpolation
  const unsigned int dx= 1;
  const unsigned int dy= width+1;

  const short *tm = ImageBuffer+ly*dy+lx;

  if (ly==height-1) {
    short hy = tm[dy];
    hy++;
  }
  if (lx==width-1) {
    short hx = tm[dx];
    hx++;
  }

  if (ix>iy) {
    // lower triangle 
    *theight = *tm+((ix*(tm[dx]-*tm)+iy*(tm[dx+dy]-tm[dx]))/256);
    return true;
  } else {
    // upper triangle
    *theight = *tm+((iy*(tm[dy]-*tm)+ix*(tm[dx+dy]-tm[dy]))/256);
    return true;
  }
}


int RasterTile::CheckTileVisibility(int view_x, int view_y) {
  if (!width || !height) {
    if (IsEnabled()) {
      Disable();
    }
    return 0;
  }

  unsigned int dx1 = abs(view_x - xstart);
  unsigned int dx2 = abs(xend - view_x);
  unsigned int dy1 = abs(view_y - ystart);
  unsigned int dy2 = abs(yend - view_y);
  
  if (min(dx1,dx2)*2 < width*3) {
    if (min(dy1,dy2) < height) {
      return 1;
    }
  } 
  if (min(dy1,dy2)*2 < height*3) {
    if (min(dx1,dx2) < width) {
      return 1;
    }
  } 
  if ( (max(dx1,dx2) > width*2)
       || (max(dy1,dy2) > height*2)) {
    if (IsEnabled()) {
      Disable();
    }
    return 0;
  } 
  return 0;
}


bool RasterTile::VisibilityChanged(int view_x, int view_y) {
  request = (IsDisabled() && CheckTileVisibility(view_x, view_y));
  return request;
}


short* RasterTileCache::GetImageBuffer(int index) {
  if (index< MAX_RTC_TILES) {
    return tiles[index].GetImageBuffer();
  } else {
    return NULL;
  }
}


void RasterTileCache::SetTile(int index, 
                              int xstart, int ystart, 
                              int xend, int yend) {
  if (index<MAX_RTC_TILES) {
    tiles[index].xstart = xstart;
    tiles[index].ystart = ystart;
    tiles[index].xend = xend;
    tiles[index].yend = yend;
    tiles[index].width = tiles[index].xend-tiles[index].xstart;
    tiles[index].height = tiles[index].yend-tiles[index].ystart;
  } else {
    int j=index;
    j++;
    // error!
  }
}


bool RasterTileCache::PollTiles(int x, int y) {
  bool retval = false;
  int i, num_used=0;
  view_x = x;
  view_y = y;

  if (scan_overview) {
    return false;
  }

  for (i= MAX_ACTIVE_TILES; i--; ) {
    ActiveTiles[i] = -1;
  }
  for (i= MAX_RTC_TILES; i--; ) {
    retval |= tiles[i].VisibilityChanged(view_x, view_y);
    if (tiles[i].IsEnabled()) {
      ActiveTiles[num_used] = i;
      num_used++;
    }
  }
  return retval;
}


bool RasterTileCache::TileRequest(int index) {
  bool retval = 0;
  int num_used = 0;

  if (index>=MAX_RTC_TILES) {
    // tile index too big!
    return false;
  }

  retval = tiles[index].request;
  if (!retval) return false;

  for (int i=0; i< MAX_RTC_TILES; i++) {
    if (tiles[i].IsEnabled()) {
      num_used++;
    }
  }

  if (loaded_one) {
    return false; // already loaded one
  }
  
  if (num_used< MAX_ACTIVE_TILES) {
    loaded_one = true;
    tiles[index].Enable();
    return true; // want to load this one!
  } else {
    return false; // not enough memory for it or not visible anyway
  }
};


short RasterTileCache::GetField(unsigned int lx, 
                                unsigned int ly) {

  if ((lx>= overview_width_fine) ||
      (ly>= overview_height_fine)) {
    // outside overall bounds
    return TERRAIN_INVALID;
  }

  short retval;

  // search starting from last found tile
  if (tiles[tile_last].GetField(lx, ly, &retval)) {
    return retval;
  }

  int tile_this;
  for (unsigned int i= MAX_ACTIVE_TILES; i--; ) {
    if (((tile_this = ActiveTiles[i])>=0) &&
        (tile_this != tile_last) && 
        tiles[tile_this].GetField(lx, ly, &retval)) {
      tile_last = tile_this;
      return retval;
    }
  } 
  // still not found, so go to overview
  if (Overview) {
    return GetOverviewField(lx/RTC_SUBSAMPLING, ly/RTC_SUBSAMPLING);
  } else {
    return TERRAIN_INVALID;
  }
};

short RasterTileCache::GetOverviewField(unsigned int lx,
                                        unsigned int ly) {
  // check x in range, and decompose fraction part
  const unsigned int ix = CombinedDivAndMod(lx);
  if (lx>=overview_width) 
    return TERRAIN_INVALID;

  // check y in range, and decompose fraction part
  const unsigned int iy = CombinedDivAndMod(ly);
  if (ly>= overview_height) 
    return TERRAIN_INVALID;

  // perform piecewise linear interpolation
  const unsigned int dx= (lx==overview_width-1)? 0: 1;
  const unsigned int dy= (ly==overview_height-1)? 0: overview_width;
  const short *tm = Overview+ly*overview_width+lx;

  if (ix>iy) {
    // lower triangle 
    return *tm+((ix*(tm[dx]-*tm)-iy*(tm[dx]-tm[dx+dy]))>>8);
  } else {
    // upper triangle
    return *tm+((iy*(tm[dy]-*tm)-ix*(tm[dy]-tm[dx+dy]))>>8);
  }
};


void RasterTileCache::StitchTile(unsigned int src_tile) {
  short *h_src = tiles[src_tile].GetImageBuffer();
  if (!h_src) 
    return;

  const unsigned int width = tiles[src_tile].width;
  const unsigned int height = tiles[src_tile].height;
  const unsigned int xstart = tiles[src_tile].xstart;
  const unsigned int ystart = tiles[src_tile].ystart;
  unsigned int i;

  for (unsigned int dst_tile= MAX_RTC_TILES; dst_tile--; ) {
    if (tiles[dst_tile].GetImageBuffer() && (dst_tile != src_tile)) {

      short h;

      for (i=0; i<width; i++) {
        h = h_src[i];
        tiles[dst_tile].SetEdgeIfInRange(i+xstart, ystart, h);
      }
      for (i=0; i<height; i++) {
        h = h_src[i*(width+1)];
        tiles[dst_tile].SetEdgeIfInRange(xstart, i+ystart, h);
      }
    }
  }

}


void RasterTileCache::StitchTiles(void) {
  for (unsigned int i= MAX_RTC_TILES; i--; ) {
    if (tiles[i].GetImageBuffer()) {
      StitchTile(i);
    }
  }
}



void RasterTileCache::SetSize(int _width, int _height) {
  width = _width;
  height = _height;
  if (!Overview) {
    overview_width = width/RTC_SUBSAMPLING;
    overview_height = height/RTC_SUBSAMPLING;
    overview_width_fine = width*256;
    overview_height_fine = height*256;

    Overview = (short*)malloc(overview_width*overview_height
                              *sizeof(short));
  }
}


void RasterTileCache::SetLatLonBounds(double _lon_min, double _lon_max, 
                                      double _lat_min, double _lat_max) {
  lat_min = min(_lat_min,_lat_max);
  lat_max = max(_lat_min,_lat_max);
  lon_min = min(_lon_min,_lon_max);
  lon_max = max(_lon_min,_lon_max);
}


void RasterTileCache::Reset() {
  tile_last = 0;
  view_x = 0;
  view_y = 0;
  width = 0;
  height = 0;
  initialised = false;
  scan_overview = true;
  if (Overview) {
    free(Overview);
    Overview = 0;
  }
  int i;
  for (i=0; i< MAX_RTC_TILES; i++) {
    if (tiles[i].IsEnabled()) {
      tiles[i].Disable();
    }
  }
  for (i= MAX_ACTIVE_TILES; i--; ) {
    ActiveTiles[i] = -1;
  }
}


void RasterTileCache::SetInitialised(bool val) {
  if (!initialised && val) {
    if (lon_max-lon_min<0) {
      return;
    }
    if (lat_max-lat_min<0) {
      return;
    }
    initialised = true;
    scan_overview = false;

    return;
  }
  initialised = val;
  loaded_one = false;
}


bool RasterTileCache::GetInitialised(void) {
  return initialised;
}

short* RasterTileCache::GetOverview(void) {
  return Overview;
}

bool RasterTileCache::GetScanType(void) {
  return scan_overview;
}

short RasterTileCache::GetMaxElevation(void) {
  short max_elevation = 0;
  if (Overview) {
    for (unsigned int i= overview_width*overview_height; i--; ) {
      max_elevation = max(max_elevation, Overview[i]);
    }
  }
  return max_elevation;
}

extern RasterTileCache *raster_tile_current;

void RasterTileCache::LoadJPG2000(char* jp2_filename) {
  jas_stream_t *in;
  
  raster_tile_current = this;

  in = jas_stream_fopen(jp2_filename, "rb");
  if (!in) {
    SetInitialised(false);
  } else {
    jas_image_decode(in, -1, (char*)"xcsoar=1");
    jas_stream_close(in);
  }
  if (GetInitialised()) {
    StitchTiles();
  }
};
