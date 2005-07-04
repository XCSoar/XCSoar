// omaplibdemo.cpp : Defines the entry point for the application.
//

#include "Terrain.h"
#include "MapWindow.h"
#include "Topology.h"
#include "resource.h"
#include "Calculations.h"
#include "STScreenBuffer.h"
#include "externs.h"
#include "Utils.h"
#include "VarioSound.h"
#include "Sizes.h"

//////////////////////////////////////////////////


Topology* TopoStore[MAXTOPOLOGY];

TopologyWriter *topo_marks;

rectObj GetRectBounds(RECT rc) {
  rectObj bounds;
  double xmin, xmax, ymin, ymax;
  double x;
  double y;

  x=0; y=0;
  GetLocationFromScreen(&x, &y);
  xmin = x; xmax = x;
  ymin = y; ymax = y;

  x = rc.left-100; y = rc.top-100;
  GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  x = rc.right+100; y = rc.top-100;
  GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  x = rc.left-100; y = rc.bottom+100;
  GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  x = rc.right+100; y = rc.bottom+100;
  GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  bounds.maxx = xmax;
  bounds.minx = xmin;
  bounds.maxy = ymax;
  bounds.miny = ymin;
  return bounds;

};


void SetTopologyBounds(RECT rcin) {
  static rectObj bounds;
  rectObj bounds_new;

  bounds = GetRectBounds(rcin);

  double threshold = BORDERFACTOR*max(bounds.maxx-bounds.minx,
                                      bounds.maxy-bounds.miny);
  bool recompute = false;

  // make bounds bigger than screen
  bounds_new.maxx = bounds.maxx+threshold;
  bounds_new.minx = bounds.minx-threshold;
  bounds_new.maxy = bounds.maxy+threshold;
  bounds_new.miny = bounds.miny-threshold;

  // only recalculate which shapes when bounds change significantly
  // need to have some trigger for this..

  // trigger if any border moves more than X% of screen width
  if (fabs(bounds_new.maxx-bounds.maxx)>threshold) {
    recompute = true;
  }
  if (fabs(bounds_new.minx-bounds.minx)>threshold) {
    recompute = true;
  }
  if (fabs(bounds_new.maxy-bounds.maxy)>threshold) {
    recompute = true;
  }
  if (fabs(bounds_new.miny-bounds.miny)>threshold) {
    recompute = true;
  }

  if (recompute) {
    bounds.maxx = bounds_new.maxx;
    bounds.maxy = bounds_new.maxy;
    bounds.minx = bounds_new.minx;
    bounds.miny = bounds_new.miny;

    if (EnableTopology) {

      LockTerrainData();
      for (int z=0; z<MAXTOPOLOGY; z++) {
        if (TopoStore[z]) {
          TopoStore[z]->updateCache(bounds);
        }
      }
      UnlockTerrainData();

    }
    topo_marks->updateCache(bounds);
  } else {
    if (topo_marks->triggerUpdateCache)
      topo_marks->updateCache(bounds);
  }
}


#include "MapWindow.h"


void ReadTopology() {

  LockTerrainData();
  topo_marks =
    new TopologyWriter("My Documents/xcsoar-marks", RGB(0xD0,0xD0,0xD0));

  topo_marks->scaleThreshold = 30.0;

  topo_marks->loadBitmap(IDB_MARK);
  UnlockTerrainData();
}


void CloseTopology() {

  LockTerrainData();
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      delete TopoStore[z];
    }
  }
  delete topo_marks;
  UnlockTerrainData();
}


void MarkLocation(double lon, double lat)
{
  LockTerrainData();

  if (EnableSoundModes) {
    PlayResource(TEXT("IDR_WAV_CLEAR"));
  }

  topo_marks->addPoint(lon, lat);
  topo_marks->triggerUpdateCache = true;
  UnlockTerrainData();
}

void DrawMarks (HDC hdc, RECT rc)
{

  LockTerrainData();
  topo_marks->Paint(hdc, rc);
  UnlockTerrainData();

}


void DrawTopology( HDC hdc, RECT rc)
{

  LockTerrainData();

  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      TopoStore[z]->Paint(hdc,rc);
    }
  }

  UnlockTerrainData();

}

////////


#define NUMTERRAINRAMP 12

COLORRAMP terrain_colors[] = {
  {-1,          0xff, 0xff, 0xff},
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
  {5000,        0xb7, 0xb9, 0xff}
};


void ColorRampLookup(short h, BYTE *r, BYTE *g, BYTE *b,
		     COLORRAMP* ramp_colors, int numramp) {

  int i;
  int tr, tg, tb;
  short f, of;

  // check if h lower than lowest
  if (h<=ramp_colors[0].h) {
    *r = ramp_colors[0].r;
    *g = ramp_colors[0].g;
    *b = ramp_colors[0].b;
    return;
  }
  // gone past end, so use last color
  if (h>=ramp_colors[numramp-1].h) {
    *r = ramp_colors[numramp-1].r;
    *g = ramp_colors[numramp-1].g;
    *b = ramp_colors[numramp-1].b;
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
      *r = tr/255;
      *g = tg/255;
      *b = tb/255;
      return;
    }
  }

}


