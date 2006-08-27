/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
// omaplibdemo.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "Terrain.h"
#include "RasterTerrain.h"
#include "MapWindow.h"
#include "Topology.h"
#include "resource.h"
#include "Calculations.h"
#include "STScreenBuffer.h"
#include "externs.h"
#include "Utils.h"
#include "VarioSound.h"
#include "InfoBoxLayout.h"
#include "Sizes.h"

//////////////////////////////////////////////////


Topology* TopoStore[MAXTOPOLOGY];

TopologyWriter *topo_marks;

#define MINRANGE 0.2

bool RectangleIsInside(rectObj r_exterior, rectObj r_interior) {
  if ((r_interior.minx >= r_exterior.minx)&&
      (r_interior.maxx <= r_exterior.maxx)&&
      (r_interior.miny >= r_exterior.miny)&&
      (r_interior.maxy <= r_exterior.maxy))    
    return true;
  else 
    return false;
}

void SetTopologyBounds(const RECT rcin, const bool force) {
  static rectObj bounds_active;
  static double range_active = 1.0;
  rectObj bounds_screen;

  bounds_screen = MapWindow::CalculateScreenBounds(1.0);

  bool recompute = false;

  // only recalculate which shapes when bounds change significantly
  // need to have some trigger for this..

  // trigger if the border goes outside the stored area
  if (!RectangleIsInside(bounds_active, bounds_screen)) {
    recompute = true;
  }
  
  // also trigger if the scale has changed heaps
  double range_real = max((bounds_screen.maxx-bounds_screen.minx), 
			  (bounds_screen.maxy-bounds_screen.miny));
  double range = max(MINRANGE,range_real);

  double scale = range/range_active;
  if (max(scale, 1.0/scale)>4) {
    recompute = true;
  }

  if (recompute || force) {

    // make bounds bigger than screen
    if (range_real<MINRANGE) {
      scale = BORDERFACTOR*MINRANGE/range_real;
    } else {
      scale = BORDERFACTOR;
    }
    bounds_active = MapWindow::CalculateScreenBounds(scale);

    range_active = max((bounds_active.maxx-bounds_active.minx), 
		       (bounds_active.maxy-bounds_active.miny));
    
    for (int z=0; z<MAXTOPOLOGY; z++) {
      if (TopoStore[z]) {
	TopoStore[z]->triggerUpdateCache=true;          
      }
    }
    topo_marks->triggerUpdateCache = true;

    // now update visibility of objects in the map window
    MapWindow::ScanVisibility(&bounds_active);

  }

  // check if things have come into or out of scale limit
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      TopoStore[z]->TriggerIfScaleNowVisible();          
    }
  }

  // ok, now update the caches

  topo_marks->updateCache(bounds_active);    
  
  if (EnableTopology) {
    // check if any needs to have cache updates because wasnt 
    // visible previously when bounds moved
    bool sneaked= false;
    bool rta;

    // we will make sure we update at least one cache per call
    // to make sure eventually everything gets refreshed

    int total_shapes_visible = 0;
    for (int z=0; z<MAXTOPOLOGY; z++) {
      if (TopoStore[z]) {
	rta = MapWindow::RenderTimeAvailable() || force || !sneaked;
	if (TopoStore[z]->triggerUpdateCache) {
	  sneaked = true;
	}
	TopoStore[z]->updateCache(bounds_active, !rta);
	total_shapes_visible += TopoStore[z]->shapes_visible_count;
      }
    }
#ifdef DEBUG
    char tmptext[100];
    sprintf(tmptext,"%d # shapes\n", total_shapes_visible);
    DebugStore(tmptext);
#endif

  }
}


#include "MapWindow.h"


void ReadTopology() {

  LockTerrainDataGraphics();
	  
  // TODO - This convert to non-unicode will not support all languages
  //		(some may use more complicated PATH names, containing Unicode)
  //  char buffer[MAX_PATH];
  //  ConvertTToC(buffer, LocalPath(TEXT("xcsoar-marks")));
  // DISABLED LocalPath
  // JMW localpath does NOT work for the shapefile renderer!

  if (topo_marks) {
    delete topo_marks;
  }

  topo_marks = 
	  new TopologyWriter("xcsoar-marks", RGB(0xD0,0xD0,0xD0));

  topo_marks->scaleThreshold = 30.0;

  topo_marks->loadBitmap(IDB_MARK);
  UnlockTerrainDataGraphics();
}


