/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TerrainRenderer.h"
#include "XCSoar.h"
#include "RasterTerrain.h"
#include "RasterWeather.h"
#include "MapWindowProjection.hpp"
#include "Topology.h"
#include "Screen/STScreenBuffer.h"
#include "Dialogs.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "InfoBoxLayout.h"
#include "Compatibility/string.h"
#include "Registry.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Graphics.hpp"
#include "LocalPath.hpp"
#include "Utils.h"
#include "LogFile.hpp"

#include <assert.h>

///////////////////////////////////////////////////

#define NUM_COLOR_RAMP_LEVELS 13


const COLORRAMP weather_colors[6][NUM_COLOR_RAMP_LEVELS] = {
  { // Blue to red       // vertical speed
    {   0,       0,     0,     255}, // -200
    { 100,       0,     195,   255}, // -100
    { 200,     52,      192,    11}, // 0
    { 250,     182,     233,     4}, // 40
    { 300,     255,     233,     0}, // 80
    { 360,     255,     209,     0}, // 120
    { 420,     255,     155,     0}, // 160
    { 480,     255,     109,     0}, // 200
    { 540,     255,     35,      0}, // 240
    { 600,     255,     00,      0}, // 300
    {1000,         0xFF, 0x00, 0x00},
    {8000,         0xFF, 0x00, 0x00},
    {9000,         0xFF, 0x00, 0x00}
  },
  {
    {0,            0xFF, 0xFF, 0xFF},
    {250,          0x80, 0x80, 0xFF},
    {500,          0x80, 0xFF, 0xFF},
    {750,          0xFF, 0xFF, 0x80},
    {1000,         0xFF, 0x80, 0x80},
    {1250,         0xFF, 0x80, 0x80},
    {2000,         0xFF, 0xA0, 0xA0},
    {3000,         0xFF, 0xA0, 0xA0},
    {4000,         0xFF, 0x00, 0x00},
    {5000,         0xFF, 0x00, 0x00},
    {6000,         0xFF, 0x00, 0x00},
    {7000,         0xFF, 0x00, 0x00},
    {8000,         0xFF, 0x00, 0x00}
  },
  {
    {0,            0xFF, 0xFF, 0xFF},
    {750,          0x80, 0x80, 0xFF},
    {1500,          0x80, 0xFF, 0xFF},
    {2250,          0xFF, 0xFF, 0x80},
    {3000,          0xFF, 0x80, 0x80},
    {3500,         0xFF, 0x80, 0x80},
    {6000,         0xFF, 0xA0, 0xA0},
    {8000,         0xFF, 0xA0, 0xA0},
    {9000,         0xFF, 0x00, 0x00},
    {9500,         0xFF, 0x00, 0x00},
    {9600,         0xFF, 0x00, 0x00},
    {9700,         0xFF, 0x00, 0x00},
    {20000,         0xFF, 0x00, 0x00}
  },
  { // Blue to Gray, 8 steps
    {   0,       0,     153,     204},
    {  12,     102,     229,     255},
    {  25,     153,     255,     255},
    {  37,     204,     255,     255},
    {  50,     229,     229,     229},
    {  62,     173,     173,     173},
    {  75,     122,     122,     122},
    { 100,      81,      81,      81},
    {5000,      71,      71,      71},
    {6000,         0xFF, 0x00, 0x00},
    {7000,         0xFF, 0x00, 0x00},
    {8000,         0xFF, 0x00, 0x00},
    {9000,         0xFF, 0x00, 0x00}
  },
  { // sfctemp, blue to orange to red
    {   0,       7,      90,     255},
    {  30,      50,     118,     255},
    {  70,      89,     144,     255},
    {  73,     140,     178,     255},
    {  76,     191,     212,     255},
    {  79,     229,     238,     255},
    {  82,     247,     249,     255},
    {  85,     255,     255,     204},
    {  88,     255,     255,     153},
    {  91,     255,     255,       0},
    {  95,     255,     204,       0},
    { 100,     255,     153,       0},
    { 120,     255,       0,       0}
  },
  { // Blue to white to red       // vertical speed (convergence)
    {   0,       7,      90,     255},
    { 100,      50,     118,     255},
    { 140,      89,     144,     255},
    { 160,     140,     178,     255},
    { 180,     191,     212,     255},
    { 190,     229,     238,     255},
    { 200,     247,     249,     255},
    { 210,     255,     255,     204},
    { 220,     255,     255,     153},
    { 240,     255,     255,       0},
    { 260,     255,     204,       0},
    { 300,     255,     153,       0},
    {1000,     255,     102,       0},
  },
};