void TerrainColorMap(short h, BYTE *r, BYTE *g, BYTE *b) {
  ColorRampLookup(h*8/4, r, g, b, terrain_colors, NUMTERRAINRAMP);
}


void TerrainIllumination(short illum, BYTE *r, BYTE *g, BYTE *b)
{
  static float contrast = (float)0.6;
  static short contrastpos = (short)(contrast * 255);
  static short contrastneg = (short)((1.0-contrast) * 255);

  short il = illum*contrastpos/256+contrastneg;
  *r = (BYTE)((int)*r*il/256);
  *g = (BYTE)((int)*g*il/256);
  *b = (BYTE)((int)*b*il/256);
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

    ixs = iround((rc.right-rc.left)/DTQUANT);
    iys = iround((rc.bottom-rc.top)/DTQUANT);

    sbuf = new CSTScreenBuffer();
    sbuf->Create(ixs*OVS,iys*OVS,RGB(0xff,0xff,0xff));
    ixs = sbuf->GetCorrectedWidth()/OVS;

    hBuf = (short*)malloc(sizeof(short)*ixs*iys);
    nxBuf = (short*)malloc(sizeof(short)*ixs*iys);
    nyBuf = (short*)malloc(sizeof(short)*ixs*iys);
    nzBuf = (short*)malloc(sizeof(short)*ixs*iys);
    ilBuf = (short*)malloc(sizeof(short)*ixs*iys);

    pixelsize = MapScale/30.0*DTQUANT;

  }

  int ixs, iys; // screen dimensions in coarse pixels

  CSTScreenBuffer *sbuf;

  double pixelsize;

  short *hBuf;
  short *nxBuf;
  short *nyBuf;
  short *nzBuf;
  short *ilBuf;

  void Height(RECT rc) {
    double X, Y;
    short X0, Y0;
    short X1, Y1;
    X0 = DTQUANT/2; // +rc.left
    Y0 = DTQUANT/2; // +rc.top
    X1 = X0+DTQUANT*ixs;
    Y1 = Y0+DTQUANT*iys;
    short* myhbuf = hBuf;
    LockTerrainData();

    // grid spacing = 250*rounding; m

    terrain_dem.SetTerrainRounding(pixelsize);
    kpixel = (float)(terrain_dem.GetTerrainSlopeStep()*3.0);
    // magnify gradient to make it
    // more obvious

    short pval = 0; // y*ixs+x;
    if (rc.top>0) {
      pval = ixs*(rc.top/DTQUANT-1);
      myhbuf+= pval;
      Y0 += rc.top-DTQUANT;
      Y1 -= rc.top-DTQUANT; // this is a cheat since we don't really know how
		    // far the bottom goes down
    }

    for (int y = Y0; y<Y1; y+= DTQUANT) {
      for (int x = X0; x<X1; x+= DTQUANT) {
        X = x;
        Y = y;
        GetLocationFromScreen(&X, &Y);
        *myhbuf = terrain_dem.GetTerrainHeight(Y, X);
	myhbuf++;
        // latitude, longitude
      }
    }
    UnlockTerrainData();
  }

  float kpixel;

  void Slope() {
    short mag;

    short nx, ny, nz;
    short pval=0;
    for (int y = 0; y<iys; y++) {
      for (int x = 0; x<ixs; x++) {

        if (x==0) {
          nx= (short)((hBuf[pval+1]-hBuf[pval])*kpixel*2);
        } else if (x==ixs-1) {
          nx= (short)((hBuf[pval]-hBuf[pval-1])*kpixel*2);
        } else {
          nx= (short)((hBuf[pval+1]-hBuf[pval-1])*kpixel);
        }
        if (y==0) {
          ny= (short)((hBuf[pval+ixs]-hBuf[pval])*kpixel*2);
        } else if (y==iys-1) {
          ny= (short)((hBuf[pval]-hBuf[pval-ixs])*kpixel*2);
        } else {
          ny= (short)((hBuf[pval+ixs]-hBuf[pval-ixs])*kpixel);
        }
        nz= 256;
        mag = isqrt4(nx*nx+ny*ny+nz*nz);

        nxBuf[pval] = nx*256/mag;
        nyBuf[pval] = ny*256/mag;
        nzBuf[pval] = nz*256/mag;

	pval++;
      }
    }
  }

  void Illumination(short sx, short sy, short sz) {
    short *tnxBuf = nxBuf;
    short *tnyBuf = nyBuf;
    short *tnzBuf = nzBuf;
    short *tilBuf = ilBuf;
    int mag;
    for (int i=0; i<ixs*iys; i++) {
      mag = (*tnxBuf*sx+*tnyBuf*sy+*tnzBuf*sz)/256;
      *tilBuf = max(0,(short)mag);

      tnxBuf++;
      tnyBuf++;
      tnzBuf++;
      tilBuf++;
    }
  }

  void FillColorBuffer() {
    BYTE r=0xff,g=0xff,b=0xff;
    short pval = 0; // y*ixs+x;
    for (int y = 0; y<iys; y++) {
      for (int x = 0; x<ixs; x++) {
        if (hBuf[pval]<=0) {
          // water color
          r = 64;
          g = 96;
          b = 240;
        } else {
          TerrainColorMap(hBuf[pval],&r,&g,&b);
          TerrainIllumination(ilBuf[pval], &r,&g,&b);
        }

        int ix0, iy0, ix1, iy1;
        ix0 = x*OVS;
        ix1 = ix0+OVS;
        iy0 = y*OVS;
        iy1 = iy0+OVS;

        for (int iy=iy0; iy< iy1; iy++) {
          for (int ix=ix0; ix< ix1; ix++) {
            sbuf->SetPoint(ix, iy, r, g, b);
          }
        }

	pval++;
      }
    }

  }

  void Draw(HDC hdc, RECT rc) {
    sbuf->Smooth2();
    sbuf->Quantise();
    sbuf->DrawStretch(&hdc, rc);
  }

};