void CloseTopology() {
  StartupStore(TEXT("CloseTopology\r\n"));

  LockTerrainDataGraphics();
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      delete TopoStore[z];
    }
  }
  if (topo_marks) {
    delete topo_marks;
    topo_marks = NULL;
  }
  UnlockTerrainDataGraphics();
}


void MarkLocation(const double lon, const double lat)
{
  LockTerrainDataGraphics();

#ifndef DISABLEAUDIO
  if (EnableSoundModes) {
    PlayResource(TEXT("IDR_WAV_CLEAR"));
  }
#endif 

  topo_marks->addPoint(lon, lat);
  topo_marks->triggerUpdateCache = true;
  UnlockTerrainDataGraphics();

  //////////

  char message[160];

  sprintf(message,"Lon:%f Lat:%f\r\n", lon, lat);

  FILE *stream;
  stream = _wfopen(LocalPath(TEXT("xcsoar-marks.txt")),TEXT("a+"));
  if (stream != NULL){
    fwrite(message,strlen(message),1,stream);
    fclose(stream);
  }

#if (EXPERIMENTAL > 0)
  bsms.SendSMS(message);
#endif

}

void DrawMarks (const HDC hdc, const RECT rc)
{

  LockTerrainDataGraphics();
  topo_marks->Paint(hdc, rc);
  UnlockTerrainDataGraphics();

}


void DrawTopology(const HDC hdc, const RECT rc)
{

  LockTerrainDataGraphics();

  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      TopoStore[z]->Paint(hdc,rc);
    }
  }

  UnlockTerrainDataGraphics();

}

////////


#define NUMTERRAINRAMP 12

COLORRAMP terrain_colors[] = {
  {-1,          0xff, 0xff, 0xff},
  {0,           0x70, 0xc0, 0xa7},
  {250,         0xca, 0xe7, 0xb9},
  {500,        0xf4, 0xea, 0xaf},
  {750,        0xdc, 0xb2, 0x82},
  {1000,        0xca, 0x8e, 0x72},
  {1250,        0xde, 0xc8, 0xbd},
  {1500,        0xe3, 0xe4, 0xe9},
  {1750,        0xdb, 0xd9, 0xef},
  {2000,        0xce, 0xcd, 0xf5},
  {2250,        0xc2, 0xc1, 0xfa},
  {2500,        0xb7, 0xb9, 0xff}
};


void ColorRampLookup(const short h, BYTE &r, BYTE &g, BYTE &b,
		     COLORRAMP* ramp_colors, const int numramp) {

  int i;
  int tr, tg, tb;
  short f, of;

  // check if h lower than lowest
  if (h<=ramp_colors[0].h) {
    r = ramp_colors[0].r;
    g = ramp_colors[0].g;
    b = ramp_colors[0].b;
    return;
  }
  // gone past end, so use last color
  if (h>=ramp_colors[numramp-1].h) {
    r = ramp_colors[numramp-1].r;
    g = ramp_colors[numramp-1].g;
    b = ramp_colors[numramp-1].b;
    return;
  }

  for (i=numramp-2; i>=0; i--) {
    if (h>=ramp_colors[i].h) {
      f = (h-ramp_colors[i].h)*255
        /(ramp_colors[i+1].h-ramp_colors[i].h);
      of = 255-f;
      tr = f*ramp_colors[i+1].r+of*ramp_colors[i].r;
      tg = f*ramp_colors[i+1].g+of*ramp_colors[i].g;
      tb = f*ramp_colors[i+1].b+of*ramp_colors[i].b;
      r = tr >> 8; // was /256
      g = tg >> 8;
      b = tb >> 8;
      return;
    }
  }

}