const COLORRAMP terrain_colors[7][NUM_COLOR_RAMP_LEVELS] = {
  {
    {0,           0x70, 0xc0, 0xa7},
    {250,         0xca, 0xe7, 0xb9},
    {500,         0xf4, 0xea, 0xaf},
    {750,         0xdc, 0xb2, 0x82},
    {1000,        0xca, 0x8e, 0x72},
    {1250,        0xde, 0xc8, 0xbd},
    {1500,        0xe3, 0xe4, 0xe9},
    {1750,        0xdb, 0xd9, 0xef},
    {2000,        0xce, 0xcd, 0xf5},
    {2250,        0xc2, 0xc1, 0xfa},
    {2500,        0xb7, 0xb9, 0xff},
    {5000,        0xb7, 0xb9, 0xff},
    {6000,        0xb7, 0xb9, 0xff}
  },
  {
    {0,           0x70, 0xc0, 0xa7},
    {500,         0xca, 0xe7, 0xb9},
    {1000,        0xf4, 0xea, 0xaf},
    {1500,        0xdc, 0xb2, 0x82},
    {2000,        0xca, 0x8e, 0x72},
    {2500,        0xde, 0xc8, 0xbd},
    {3000,        0xe3, 0xe4, 0xe9},
    {3500,        0xdb, 0xd9, 0xef},
    {4000,        0xce, 0xcd, 0xf5},
    {4500,        0xc2, 0xc1, 0xfa},
    {5000,        0xb7, 0xb9, 0xff},
    {6000,        0xb7, 0xb9, 0xff},
    {7000,        0xb7, 0xb9, 0xff}
  },
  { // Imhof Type 7, geomteric 1.35 9
    {0,    153, 178, 169},
    {368,  180, 205, 181},
    {496,  225, 233, 192},
    {670,  255, 249, 196},
    {905,  255, 249, 196},
    {1222, 255, 219, 173},
    {1650, 254, 170, 136},
    {2227, 253, 107, 100},
    {3007, 255, 255, 255},
    {5000, 255, 255, 255},
    {6000, 255, 255, 255},
    {7000, 255, 255, 255},
    {8000, 255, 255, 255}
  },
  { // Imhof Type 4, geomteric 1.5 8
    {0,    175, 224, 203},
    {264,  211, 237, 211},
    {396,  254, 254, 234},
    {594,  252, 243, 210},
    {891,  237, 221, 195},
    {1336, 221, 199, 175},
    {2004, 215, 170, 148},
    {3007, 255, 255, 255},
    {4000, 255, 255, 255},
    {5000, 255, 255, 255},
    {6000, 255, 255, 255},
    {7000, 255, 255, 255},
    {8000, 255, 255, 255}
  },
  { // Imhof Type 12, geomteric  1.5 8
    {0,    165, 220, 201},
    {399,  219, 239, 212},
    {558,  254, 253, 230},
    {782,  254, 247, 211},
    {1094,  254, 237, 202},
    {1532, 254, 226, 207},
    {2145, 254, 209, 204},
    {3004, 255, 255, 255},
    {4000, 255, 255, 255},
    {5000, 255, 255, 255},
    {6000, 255, 255, 255},
    {7000, 255, 255, 255},
    {8000, 255, 255, 255}
  },
  { // Imhof Atlas der Schweiz
    {0,     47, 101, 147},
    {368,   58, 129, 152},
    {496,  117, 148, 153},
    {670,  155, 178, 140},
    {905,  192, 190, 139},
    {1222, 215, 199, 137},
    {1650, 229, 203, 171},
    {2227, 246, 206, 171},
    {3007, 252, 246, 244},
    {5001, 252, 246, 244},
    {7000, 252, 246, 244},
    {8000, 252, 246, 244},
    {9000, 252, 246, 244}
  },
  { // ICAO
    {0,           180, 205, 181},
    {199,         180, 205, 181},
    {200,         225, 233, 192},
    {499,         225, 233, 192},
    {500,         255, 249, 196},
    {999,         255, 249, 196},
    {1000,        255, 219, 173},
    {1499,        255, 219, 173},
    {1500,        254, 170, 136},
    {1999,        254, 170, 136},
    {2000,        253, 107, 100},
    {2499,        253, 107, 100},
    {2500,        255, 255, 255}
  }
};


