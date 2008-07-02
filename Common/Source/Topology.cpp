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

#include "stdafx.h"

#include "Topology.h"
#include "options.h"
#include "externs.h"
#include "wcecompat/ts_string.h"

extern HFONT MapLabelFont;

XShape::XShape() {
  hide=false;
}


XShape::~XShape() {
  clear();
}


void XShape::clear() {
  msFreeShape(&shape);
}


void XShape::load(shapefileObj* shpfile, int i) {
  msInitShape(&shape);
  msSHPReadShape(shpfile->hSHP, i, &shape);
}


void Topology::loadBitmap(int xx) {
  hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(xx));
}


Topology::Topology(char* shpname, COLORREF thecolor, bool doappend) {

  append = doappend;
  memset((void*)&shpfile, 0 ,sizeof(shpfile));
  shapefileopen = false;
  triggerUpdateCache = false;
  scaleThreshold = 0;
  shpCache= NULL;
  hBitmap = NULL;

  in_scale = false;

  strcpy( filename, shpname );
  hPen = (HPEN)CreatePen(PS_SOLID, 1, thecolor);
  hbBrush=(HBRUSH)CreateSolidBrush(thecolor);
  Open();
}


void Topology::Open() {

  shapefileopen = false;

  if (append) {
    if (msSHPOpenFile(&shpfile, "rb+", filename) == -1) {
      return;
    }
  } else {
    if (msSHPOpenFile(&shpfile, "rb", filename) == -1) {
      return;
    }
  }

  scaleThreshold = 1000.0;
  shpCache = (XShape**)malloc(sizeof(XShape*)*shpfile.numshapes);
  if (shpCache) {
    shapefileopen = true;
    for (int i=0; i<shpfile.numshapes; i++) {
      shpCache[i] = NULL;
    }
  }
}


void Topology::Close() {
  if (shapefileopen) {
    if (shpCache) {
      flushCache();
      free(shpCache); shpCache = NULL;
    }
    msSHPCloseFile(&shpfile);
    shapefileopen = false;  // added sgi
  }
}


Topology::~Topology() {
  Close();
  DeleteObject((HPEN)hPen);
  DeleteObject((HBRUSH)hbBrush);
  if (hBitmap) {
    DeleteObject(hBitmap);
  }
}


bool Topology::CheckScale(void) {
  return (MapWindow::MapScale <= scaleThreshold);
}

void Topology::TriggerIfScaleNowVisible(void) {
  triggerUpdateCache |= (CheckScale() != in_scale);
}

void Topology::flushCache() {
  for (int i=0; i<shpfile.numshapes; i++) {
    removeShape(i);
  }
  shapes_visible_count = 0;
}

void Topology::updateCache(rectObj thebounds, bool purgeonly) {
  if (!triggerUpdateCache) return;

  if (!shapefileopen) return;

  in_scale = CheckScale();

  if (!in_scale) {
    // not visible, so flush the cache
    // otherwise we waste time on looking up which shapes are in bounds
    flushCache();
    triggerUpdateCache = false;
    return;
  }

  if (purgeonly) return;

  triggerUpdateCache = false;

  msSHPWhichShapes(&shpfile, thebounds, 0);
  if (!shpfile.status) {
    // this happens if entire shape is out of range
    // so clear buffer.
    flushCache();
    return;
  }

  shapes_visible_count = 0;

  for (int i=0; i<shpfile.numshapes; i++) {

    if (msGetBit(shpfile.status, i)) {

      if (shpCache[i]==NULL) {
	// shape is now in range, and wasn't before
	shpCache[i] = addShape(i);
      }
      shapes_visible_count++;
    } else {
      removeShape(i);
    }
  }
}


XShape* Topology::addShape(int i) {
  XShape* theshape = new XShape();
  theshape->load(&shpfile,i);
  return theshape;
}


void Topology::removeShape(int i) {
  if (shpCache[i]) {
    delete shpCache[i];
    shpCache[i]= NULL;
  }
}



bool Topology::checkVisible(shapeObj* shape, rectObj *screenRect) {
  return (msRectOverlap(&shape->bounds, screenRect) == MS_TRUE);
}


///////////////