void TerrainColorMap(const short h, BYTE &r, BYTE &g, BYTE &b) {
  ColorRampLookup(h, r, g, b, terrain_colors, NUMTERRAINRAMP);
}

static short ContrastPos;
static short ContrastNeg;
short TerrainContrast = 150; 
short TerrainBrightness = 36;
short TerrainWhiteness = 0;

static void UpdateContrast(void) {
  TerrainWhiteness = TerrainBrightness;
  int tb = (255-TerrainWhiteness);
  int tc = (tb*(int)TerrainContrast)/256;
  ContrastPos = (short)(tc);
  ContrastNeg = (short)(tb-tc);
}


void TerrainIllumination(const short illum, BYTE &r, BYTE &g, BYTE &b)
{
  /*
  short il = (illum*ContrastPos)/256+ContrastNeg;
  r = (BYTE)((r*il>>8)+TerrainWhiteness);
  g = (BYTE)((g*il>>8)+TerrainWhiteness);
  b = (BYTE)((b*il>>8)+TerrainWhiteness);
  */
  r = (BYTE)((r*illum>>8));
  g = (BYTE)((g*illum>>8));
  b = (BYTE)((b*illum>>8));
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


class TerrainRenderer {
public:
  TerrainRenderer(RECT rc) {

    dtquant = DTQUANT;
    if (RasterTerrain::DirectAccess) {
      dtquant -= 2;
    }

    // scale dtquant so resolution is not too high (slow) on large PDA displays
#if (WINDOWSPC<1) 
    dtquant *= InfoBoxLayout::scale;
#endif

    ixs = iround((rc.right-rc.left)/dtquant);
    iys = iround((rc.bottom-rc.top)/dtquant);

    sbuf = new CSTScreenBuffer();
    sbuf->Create(ixs*OVS,iys*OVS,RGB(0xff,0xff,0xff));
    ixs = sbuf->GetCorrectedWidth()/OVS;

    hBuf = (short*)malloc(sizeof(short)*ixs*iys);
    nxBuf = (short*)malloc(sizeof(short)*ixs*iys);
    nyBuf = (short*)malloc(sizeof(short)*ixs*iys);
    nzBuf = (short*)malloc(sizeof(short)*ixs*iys);
    ilBuf = (short*)malloc(sizeof(short)*ixs*iys);

    pixelsize = (float)(MapWindow::MapScale/MapWindow::GetMapResolutionFactor()
			*dtquant);

  }
  ~TerrainRenderer() {
	if (hBuf) free(hBuf);
	if (nxBuf) free(nxBuf);
	if (nyBuf) free(nyBuf);
	if (nzBuf) free(nzBuf);
	if (ilBuf) free(ilBuf);
	if (sbuf) delete sbuf;
  }

  int ixs, iys; // screen dimensions in coarse pixels
  int dtquant;
  int epx; // step size used for slope calculations

  CSTScreenBuffer *sbuf;

  float pixelsize;

  short *hBuf;
  short *nxBuf;
  short *nyBuf;
  short *nzBuf;
  short *ilBuf;

  float Xrounding;
  float Yrounding;

#define TERRAIN_ANTIALIASING 1

  void Height(RECT rc) {
    double X, Y;
    int x, y; 
    short X0, Y0;
    short X1, Y1;
    X0 = dtquant/2; 
    Y0 = dtquant/2;
    X1 = X0+dtquant*ixs;
    Y1 = Y0+dtquant*iys;
    short* myhbuf = hBuf;

    if(!terrain_dem_graphics.isTerrainLoaded())
      return;

    DWORD tm, tmstart;
    tmstart = GetTickCount();

    LockTerrainDataGraphics();

    terrain_dem_graphics.SetCacheTime();

    // grid spacing = 250*rounding; in meters (dependent on resolution)

    int rfact=1;

    if (MapWindow::BigZoom) {
      MapWindow::BigZoom = false;
      rfact = 2;
    } else {
      rfact = 1;
    }
    if (RasterTerrain::DirectAccess) {
      rfact = 1;
    }

    // magnify gradient to make it
    // more obvious

    int pval = 0; // y*ixs+x;
    if (rc.top>0) {
      pval = ixs*(rc.top/dtquant-1);
      myhbuf+= pval;
      Y0 += rc.top-dtquant;
      Y1 -= rc.top-dtquant; 
       // this is a cheat since we don't really know how
       // far the bottom goes down
    }

    // JMW attempting to remove wobbling terrain
    x = ((X0+X1)/2);
    y = ((Y0+Y1)/2);
    MapWindow::Screen2LatLon(x, y, X, Y);
    double xmiddle = X;
    double ymiddle = Y;

    x = ((X0+X1)/2+dtquant*TERRAIN_ANTIALIASING*rfact);
    y = ((Y0+Y1)/2);
    MapWindow::Screen2LatLon(x, y, X, Y);
    Xrounding = (float)fabs(X-xmiddle);

    x = ((X0+X1)/2);
    y = ((Y0+Y1)/2+dtquant*TERRAIN_ANTIALIASING*rfact);
    MapWindow::Screen2LatLon(x, y, X, Y);
    Yrounding = (float)fabs(Y-ymiddle);

    // set resolution

    terrain_dem_graphics.SetTerrainRounding(Xrounding,Yrounding);

    pixelsize = (float)(MapWindow::MapScale/MapWindow::GetMapResolutionFactor()
			*dtquant);

    epx = max(terrain_dem_graphics.GetEffectivePixelSize(pixelsize)/2,1);
    epx = min(min(ixs,iys)/2, epx); 
    if (!RasterTerrain::DirectAccess) {
      epx = min(100,epx);
    }

    tm = GetTickCount();
    tm = tm-tmstart;
    tmstart = GetTickCount();

    // fill the buffer

    for (y = Y0; y<Y1; y+= dtquant) {
      for (x = X0; x<X1; x+= dtquant) {
        MapWindow::Screen2LatLon(x, y, X, Y);
        *myhbuf = terrain_dem_graphics.GetTerrainHeight(Y, X);
        ++myhbuf;
      }
    }

    tm = GetTickCount();
    tm = tm-tmstart;
    tmstart = GetTickCount();

    UnlockTerrainDataGraphics();

  }

  void Slope() {
    int mag;

    if(!terrain_dem_graphics.isTerrainLoaded())
      return;

    int ixsepx = ixs*epx;

    int tss = (int)(epx*pixelsize*1024);

    int ixsright = ixs-epx;
    int iysbottom = iys-epx;

    short *tnxBuf = nxBuf;
    short *tnyBuf = nyBuf;
    short *tnzBuf = nzBuf;
    short *thBuf = hBuf;

    for (int y = 0; y<iys; y++) {
      for (int x = 0; x<ixs; x++) {

	// JMW: if zoomed right in (e.g. one unit
	// is larger than terrain grid), then increase the
	// step size to be equal to the terrain grid
	// for purposes of calculating slope, to avoid
	// shading problems (gridding of display)
	// This is why epx is used instead of 1 previously.
	// for large zoom levels, epx=1

	int p20, p22, p31, p32;
	if (x>=ixsright) {
	  int itss = (ixs-x)-1;
	  p20= tss*itss/epx;
	  p22= thBuf[itss];
	} else {
	  p20= tss;
	  p22= thBuf[epx];
	}
	if (x<epx) {
	  int itss = x;
	  p20+= tss*itss/epx;
	  p22-= thBuf[-itss];
	} else {
	  p20+= tss;
	  p22-= thBuf[-epx];
	}

	if (y>=iysbottom) {
	  int itss = (iys-y)-1;
	  p31= tss*itss/epx;
	  p32= thBuf[itss*ixs];
	} else {
	  p31= tss;
	  p32= thBuf[ixsepx];
	}
	if (y<epx) {
	  int itss = y;
	  p31+= tss*itss/epx;
       	  p32-= thBuf[-itss*ixs];
	} else {
	  p31+= tss;
	  p32-= thBuf[-ixsepx]; 
	}
	
	int dp210, dp212;
	int dp311, dp312;
	
	dp210= p20;
	dp212= p22*2; // magnify slope
	
	dp311= p31;
	dp312= p32*2; // magnify slope 
	
	int dd0, dd1, dd2;
	dd0 = (-dp212*dp311)/tss;
	dd1 = (-dp210*dp312)/tss;
	dd2 = (dp210*dp311)/tss;
	mag = isqrt4(dd0*dd0+dd1*dd1+dd2*dd2);
	if (mag>0) {
	  dd0= (dd0<<8)/mag;
	  dd1= (dd1<<8)/mag;
	  dd2= (dd2<<8)/mag;
	} else {
	  dd0= 0;
	  dd1= 0;
	  dd2= 255;
	}
	*tnxBuf = (short)dd0;
	*tnyBuf = (short)dd1;
	*tnzBuf = (short)dd2;

	thBuf++;
	tnxBuf++;
	tnyBuf++;
	tnzBuf++;

      }
    }
  }

  void Illumination(short sx, short sy, short sz) {
    short *tnxBuf = nxBuf;
    short *tnyBuf = nyBuf;
    short *tnzBuf = nzBuf;
    short *tilBuf = ilBuf;
    int mag;

    if(!terrain_dem_graphics.isTerrainLoaded())
      return;

    UpdateContrast();

    int gsize = ixs*iys;
    short min_illumination = 1000;
    short max_illumination = 0;
    short illumination;
    int av_illumination = 0;
    int i;
    for (i=0; i<gsize; ++i) {
      
      mag = (*tnxBuf*sx+*tnyBuf*sy+*tnzBuf*sz)/256; // 8
      illumination = max(0,min(255,(short)mag));
      *tnxBuf = illumination;

      max_illumination = max(illumination, max_illumination);
      min_illumination = min(illumination, min_illumination);
      av_illumination += illumination;

      ++tnxBuf;
      ++tnyBuf;
      ++tnzBuf;

    }
    av_illumination /= gsize;

    // rescale illumination to improve contrast

    short mslope=0;
    short moffset=0;
    short bright = 128+TerrainBrightness/2;

    static short t_mslope = 256;
    static short t_moffset = 170;

    if (max_illumination-av_illumination > 0) {
      // we want max illumination to be 255
      // we want average illumination to equal bright

      // bright = av_illumination*slope/256+offset
      // 255 = max_illumination*slope/256+offset
      // 
      // offset = 255-max_illum*slope/256 = bright-av_illum*slope/256
      // 255-bright = (max_ill-av_illum)*slope/256
      // slope = 256*(255-bright)/(max_illum-av_illum)

      mslope = (255*256-bright*256)/(max_illumination-av_illumination);
    } else {
      mslope = 256;
    }
    mslope = min(256*2,mslope);
    moffset = 256-mslope*max_illumination/256;

    t_mslope = (4*t_mslope+4*mslope)/8;
    t_moffset = (4*t_moffset+4*moffset)/8;

    // make quick tabular lookup for illumination
    short ttab[257];
    for (i=0; i<256; i++) {
      ttab[i] = max(0,min(255,i*t_mslope/256+t_moffset));
    }
    ttab[256]=ttab[255];

    // smooth shading buffer

#ifndef NEWSMOOTH
    tilBuf = ilBuf;
    tnxBuf = nxBuf;
    for (i=0; i<gsize; ++i) {
      *tilBuf = ttab[*tnxBuf];
      tilBuf++;
      tnxBuf++;
    }
#else
    int index = 0;
    tilBuf = ilBuf;
    for (int y = 0; y<iys; y++) {
      for (int x = 0; x<ixs; x++) {
	
	  short ff;
	  short vv;

	  vv = 3;
	  ff = 3*nxBuf[index];
	  if (x>0) {
	  ++vv;
	  ff += nxBuf[index-1];
	  }
	if (x<ixs-1) {
	  ++vv;
	  ff += nxBuf[index+1];
	}
	if (y>0) {
	  ++vv;
	  ff += nxBuf[index-ixs];
	}
	if (y<iys-1) {
	  ++vv;
	  ff += nxBuf[index+ixs];
	}
	ff/= vv;
	

	// *tilBuf= ttab[min(255,max(0,ff))];
	ilBuf[index] = ttab[nxBuf[index]];

	//	++tilBuf;
	index++;
      }
    } 
#endif

  }

  void FillColorBuffer() {
    BYTE r=0xff,g=0xff,b=0xff;
    int pval = 0; 

    if(!terrain_dem_graphics.isTerrainLoaded())
      return;

    int gsize = iys*ixs;
    for (pval=0; pval< gsize; pval++) {
      if (hBuf[pval]<=0) {
	// water color
	r = 64;
	g = 96;
	b = 240;
      } else {
	TerrainColorMap(hBuf[pval],r,g,b);
	TerrainIllumination(ilBuf[pval], r,g,b);
      }
      sbuf->SetPoint(pval, r, g, b);
    }
    sbuf->Zoom(OVS);
  }

  void Draw(HDC hdc, RECT rc) {

    if(!terrain_dem_graphics.isTerrainLoaded())
      return;

    DWORD tm, tmstart;

    tmstart = GetTickCount();

    sbuf->HorizontalBlur(3); // now 51 ms!
    sbuf->VerticalBlur(3);

    tm = GetTickCount();
    tm = tm-tmstart;
    tmstart = GetTickCount();

    //    sbuf->Quantise();

    sbuf->DrawStretch(&hdc, rc); // 84 ms

    tm = GetTickCount();
    tm = tm-tmstart;
    tmstart = GetTickCount();

  }

};



//////////////////////////////////////////////////

TerrainRenderer *trenderer = NULL;

int CacheEfficiency = 0;
int Performance = 0;

void CloseTerrainRenderer() {
  if (trenderer) {
    delete trenderer;
  }
}

void OptimizeTerrainCache() 
{
  if (terrain_dem_graphics.DirectAccess)
    return;

  LockTerrainDataGraphics();

  if(!terrain_dem_graphics.isTerrainLoaded()) {
    UnlockTerrainDataGraphics();
    return;
  }
  if (terrain_dem_graphics.terraincachemisses > 0){
    DWORD tm =GetTickCount();
    terrain_dem_graphics.OptimizeCash();
    tm =GetTickCount()-tm;
  }
  
  UnlockTerrainDataGraphics();
}


void DrawTerrain( const HDC hdc, const RECT rc, const double sunazimuth, const double sunelevation)
{

  DWORD tm, tmstart;

  if(!terrain_dem_graphics.isTerrainLoaded())
    return;

  if (!trenderer) {
    trenderer = new TerrainRenderer(MapWindow::MapRectBig);
  }

  // step 1: calculate sunlight vector
  short sx, sy, sz;
  double fudgeelevation = (90.0-80.0*TerrainContrast/255.0);

  sx = (short)(-256*(fastcosine(fudgeelevation)*fastsine(sunazimuth)));
  sy = (short)(-256*(fastcosine(fudgeelevation)*fastcosine(sunazimuth)));
  sz = (short)(256*fastsine(fudgeelevation));

  // step 2: fill height buffer
  tmstart = GetTickCount();

  trenderer->Height(rc); // 106ms

  tm = GetTickCount();
  tm = tm-tmstart;
  tmstart = GetTickCount();

  CacheEfficiency = terrain_dem_graphics.terraincacheefficiency;
  //  Performance = tm;

  // step 3: calculate derivatives of height buffer
  trenderer->Slope(); // 15ms

  tm = GetTickCount();
  tm = tm-tmstart;
  tmstart = GetTickCount(); 

  // step 4: calculate illumination

  trenderer->Illumination(sx, sy, sz); // 4ms

  tm = GetTickCount();
  tm = tm-tmstart;
  tmstart = GetTickCount();

  // step 5: calculate colors

  trenderer->FillColorBuffer(); // 16 ms

  tm = GetTickCount();
  tm = tm-tmstart;
  tmstart = GetTickCount();

  // step 6: draw // 128 ms
  trenderer->Draw(hdc, MapWindow::MapRect); 

  tm = GetTickCount();
  tm = tm-tmstart;
  tmstart = GetTickCount();

}


///////////////

extern TCHAR szRegistryTopologyFile[];



void OpenTopology() {
  StartupStore(TEXT("OpenTopology\r\n"));

  static TCHAR  szFile[MAX_PATH] = TEXT("\0");
  static HANDLE hFile;
  static  TCHAR Directory[MAX_PATH];

  LockTerrainDataGraphics();

  for (int z=0; z<MAXTOPOLOGY; z++) {
    TopoStore[z] = 0;
  }
 
  GetRegistryString(szRegistryTopologyFile, szFile, MAX_PATH);
  ExpandLocalPath(szFile);

  SetRegistryString(szRegistryTopologyFile, TEXT("\0"));

  if (_tcslen(szFile)==0) {
    UnlockTerrainDataGraphics();
    return;
  }

  ExtractDirectory(Directory,szFile);

  hFile = NULL;
  hFile = CreateFile(szFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( hFile == NULL)
    {
      UnlockTerrainDataGraphics();
      return;
    }

  TCHAR ctemp[80];
  TCHAR TempString[200];
  TCHAR ShapeName[50];
  double ShapeRange;
  long ShapeIcon;
  long ShapeField;
  TCHAR wShapeFilename[200];
  TCHAR *Stop;
  int numtopo = 0;
  char ShapeFilename[200];

  if(hFile != INVALID_HANDLE_VALUE )
    {
      
      while(ReadString(hFile,200,TempString))
        {
          
          if(_tcslen(TempString) > 0 && _tcsstr(TempString,TEXT("*")) != TempString) // Look For Comment
            {

              BYTE red, green, blue;
              // filename,range,icon,field

              // File name
              PExtractParameter(TempString, ctemp, 0);
              _tcscpy(ShapeName, ctemp);

              _tcscpy(wShapeFilename, Directory);
              wcscat(wShapeFilename,ShapeName);
              wcscat(wShapeFilename,TEXT(".shp"));

              WideCharToMultiByte( CP_ACP, 0, wShapeFilename,
                                   _tcslen(wShapeFilename)+1, 
                                   ShapeFilename,   
                                   200, NULL, NULL);

              // Shape range
              PExtractParameter(TempString, ctemp, 1);
              ShapeRange = StrToDouble(ctemp,NULL);

              // Shape icon
              PExtractParameter(TempString, ctemp, 2);
              ShapeIcon = _tcstol(ctemp, &Stop, 10);

              // Shape field for text display

	      // sjt 02NOV05 - field parameter enabled
              PExtractParameter(TempString, ctemp, 3);
              if (iswalnum(ctemp[0])) {
		ShapeField = _tcstol(ctemp, &Stop, 10);
	        ShapeField--;
	      } else
		ShapeField = -1;

              // Red component of line / shading colour
              PExtractParameter(TempString, ctemp, 4);
              red = (BYTE)_tcstol(ctemp, &Stop, 10);

              // Green component of line / shading colour
              PExtractParameter(TempString, ctemp, 5);
              green = (BYTE)_tcstol(ctemp, &Stop, 10);

              // Blue component of line / shading colour
              PExtractParameter(TempString, ctemp, 6);
    		  blue = (BYTE)_tcstol(ctemp, &Stop, 10);
  
              if (ShapeField<0) {
                Topology* newtopo;
                newtopo = new Topology(ShapeFilename, RGB(red,green,blue));
                TopoStore[numtopo] = newtopo;
              } else {
                TopologyLabel *newtopol;
                newtopol = new TopologyLabel(ShapeFilename, 
					     RGB(red,green,blue),
					     ShapeField);
                TopoStore[numtopo] = newtopol;
              }
              if (ShapeIcon!=0) 
                TopoStore[numtopo]->loadBitmap(ShapeIcon);

              TopoStore[numtopo]->scaleThreshold = ShapeRange;

              numtopo++;
            }
        }

      CloseHandle (hFile);

      // file was OK, so save it
      ContractLocalPath(szFile);
      SetRegistryString(szRegistryTopologyFile, szFile);

    }
  UnlockTerrainDataGraphics();

}