short TerrainContrast = 150;
short TerrainBrightness = 36;
short TerrainRamp = 0;


#define MIX(x,y,i) (BYTE)((x*i+y*((1<<7)-i))>>7)

inline void TerrainShading(const short illum, BYTE &r, BYTE &g, BYTE &b)
{
  char x;
  if (illum<0) {           // shadow to blue
    x = min(63,-illum);
    r = MIX(0,r,x);
    g = MIX(0,g,x);
    b = MIX(64,b,x);
  } else if (illum>0) {    // highlight to yellow
    x = min(32,illum/2);
    r = MIX(255,r,x);
    g = MIX(255,g,x);
    b = MIX(16,b,x);
  }
}


// map scale is approximately 2 points on the grid
// therefore, want one to one mapping if mapscale is 0.5
// there are approx 30 pixels in mapscale
// 240/DTQUANT resolution = 6 pixels per terrain
// (mapscale/30)  km/pixels
//        0.250   km/terrain
// (0.25*30/mapscale) pixels/terrain
//  mapscale/(0.25*30)
//  mapscale/7.5 terrain units/pixel
//
// this is for TerrainInfo.StepSize = 0.0025;

extern int misc_tick_count;


class TerrainRenderer {
public:
  TerrainRenderer(RECT rc) {

    if (!RasterTerrain::IsDirectAccess()) {
      dtquant = 6;
    } else {
      // SAM: experiment with dtquant between 2 and 4
      dtquant = 2;

      // on my PDA (600MhZ, 320x240 screen):
      // dtquant=2, latency=170 ms
      // dtquant=3, latency=136 ms
      // dtquant=4, latency= 93 ms
    }
    blursize = max(0, (dtquant-1)/2);
    oversampling = max(1,(blursize+1)/2+1);
    if (blursize==0) {
      oversampling = 1; // no point in oversampling, just let stretchblt do the scaling
    }

    /*
      dtq  ovs  blur  res_x  res_y   sx  sy  terrain_loads  pixels
       1    1    0    320    240    320 240    76800        76800
       2    1    0    160    120    160 120    19200        19200
       3    2    1    213    160    107  80     8560        34080
       4    2    1    160    120     80  60     4800        19200
       5    3    2    192    144     64  48     3072        27648
    */

    // scale dtquant so resolution is not too high on large displays
    dtquant *= InfoBoxLayout::scale;

    int res_x = iround((rc.right-rc.left)*oversampling/dtquant);
    int res_y = iround((rc.bottom-rc.top)*oversampling/dtquant);

    sbuf = new CSTScreenBuffer();
    sbuf->Create(res_x, res_y, Color(0xff,0xff,0xff));
    ixs = sbuf->GetCorrectedWidth()/oversampling;
    iys = sbuf->GetHeight()/oversampling;

    hBuf = (unsigned short*)malloc(sizeof(unsigned short)*ixs*iys);

    colorBuf = (BGRColor*)malloc(256*128*sizeof(BGRColor));

  }

  ~TerrainRenderer() {
    if (hBuf) free(hBuf);
    if (colorBuf) free(colorBuf);
    if (sbuf) delete sbuf;
  }

public:
  POINT spot_max_pt;
  POINT spot_min_pt;
  short spot_max_val;
  short spot_min_val;

private:

  unsigned int ixs, iys; // screen dimensions in coarse pixels
  unsigned int dtquant;
  unsigned int epx; // step size used for slope calculations

  RECT rect_visible;

  CSTScreenBuffer *sbuf;

  double pixelsize_d;

  int oversampling;
  int blursize;