void Topology::Paint(HDC hdc, RECT rc) {

  if (!shapefileopen) return;

  if (MapWindow::MapScale > scaleThreshold)
    return;

  // TODO: only draw inside screen!
  // this will save time with rendering pixmaps especially

  HPEN  hpOld;
  HBRUSH hbOld;
  HFONT hfOld;

  hpOld = (HPEN)SelectObject(hdc,hPen);
  hbOld = (HBRUSH)SelectObject(hdc, hbBrush);
  hfOld = (HFONT)SelectObject(hdc, MapLabelFont);

  // get drawing info

  int iskip = 1;

  if (MapWindow::MapScale>0.25*scaleThreshold) {
    iskip = 2;
  }
  if (MapWindow::MapScale>0.5*scaleThreshold) {
    iskip = 3;
  }
  if (MapWindow::MapScale>0.75*scaleThreshold) {
    iskip = 4;
  }

  rectObj screenRect = MapWindow::CalculateScreenBounds(0.0);

  static POINT pt[MAXCLIPPOLYGON];

  for (int ixshp = 0; ixshp < shpfile.numshapes; ixshp++) {

    if (!shpCache[ixshp]) continue;
    if (shpCache[ixshp]->hide) continue;

    shapeObj *shape = &(shpCache[ixshp]->shape);

    switch(shape->type) {

        ///////////////////////////////////////
      case(MS_SHAPE_POINT):{

        if (checkVisible(shape, &screenRect))
          for (int tt = 0; tt < shape->numlines; tt++) {

            for (int jj=0; jj< shape->line[tt].numpoints; jj++) {

              POINT sc;
              MapWindow::LatLon2Screen(shape->line[tt].point[jj].x,
                                       shape->line[tt].point[jj].y,
                                       sc);
              MapWindow::DrawBitmapIn(hdc, sc, hBitmap);

              shpCache[ixshp]->renderSpecial(hdc, sc.x, sc.y);

          }
        }

      }; break;

    case(MS_SHAPE_LINE):

      if (checkVisible(shape, &screenRect))
        for (int tt = 0; tt < shape->numlines; tt ++) {

          int minx = rc.right;
          int miny = rc.bottom;
          int msize = min(shape->line[tt].numpoints, MAXCLIPPOLYGON);
          for (int jj=0; jj< msize; jj++) {

            MapWindow::LatLon2Screen(shape->line[tt].point[jj].x,
                                     shape->line[tt].point[jj].y,
                                     pt[jj]);
            if (pt[jj].x<=minx) {
              minx = pt[jj].x;
              miny = pt[jj].y;
            }

          }
          ClipPolygon(hdc, pt, msize, rc, false);
          shpCache[ixshp]->renderSpecial(hdc,minx,miny);
        }
      break;

    case(MS_SHAPE_POLYGON):

      if (checkVisible(shape, &screenRect))
        for (int tt = 0; tt < shape->numlines; tt ++) {

          int minx = rc.right;
          int miny = rc.bottom;
          int msize = min(shape->line[tt].numpoints/iskip, MAXCLIPPOLYGON);

          for (int jj=0; jj< msize; jj++) {
            MapWindow::LatLon2Screen(shape->line[tt].point[jj*iskip].x,
                                     shape->line[tt].point[jj*iskip].y,
                                     pt[jj]);
            if (pt[jj].x<=minx) {
              minx = pt[jj].x;
              miny = pt[jj].y;
            }
          }
          ClipPolygon(hdc,pt, msize, rc, true);
          shpCache[ixshp]->renderSpecial(hdc,minx,miny);
        }
      break;

    default:
      break;
    }
  }

  SelectObject(hdc, hbOld);
  SelectObject(hdc, hpOld);
  SelectObject(hdc, (HFONT)hfOld);

}


///////////////////////////////////////////////////////////


TopologyLabel::TopologyLabel(char* shpname, COLORREF thecolor, int field1):Topology(shpname, thecolor)
{
  //sjt 02nov05 - enabled label fields
  setField(max(0,field1));
  // JMW this is causing XCSoar to crash on my system!
};

TopologyLabel::~TopologyLabel()
{
}


void TopologyLabel::setField(int i) {
  field = i;
}

XShape* TopologyLabel::addShape(int i) {

  XShapeLabel* theshape = new XShapeLabel();
  theshape->load(&shpfile,i);
  theshape->setlabel(msDBFReadStringAttribute( shpfile.hDBF, i, field));
  return theshape;
}