//////////////////////////////////////////////////

TerrainRenderer *trenderer = NULL;
extern RECT MapRectBig;

void DrawTerrain( HDC hdc, RECT rc, double sunazimuth, double sunelevation)
{

  if (!trenderer) {
    trenderer = new TerrainRenderer(MapRectBig);
  }

  // step 1: calculate sunlight vector
  short sx, sy, sz;
  sx = (short)(256*(fastcosine(sunelevation)*fastsine(sunazimuth)));
  sy = (short)(256*(fastcosine(sunelevation)*fastcosine(sunazimuth)));
  sz = (short)(256*fastsine(sunelevation));

  // step 2: fill height buffer
  trenderer->Height(rc);

  // step 3: calculate derivatives of height buffer
  trenderer->Slope();

  // step 4: calculate illumination

  trenderer->Illumination(sx, sy, sz);

  // step 5: calculate colors

  trenderer->FillColorBuffer();

  // step 6: draw
  trenderer->Draw(hdc, MapRectBig);
}


///////////////

extern TCHAR szRegistryTopologyFile[];

void ExtractDirectory(TCHAR *Dest, TCHAR *Source) {
  int len = _tcslen(Source);
  int found = -1;
  int i;
  for (i=0; i<len; i++) {
    if ((Source[i]=='/')||(Source[i]=='\\')) {
      found = i;
    }
  }
  for (i=0; i<=found; i++) {
    Dest[i]= Source[i];
  }
  Dest[i]= 0;
}


void OpenTopology() {
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");
  static HANDLE hFile;
  static  TCHAR Directory[MAX_PATH];

  LockTerrainData();

  for (int z=0; z<MAXTOPOLOGY; z++) {
    TopoStore[z] = 0;
  }

  GetRegistryString(szRegistryTopologyFile, szFile, MAX_PATH);

  if (!szFile[0]) {
    UnlockTerrainData();
    return;
  }

  ExtractDirectory(Directory,szFile);

  hFile = NULL;
  hFile = CreateFile(szFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( hFile == NULL)
    {
      UnlockTerrainData();
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

          if(_tcsstr(TempString,TEXT("*")) != TempString) // Look For Comment
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
              PExtractParameter(TempString, ctemp, 3);
              ShapeField = _tcstol(ctemp, &Stop, 10);

              // Shape field for text display
              PExtractParameter(TempString, ctemp, 4);
              red = (BYTE)_tcstol(ctemp, &Stop, 10);

              // Shape field for text display
              PExtractParameter(TempString, ctemp, 5);
              green = (BYTE)_tcstol(ctemp, &Stop, 10);

              // Shape field for text display
              PExtractParameter(TempString, ctemp, 6);

			  blue = (BYTE)_tcstol(ctemp, &Stop, 10);

              if (ShapeField==0) {
                Topology* newtopo;
                newtopo = new Topology(ShapeFilename, RGB(red,green,blue));
                TopoStore[numtopo] = newtopo;
              } else {
                TopologyLabel *newtopol;
                newtopol = new TopologyLabel(ShapeFilename, RGB(red,green,blue));
                TopoStore[numtopo] = newtopol;
              }
              if (ShapeIcon!=0)
                TopoStore[numtopo]->loadBitmap(ShapeIcon);

              TopoStore[numtopo]->scaleThreshold = ShapeRange;

              numtopo++;
            }
        }
      CloseHandle (hFile);
    }
  UnlockTerrainData();

}