  unsigned short *hBuf;
  BGRColor *colorBuf;
  bool do_shading;
  bool do_water;
  RasterMap *DisplayMap;
  bool is_terrain;
  int interp_levels;
  COLORRAMP* color_ramp;
  unsigned int height_scale;

public:
  bool SetMap(double lon, double lat) {
    if (RasterTerrain::render_weather) {
      RASP.Reload(lat, lon);
    }
    interp_levels = 5;
    switch (RasterTerrain::render_weather) {
    case 1: // wstar
      is_terrain = false;
      do_water = false;
      height_scale = 2; // max range 256*(2**2) = 1024 cm/s = 10 m/s
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[0][0];
      break;
    case 2: // bl wind spd
      is_terrain = false;
      do_water = false;
      height_scale = 3;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[1][0];
      break;
    case 3: // hbl
      is_terrain = false;
      do_water = false;
      height_scale = 4;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[2][0];
      break;
    case 4: // dwcrit
      is_terrain = false;
      do_water = false;
      height_scale = 4;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[2][0];
      break;
    case 5: // blcloudpct
      is_terrain = false;
      do_water = true;
      height_scale = 0;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[3][0];
      break;
    case 6: // sfctemp
      is_terrain = false;
      do_water = false;
      height_scale = 0;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[4][0];
      break;
    case 7: // hwcrit
      is_terrain = false;
      do_water = false;
      height_scale = 4;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[2][0];
      break;
    case 8: // wblmaxmin
      is_terrain = false;
      do_water = false;
      height_scale = 1; // max range 256*(1**2) = 512 cm/s = 5.0 m/s
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[5][0];
      break;
    case 9: // blcwbase
      is_terrain = false;
      do_water = false;
      height_scale = 4;
      DisplayMap = RASP.weather_map[RasterTerrain::render_weather-1];
      color_ramp = (COLORRAMP*)&weather_colors[2][0];
      break;
    default:
    case 0:
      interp_levels = 2;
      is_terrain = true;
      do_water = true;
      height_scale = 4;
      DisplayMap = RasterTerrain::TerrainMap;
      color_ramp = (COLORRAMP*)&terrain_colors[TerrainRamp][0];
      break;
    }

    /////////////////////////

    if (is_terrain) {
      do_shading = true;
    } else {
      do_shading = false;
    }

    if (DisplayMap)
      return true;
    else
      return false;

  }

  void Height(bool isBigZoom) {
    double X, Y;
    int x, y;
    int X0 = (unsigned int)(dtquant/2);
    int Y0 = (unsigned int)(dtquant/2);
    int X1 = (unsigned int)(X0+dtquant*ixs);
    int Y1 = (unsigned int)(Y0+dtquant*iys);

    unsigned int rfact=1;

    if (isBigZoom && !RasterTerrain::IsDirectAccess()) {
      // first time displaying this data, so do it at half resolution
      // to avoid too many cache misses
      rfact = 2;
    }

    double pixelDX, pixelDY;

    x = (X0+X1)/2;
    y = (Y0+Y1)/2;
    MapWindowProjection::Screen2LatLon(x, y, X, Y);
    double xmiddle = X;
    double ymiddle = Y;
    int dd = (int)lround(dtquant*rfact);

    x = (X0+X1)/2+dd;
    y = (Y0+Y1)/2;
    MapWindowProjection::Screen2LatLon(x, y, X, Y);
    float Xrounding = (float)fabs(X-xmiddle);
    DistanceBearing(ymiddle, xmiddle, Y, X, &pixelDX, NULL);

    x = (X0+X1)/2;
    y = (Y0+Y1)/2+dd;
    MapWindowProjection::Screen2LatLon(x, y, X, Y);
    float Yrounding = (float)fabs(Y-ymiddle);
    DistanceBearing(ymiddle, xmiddle, Y, X, &pixelDY, NULL);

    pixelsize_d = sqrt((pixelDX*pixelDX+pixelDY*pixelDY)/2.0);

    // OK, ready to start loading height

    DisplayMap->Lock();

    misc_tick_count = GetTickCount();

    // TODO code: not needed   RasterTerrain::SetCacheTime();

    // set resolution

    if (DisplayMap->IsDirectAccess()) {
      DisplayMap->SetFieldRounding(0,0);
    } else {
      DisplayMap->SetFieldRounding(Xrounding,Yrounding);
    }

    epx = DisplayMap->GetEffectivePixelSize(&pixelsize_d,
                                            ymiddle, xmiddle);

    if (epx> min(ixs,iys)/4) {
      do_shading = false;
    }

    POINT orig = MapWindowProjection::GetOrigScreen();
    RECT MapRectBig = MapWindowProjection::GetMapRectBig();
    RECT MapRect    = MapWindowProjection::GetMapRect();

    rect_visible.left = max((long)MapRectBig.left,
                            (long)(MapRect.left-(long)epx*dtquant))-orig.x;
    rect_visible.right = min((long)MapRectBig.right,
                             (long)(MapRect.right+(long)epx*dtquant))-orig.x;
    rect_visible.top = max((long)MapRectBig.top,
                           (long)(MapRect.top-(long)epx*dtquant))-orig.y;
    rect_visible.bottom = min((long)MapRectBig.bottom,
                              (long)(MapRect.bottom+(long)epx*dtquant))-orig.y;

    FillHeightBuffer(X0-orig.x, Y0-orig.y, X1-orig.x, Y1-orig.y);

    DisplayMap->Unlock();

    if (RasterTerrain::render_weather) {
      ScanSpotHeights(X0-orig.x, Y0-orig.y, X1-orig.x, Y1-orig.y);
    }
  }