void XShapeLabel::renderSpecial(HDC hDC, int x, int y) {
  if (label && (MapWindow::DeclutterLabels<2)) {

    TCHAR Temp[100];
    wsprintf(Temp,TEXT("%S"),label);
    SetBkMode(hDC,TRANSPARENT);
    if (ispunct(Temp[0])){
      DOUBLE dTemp;

      Temp[0]='0';
      dTemp = StrToDouble(Temp,NULL);
      dTemp = ALTITUDEMODIFY*dTemp;
      if (dTemp > 999)
	wsprintf(Temp,TEXT("%.1f"),(dTemp/1000));
      else
	wsprintf(Temp,TEXT("%d"),int(dTemp));
    }
    int size = _tcslen(Temp);

    SIZE tsize;
    RECT brect;
    GetTextExtentPoint(hDC, Temp, size, &tsize);
    x+= 2;
    y+= 2;
    brect.left = x;
    brect.right = brect.left+tsize.cx;
    brect.top = y;
    brect.bottom = brect.top+tsize.cy;

    if (!MapWindow::checkLabelBlock(brect))
      return;

    SetTextColor(hDC, RGB(0x20,0x20,0x20));
    ExtTextOut(hDC, x, y, 0, NULL, Temp, size, NULL);

  }
}


void XShapeLabel::setlabel(const char* src) {
  if (src &&
      (strcmp(src,"UNK") != 0) &&
      (strcmp(src,"RAILWAY STATION") != 0) &&
      (strcmp(src,"RAILROAD STATION") != 0)
      ) {
    if (label) free(label);
    label = (char*)malloc(strlen(src)+1);
    if (label) {
      strcpy(label,src);
    }
    hide=false;
  } else {
    if (label) {
      free(label);
      label= NULL;
    }
    hide=true;
  }
}


XShapeLabel::~XShapeLabel() {
  if (label) {
    free(label);
    label= NULL;
  }
}



void XShapeLabel::clear() {
  XShape::clear();
  if (label) {
    free(label);
    label= NULL;
  }
}

//       wsprintf(Scale,TEXT("%1.2f%c"),MapScale, autozoomstring);


/////////////////////////////////////////////////////////

TopologyWriter::~TopologyWriter() {
  if (shapefileopen) {
    Close();
    DeleteFiles();
  }
}


TopologyWriter::TopologyWriter(char* shpname, COLORREF thecolor):
  Topology(shpname, thecolor, true) {

  Reset();
}


void TopologyWriter::DeleteFiles(void) {
  // Delete all files, since zziplib interface doesn't handle file modes
  // properly
  if (strlen(filename)>0) {
    TCHAR fname[MAX_PATH];
    ascii2unicode(filename, fname);
    _tcscat(fname, TEXT(".shp"));
    DeleteFile(fname);
    ascii2unicode(filename, fname);
    _tcscat(fname, TEXT(".shx"));
    DeleteFile(fname);
    ascii2unicode(filename, fname);
    _tcscat(fname, TEXT(".dbf"));
    DeleteFile(fname);
  }
}


void TopologyWriter::CreateFiles(void) {
  // by default, now, this overwrites previous contents
  if (msSHPCreateFile(&shpfile, filename, SHP_POINT) == -1) {
  } else {
    char dbfname[100];
    strcpy(dbfname, filename );
    strcat(dbfname, ".dbf");
    shpfile.hDBF = msDBFCreate(dbfname);

    shapefileopen=true;
    Close();
  }
}


void TopologyWriter::Reset(void) {
  if (shapefileopen) {
    Close();
  }

  DeleteFiles();
  CreateFiles();

  Open();
}


void TopologyWriter::addPoint(double x, double y) {
  pointObj p = {x,y};

  if (shapefileopen) {
    msSHPWritePoint(shpfile.hSHP, &p);
    Close();
  }
  Open();

}



/////////////////////



// The "OutputToInput" function sets the resulting polygon of this
// step up to be the input polygon for next step of the clipping
// algorithm. As the Sutherland-Hodgman algorithm is a polygon
// clipping algorithm, it does not handle line clipping very well. The
// modification so that lines may be clipped as well as polygons is
// included in this function. The code for this function is:

void OutputToInput(unsigned int *inLength,
                   POINT *inVertexArray,
                   unsigned int *outLength,
                   POINT *outVertexArray )
{
  if ((*inLength==2) && (*outLength==3)) //linefix
    {
      inVertexArray[0].x=outVertexArray [0].x;
      inVertexArray[0].y=outVertexArray [0].y;
      if ((outVertexArray[0].x==outVertexArray[1].x)
          && (outVertexArray[0].y==outVertexArray[1].y)) /*First two vertices
                                                      are same*/
        {
          inVertexArray[1].x=outVertexArray [2].x;
          inVertexArray[1].y=outVertexArray [2].y;
        }
      else                    /*First vertex is same as third vertex*/
        {
          inVertexArray[1].x=outVertexArray [1].x;
          inVertexArray[1].y=outVertexArray [1].y;
        }

      *inLength=2;

    }
  else  /* set the outVertexArray as inVertexArray for next step*/
    {
      *inLength= *outLength;
      memcpy((void*)inVertexArray, (void*)outVertexArray,
             (*outLength)*sizeof(POINT));
    }
}


// The "Inside" function returns TRUE if the vertex tested is on the
// inside of the clipping boundary. "Inside" is defined as "to the
// left of clipping boundary when one looks from the first vertex to
// the second vertex of the clipping boundary". The code for this
// function is:

bool Inside (const POINT *testVertex, const POINT *clipBoundary)
{
  if (clipBoundary[1].x > clipBoundary[0].x)              /*bottom edge*/
    if (testVertex->y <= clipBoundary[0].y) return TRUE;
  if (clipBoundary[1].x < clipBoundary[0].x)              /*top edge*/
   if (testVertex->y >= clipBoundary[0].y) return TRUE;
  if (clipBoundary[1].y < clipBoundary[0].y)              /*right edge*/
    if (testVertex->x <= clipBoundary[1].x) return TRUE;
  if (clipBoundary[1].y > clipBoundary[0].y)              /*left edge*/
    if (testVertex->x >= clipBoundary[1].x) return TRUE;
  return FALSE;
}

// The "Intersect" function calculates the intersection of the polygon
// edge (vertex s to p) with the clipping boundary. The code for this
// function is:


void Intersect (POINT first, POINT  second, POINT  *clipBoundary,
                POINT *intersectPt)
{
  float f;
  if (clipBoundary[0].y==clipBoundary[1].y)     /*horizontal*/
   {
     intersectPt->y=clipBoundary[0].y;
     if (second.y != first.y) {
       f = ((float)(second.x-first.x))/((float)(second.y-first.y));
       intersectPt->x= first.x + (long)(((clipBoundary[0].y-first.y)*f));
     } else {
       intersectPt->x = first.x;
     }
   } else { /*Vertical*/
    intersectPt->x=clipBoundary[0].x;
    if (second.x != first.x) {
      f = ((float)(second.y-first.y))/((float)(second.x-first.x));
      intersectPt->y=first.y + (long)(((clipBoundary[0].x-first.x)*f));
    } else {
      intersectPt->y = first.y;
    }
  }
}


// The "Output" function moves "newVertex" to "outVertexArray" and
// updates "outLength".

void Output(const POINT *newVertex,
            unsigned int *outLength, POINT *outVertexArray)
{
  if (*outLength) {
    if ((newVertex->x == outVertexArray[*outLength-1].x)
        &&(newVertex->y == outVertexArray[*outLength-1].y)) {
      // no need for duplicates
      return;
    }
  }
  outVertexArray[*outLength].x= newVertex->x;
  outVertexArray[*outLength].y= newVertex->y;
  (*outLength)++;
}


void SutherlandHodgmanPolygoClip (POINT* inVertexArray,
                                  POINT* outVertexArray,
                                  unsigned int inLength,
                                  unsigned int *outLength,
                                  POINT *clipBoundary, bool fill)
{
  POINT *s, *p; /*Start, end point of current polygon edge*/
  POINT i;   /*Intersection point with a clip boundary*/
  unsigned int j;       /*Vertex loop counter*/
  *outLength = 0;

  if (inLength<1) return;

  s = inVertexArray + inLength-1;
  p = inVertexArray;
  /*Start with the last vertex in inVertexArray*/
  for (j=inLength; j--;)
    {
      /*Now s and p correspond to the vertices*/
      if (fill || (p!=inVertexArray)) {
        if (Inside(p,clipBoundary))      /*Cases 1 and 4*/
          {
            if (Inside(s, clipBoundary))
              {
                Output(p, outLength, outVertexArray); /*Case 1*/
              }
            else                            /*Case 4*/
              {
                Intersect(*s, *p, clipBoundary, &i);
                Output(&i, outLength, outVertexArray);
                Output(p, outLength, outVertexArray);
              }
          }
        else                   /*Cases 2 and 3*/
          {
            if (Inside(s, clipBoundary))  /*Cases 2*/
              {
                Output(p, outLength, outVertexArray);
              }
          }                          /*No action for case 3*/
      } else {
        if (Inside(p, clipBoundary))
          Output(p, outLength, outVertexArray); /*Case 1*/
      }
      /*Advance to next pair of vertices*/
      s = p;
      p++;
    }
}


static POINT clip_ptout[MAXCLIPPOLYGON];
static POINT clip_ptin[MAXCLIPPOLYGON];

void ClipPolygon(HDC hdc, POINT *m_ptin, unsigned int inLength,
                 RECT rc, bool fill) {
  POINT edge[2];
  unsigned int outLength = 0;

  if (inLength>=MAXCLIPPOLYGON-1) {
    inLength=MAXCLIPPOLYGON-2;
  }
  if (inLength<2) {
    return;
  }

  memcpy((void*)clip_ptin, (void*)m_ptin, inLength*sizeof(POINT));

  // add extra point for final point if it doesn't equal the first
  // this is required to close some airspace areas that have missing
  // final point
  if (fill) {
    if ((m_ptin[inLength-1].x != m_ptin[0].x) &&
	(m_ptin[inLength-1].y != m_ptin[0].y)) {
      clip_ptin[inLength] = clip_ptin[0];
      inLength++;
    }
  }

  rc.top--;
  rc.bottom++;
  rc.left--;
  rc.right++;

  // LEFT EDGE
  // Top_Left_Vertex_of_Clipping_Window;
  edge[0].x = rc.left;
  edge[0].y = rc.top;
  // Bottom_Left_Vertex_of_Clipping_Window;
  edge[1].x = rc.left;
  edge[1].y = rc.bottom;

  SutherlandHodgmanPolygoClip (clip_ptin, clip_ptout,
                               inLength,
                               &outLength, edge, fill);

  OutputToInput(&inLength, clip_ptin, &outLength, clip_ptout);

  // BOTTOM EDGE
  // Bottom_Left_Vertex_of_Clipping_Window;
  edge[0].x = rc.left;
  edge[0].y = rc.bottom;
  // Bottom_Right_Vertex_of_Clipping_Window;
  edge[1].x = rc.right;
  edge[1].y = rc.bottom;

  SutherlandHodgmanPolygoClip (clip_ptin, clip_ptout,
                               inLength,
                               &outLength, edge, fill);
  OutputToInput(&inLength, clip_ptin, &outLength, clip_ptout);

  // RIGHT EDGE
  // Bottom_Right_Vertex_of_Clipping_Window;
  edge[0].x = rc.right;
  edge[0].y = rc.bottom;
  // Top_Right_Vertex_of_Clipping_Window;
  edge[1].x = rc.right;
  edge[1].y = rc.top;

  SutherlandHodgmanPolygoClip (clip_ptin, clip_ptout,
                               inLength,
                               &outLength, edge, fill);
  OutputToInput(&inLength, clip_ptin, &outLength, clip_ptout);

  // TOP EDGE
  // Top_Right_Vertex_of_Clipping_Window;
  edge[0].x = rc.right;
  edge[0].y = rc.top;
  // Top_Left_Vertex_of_Clipping_Window;
  edge[1].x = rc.left;
  edge[1].y = rc.top;

  SutherlandHodgmanPolygoClip (clip_ptin, clip_ptout,
                               inLength,
                               &outLength, edge, fill);

  OutputToInput(&inLength, clip_ptin, &outLength, clip_ptout);

  if (fill) {
    if (outLength>2) {
      Polygon(hdc, clip_ptout, outLength);
    }
  } else {
    if (outLength>1) {
      Polyline(hdc, clip_ptout, outLength);
    }
  }
}