  void ScanSpotHeights(const int X0, const int Y0, const int X1, const int Y1) {
    unsigned short* myhbuf = hBuf;
#ifndef NDEBUG
    unsigned short* hBufTop = hBuf+ixs*iys;
#endif

    spot_max_pt.x = -1;
    spot_max_pt.y = -1;
    spot_min_pt.x = -1;
    spot_min_pt.y = -1;
    spot_max_val = -1;
    spot_min_val = 32767;

    RECT rect_spot;
    rect_spot.left =   rect_visible.left+IBLSCALE(30);
    rect_spot.right =  rect_visible.right-IBLSCALE(30);
    rect_spot.top =    rect_visible.top+IBLSCALE(30);
    rect_spot.bottom = rect_visible.bottom-IBLSCALE(30);

    for (int y = Y0; y<Y1; y+= dtquant) {
      for (int x = X0; x<X1; x+= dtquant, myhbuf++) {
        if ((x>= rect_spot.left) &&
            (x<= rect_spot.right) &&
            (y>= rect_spot.top) &&
            (y<= rect_spot.bottom)) {
          assert(myhbuf<hBufTop);

          short val = *myhbuf;
          if (val>spot_max_val) {
            spot_max_val = val;
            spot_max_pt.x = x;
            spot_max_pt.y = y;
          }
          if (val<spot_min_val) {
            spot_min_val = val;
            spot_min_pt.x = x;
            spot_min_pt.y = y;
          }
        }
      }
    }
  }

  void FillHeightBuffer(const int X0, const int Y0, const int X1, const int Y1) {
    // fill the buffer
    unsigned short* myhbuf = hBuf;
#ifndef NDEBUG
    unsigned short* hBufTop = hBuf+ixs*iys;
#endif

#ifndef SLOW_STUFF

    // This code is quickest but not so readable

    const double PanLatitude =  MapWindowProjection::GetPanLatitude();
    const double PanLongitude = MapWindowProjection::GetPanLongitude();
    const double InvDrawScale = MapWindowProjection::GetInvDrawScale()/1024.0;
    const double DisplayAngle = MapWindowProjection::GetDisplayAngle();
    const int cost = ifastcosine(DisplayAngle);
    const int sint = ifastsine(DisplayAngle);

    for (int y = Y0; y<Y1; y+= dtquant) {
      int ycost = y*cost;
      int ysint = y*sint;
      for (int x = X0; x<X1; x+= dtquant, myhbuf++) {
        if ((x>= rect_visible.left) &&
            (x<= rect_visible.right) &&
            (y>= rect_visible.top) &&
            (y<= rect_visible.bottom)) {
          assert(myhbuf<hBufTop);

          double Y = PanLatitude - (ycost+x*sint)*InvDrawScale;
          double X = PanLongitude + (x*cost-ysint)*invfastcosine(Y)*InvDrawScale;
          *myhbuf = max(0, DisplayMap->GetField(Y,X));
        } else {
          *myhbuf = 0;
        }
      }
    }

#else

    // This code is marginally slower but readable
    double X, Y;
    for (int y = Y0; y<Y1; y+= dtquant) {
      for (int x = X0; x<X1; x+= dtquant) {
        MapWindowProjection::Screen2LatLon(x,y,X,Y);
        *myhbuf++ = max(0, DisplayMap->GetField(Y, X));
      }
    }

#endif

  }

  // JMW: if zoomed right in (e.g. one unit is larger than terrain
  // grid), then increase the step size to be equal to the terrain
  // grid for purposes of calculating slope, to avoid shading problems
  // (gridding of display) This is why epx is used instead of 1
  // previously.  for large zoom levels, epx=1

  void Slope(const int sx, const int sy, const int sz) {

    const int iepx = (int)epx;
    const unsigned int cixs = ixs;
    const unsigned int ciys = iys;
    const unsigned int ixsepx = cixs*epx;
    const unsigned int ixsright = cixs-1-iepx;
    const unsigned int iysbottom = ciys-iepx;
    const int hscale = max(1,(int)(pixelsize_d));
    const int tc = TerrainContrast;
    unsigned short *thBuf = hBuf;

    const BGRColor* oColorBuf = colorBuf+64*256;
    BGRColor* imageBuf = sbuf->GetBuffer();
    if (!imageBuf) return;

    short h;

#ifndef NDEBUG
    unsigned short* hBufTop = hBuf+cixs*ciys;
#endif

    for (unsigned int y = 0; y< iys; y++) {
      const int itss_y = ciys-1-y;
      const int itss_y_ixs = itss_y*cixs;
      const int yixs = y*cixs;
      bool ybottom=false;
      bool ytop=false;
      int p31, p32, p31s;

      if (y<iysbottom) {
        p31= iepx;
        ybottom = true;
      } else {
        p31= itss_y;
      }

      if (y >= (unsigned int) iepx) {
        p31+= iepx;
      } else {
        p31+= y;
        ytop = true;
      }
      p31s = p31*hscale;

      for (unsigned int x = 0 ; x<cixs; x++, thBuf++, imageBuf++) {

        assert(thBuf< hBufTop);

        if ((h = *thBuf)>0) {
	  int p20, p22;

          h = min(255, h>>height_scale);
          // no need to calculate slope if undefined height or sea level

          if (do_shading) {
            if (x<ixsright) {
              p20= iepx;
              p22= *(thBuf+iepx);
              assert(thBuf+iepx< hBufTop);
            } else {
	      int itss_x = cixs-x-2;
              p20= itss_x;
              p22= *(thBuf+itss_x);
              assert(thBuf+itss_x< hBufTop);
              assert(thBuf+itss_x>= hBuf);
            }

            if (x >= (unsigned int)iepx) {
              p20+= iepx;
              p22-= *(thBuf-iepx);
              assert(thBuf-iepx>= hBuf);
            } else {
              p20+= x;
              p22-= *(thBuf-x);
              assert(thBuf-x>= hBuf);
            }

            if (ybottom) {
              p32 = *(thBuf+ixsepx);
              assert(thBuf+ixsepx<hBufTop);
            } else {
              p32 = *(thBuf+itss_y_ixs);
              assert(thBuf+itss_y_ixs<hBufTop);
            }
            if (ytop) {
              p32 -= *(thBuf-yixs);
              assert(thBuf-yixs>=hBuf);
            } else {
              p32 -= *(thBuf-ixsepx);
              assert(thBuf-ixsepx>=hBuf);
            }

            if ((p22==0) && (p32==0)) {

              // slope is zero, so just look up the color
              *imageBuf = oColorBuf[h];

            } else {

              // p20 and p31 are never 0... so only p22 or p32 can be zero
              // if both are zero, the vector is 0,0,1 so there is no need
              // to normalise the vector
              int dd0 = p22*p31;
              int dd1 = p20*p32;
              int dd2 = p20*p31s;

              while (dd2>512) {
                // prevent overflow of magnitude calculation
                dd0 /= 2;
                dd1 /= 2;
                dd2 /= 2;
              }
              int mag = (dd0*dd0+dd1*dd1+dd2*dd2);
              if (mag>0) {
                mag = (dd2*sz+dd0*sx+dd1*sy)/isqrt4(mag);
                mag = max(-64,min(63,(mag-sz)*tc/128));
                *imageBuf = oColorBuf[h+mag*256];
              } else {
                *imageBuf = oColorBuf[h];
              }
            }
          } else {
            // slope is zero, so just look up the color
            *imageBuf = oColorBuf[h];
          }
        } else {
          // we're in the water, so look up the color for water
          *imageBuf = oColorBuf[255];
        }
      }
    }
  };

  void ColorTable() {
    static COLORRAMP* lastColorRamp = NULL;
    if (color_ramp == lastColorRamp) {
      // no need to update the color table
      return;
    }
    lastColorRamp = color_ramp;

    for (int i=0; i<256; i++) {
      for (int mag= -64; mag<64; mag++) {
        BYTE r, g, b;
        if (i == 255) {
          if (do_water) {
            // water colours
            r = 85;
            g = 160;
            b = 255;
          } else {
            r = 255;
            g = 255;
            b = 255;
            /*
            ColorRampLookup(0, r, g, b,
                            color_ramp, NUM_COLOR_RAMP_LEVELS, interp_levels);
            */
          }
        } else {
          ColorRampLookup(i<<height_scale, r, g, b,
                          color_ramp, NUM_COLOR_RAMP_LEVELS, interp_levels);
          TerrainShading(mag, r, g, b);
        }
        colorBuf[i+(mag+64)*256] = BGRColor(r,g,b);
      }
    }
  }

  void Draw(Canvas &canvas, RECT rc) {

    sbuf->Zoom(oversampling);

    if (blursize>0) {

      sbuf->HorizontalBlur(blursize);
      sbuf->VerticalBlur(blursize);

    }
    sbuf->DrawStretch(canvas, rc);

  }

};



//////////////////////////////////////////////////

TerrainRenderer *trenderer = NULL;

int Performance = 0;

void CloseTerrainRenderer() {
  if (trenderer) {
    delete trenderer;
  }
}



void DrawTerrain(Canvas &canvas, const RECT rc,
                  const double sunazimuth, const double sunelevation,
		  double lon, double lat,
		  const bool isBigZoom)
{
  (void)sunelevation; // TODO feature: sun-based rendering option
  (void)rc;

  if (!RasterTerrain::isTerrainLoaded()) {
    return;
  }

  if (!trenderer) {
    trenderer = new TerrainRenderer(MapWindowProjection::GetMapRectBig());
  }

  if (!trenderer->SetMap(lon, lat)) {
    return;
  }

  // step 1: calculate sunlight vector
  int sx, sy, sz;
  double fudgeelevation = (10.0+80.0*TerrainBrightness/255.0);

  sx = (int)(255*(fastcosine(fudgeelevation)*fastsine(sunazimuth)));
  sy = (int)(255*(fastcosine(fudgeelevation)*fastcosine(sunazimuth)));
  sz = (int)(255*fastsine(fudgeelevation));

  trenderer->ColorTable();

  // step 2: fill height buffer

  trenderer->Height(isBigZoom);

  // step 3: calculate derivatives of height buffer
  // step 4: calculate illumination and colors
  trenderer->Slope(sx, sy, sz);

  // step 5: draw
  trenderer->Draw(canvas, MapWindowProjection::GetMapRectBig());

  misc_tick_count = GetTickCount()-misc_tick_count;
}


static void DrawSpotHeight_Internal(Canvas &canvas, TCHAR *Buffer, POINT pt) {
  int size = _tcslen(Buffer);
  if (size==0) {
    return;
  }
  POINT orig = MapWindowProjection::GetOrigScreen();
  RECT brect;
  SIZE tsize = canvas.text_size(Buffer);

  pt.x+= 2+orig.x;
  pt.y+= 2+orig.y;
  brect.left = pt.x;
  brect.right = brect.left+tsize.cx;
  brect.top = pt.y;
  brect.bottom = brect.top+tsize.cy;

  if (!checkLabelBlock(brect))
    return;

  canvas.text(pt.x, pt.y, Buffer);
}

void DrawSpotHeights(Canvas &canvas) {
  // JMW testing, display of spot max/min
  if (!RasterTerrain::render_weather)
    return;
  if (!trenderer)
    return;

  extern Font TitleWindowFont;
  canvas.select(TitleWindowFont);

  TCHAR Buffer[20];

  RASP.ValueToText(Buffer, trenderer->spot_max_val);
  DrawSpotHeight_Internal(canvas, Buffer, trenderer->spot_max_pt);

  RASP.ValueToText(Buffer, trenderer->spot_min_val);
  DrawSpotHeight_Internal(canvas, Buffer, trenderer->spot_min_pt);
}

