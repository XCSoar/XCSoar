/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts
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
#include "externs.h"

extern HFONT MapLabelFont;

XShape::XShape() {

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


Topology::Topology(char* shpname, COLORREF thecolor):append(FALSE) {
  filename = (char *) malloc(strlen(shpname)+1);
  strcpy( filename, shpname );
  hPen = (HPEN)CreatePen(PS_SOLID, 1, thecolor);
  hbBrush=(HBRUSH)CreateSolidBrush(thecolor);
  Open();
}


void Topology::Open() {

  int i;
  if (append) {
    if (msSHPOpenFile(&shpfile, "rb+", filename) == -1) {
      shapefileopen = false;
      return;
    }
  } else {
    if (msSHPOpenFile(&shpfile, "rb", filename) == -1) {
      shapefileopen = false;
      return;
    }
  }

  shapefileopen = true;
  scaleThreshold = 1000.0;
  shpCache = (XShape**)malloc(sizeof(XShape*)*shpfile.numshapes);
  if (shpCache) {
    for (i=0; i<shpfile.numshapes; i++) {
      shpCache[i] = NULL;
    }
  } else {
    shapefileopen = false;
  }
}


void Topology::Close() {
  if (shapefileopen) {
    for (int i=0; i<shpfile.numshapes; i++) {
      if (shpCache[i] != NULL) {
        delete shpCache[i];
        shpCache[i]= NULL;
      }
    }
    free(shpCache);
    msSHPCloseFile(&shpfile);
	shapefileopen = false;  // added sgi
  }
}


Topology::~Topology() {
  Close();
  DeleteObject((HPEN)hPen);
  DeleteObject((HBRUSH)hbBrush);
  free(filename);
  if (hBitmap) {
    DeleteObject(hBitmap);
  }
}


void Topology::updateCache(rectObj thebounds) {

  if (!shapefileopen) return;

  if (MapWindow::MapScale > scaleThreshold) {
    // not visible, so flush the cache
    // otherwise we waste time on looking up which shapes are in bounds
    for (int i=0; i<shpfile.numshapes; i++) {
      if (shpCache[i] != NULL) {
	delete shpCache[i];
	shpCache[i]= NULL;
      }
    }
    // note that the trigger will still be active, because as soon as
    // the map scale comes within range, we will need to regenerate the cache
    return;
  }

  triggerUpdateCache = false;

  msSHPWhichShapes(&shpfile, thebounds, 0);
  if (!shpfile.status) {
    // this happens if entire shape is out of range
    return;
  }
  int nn = 0;
  for (int i=0; i<shpfile.numshapes; i++) {

    if (msGetBit(shpfile.status, i)) {

      if (shpCache[i]==NULL) {
	// shape is now in range, and wasn't before
	shpCache[i] = addShape(i);
      }

    } else {

      if (shpCache[i] != NULL) {
	removeShape(i);
	shpCache[i]= NULL;
      }
    }
  }
}


XShape* Topology::addShape(int i) {
  XShape* theshape = new XShape();
  theshape->load(&shpfile,i);
  return theshape;
}


void Topology::removeShape(int i) {
  delete shpCache[i];
}


////


bool Topology::checkVisible(shapeObj* shape, rectObj *screenRect) {
  return (msRectOverlap(&shape->bounds, screenRect) == MS_TRUE);
}


void CalculateScreenBounds(RECT rc, rectObj *screenRect) {
  double xmin, xmax, ymin, ymax;
  double x, y;

  x = rc.left;
  y = rc.top;
  MapWindow::GetLocationFromScreen(&x, &y);
  xmin = x; xmax = x;
  ymin = y; ymax = y;

  x = rc.right;
  y = rc.top;
  MapWindow::GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  x = rc.right;
  y = rc.bottom;
  MapWindow::GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  x = rc.left;
  y = rc.bottom;
  MapWindow::GetLocationFromScreen(&x, &y);
  xmin = min(xmin, x); xmax = max(xmax, x);
  ymin = min(ymin, y); ymax = max(ymax, y);

  screenRect->minx = xmin;
  screenRect->maxx = xmax;
  screenRect->miny = ymin;
  screenRect->maxy = ymax;
}



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

  double tpp_x=0.0;
  double tpp_y=0.0;

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

  rectObj screenRect;

  CalculateScreenBounds(rc, &screenRect);

  POINT *pt= NULL;

  for (int ixshp = 0; ixshp < shpfile.numshapes; ixshp++) {

    if (!shpCache[ixshp]) continue;

    shapeObj *shape = &(shpCache[ixshp]->shape);

    int minx = rc.right;
    int miny = rc.bottom;

    switch(shape->type) {

      ///////////////////////////////////////
    case(MS_SHAPE_POINT):{

      if (checkVisible(shape, &screenRect))
      for (int tt = 0; tt < shape->numlines; tt++) {

	for (int jj=0; jj< shape->line[tt].numpoints; jj++) {

	  int x, y;

	  tpp_x = shape->line[tt].point[jj].x;
	  tpp_y = shape->line[tt].point[jj].y;

	  MapWindow::LatLon2Screen(tpp_x, tpp_y, &x, &y);

	  MapWindow::DrawBitmapIn(hdc, x, y, hBitmap);

	  shpCache[ixshp]->renderSpecial(hdc,x,y);

	}
      }

    }; break;

    case(MS_SHAPE_LINE):{

      if (checkVisible(shape, &screenRect))
      for (int tt = 0; tt < shape->numlines; tt ++) {

	pt = (POINT*)SfRealloc(pt,sizeof(POINT)*shape->line[tt].numpoints);

	for (int jj=0; jj< shape->line[tt].numpoints; jj++) {
	  int x, y;

	  tpp_x = shape->line[tt].point[jj].x;
	  tpp_y = shape->line[tt].point[jj].y;
	  MapWindow::LatLon2Screen(tpp_x, tpp_y, &x, &y);

	  if (x<=minx) {
	    minx = x;
	    miny = y;
	  }

	  pt[jj].x = x;
	  pt[jj].y = y;

	}
	Polyline(hdc, pt, shape->line[tt].numpoints);
	shpCache[ixshp]->renderSpecial(hdc,minx,miny);

      }
    }
      break;


      /////////////////////////////////////////////////////
	case(MS_SHAPE_POLYGON):{

	  if (checkVisible(shape, &screenRect))
	  for (int tt = 0; tt < shape->numlines; tt ++) {

	    pt = (POINT*)SfRealloc(pt,sizeof(POINT)*shape->line[tt].numpoints/iskip);

	    for (int jj=0; jj< shape->line[tt].numpoints/iskip; jj++) {
	      int x, y;

	      tpp_x = shape->line[tt].point[jj*iskip].x;
	      tpp_y = shape->line[tt].point[jj*iskip].y;
		  MapWindow::LatLon2Screen(tpp_x, tpp_y, &x, &y);

	      pt[jj].x = x;
	      pt[jj].y = y;

	      // find leftmost visible point for display of label
	      if ((x<=minx)&&(x>=rc.left)&&(y>=rc.top)&&(y<=rc.bottom)) {
		minx = x;
		miny = y;
	      }

	    }
	    Polygon(hdc, pt, shape->line[tt].numpoints/iskip);
	    shpCache[ixshp]->renderSpecial(hdc,minx,miny);

	  }
	}
	  break;

	  ///////////////////////////////////
	    default:;
    }
  }
  //
  if (pt) free(pt);

  SelectObject(hdc, hbOld);
  SelectObject(hdc, hpOld);
  SelectObject(hdc, (HFONT)hfOld);

}


///////////////////////////////////////////////////////////


TopologyLabel::TopologyLabel(char* shpname, COLORREF thecolor):Topology(shpname, thecolor)
{
  field = 0;
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
  // TODO: draw label at x,y
  if (label) {
    TCHAR Temp[100];
    wsprintf(Temp,TEXT("%S"),label);
    SetBkMode(hDC,TRANSPARENT);
    ExtTextOut(hDC, x+2, y+2, 0, NULL, Temp, _tcslen(Temp), NULL);
  }
}


void XShapeLabel::setlabel(const char* src) {
  if (strcmp(src,"UNK") != 0) {
    label = _strdup(src);
  } else {
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



TopologyWriter::TopologyWriter(char* shpname, COLORREF thecolor):
  Topology(shpname, thecolor) {
  char *dbfname;

  dbfname = (char*)malloc(strlen(shpname)+4);
  strcpy(dbfname, shpname );
  strcat(dbfname, ".dbf");
  strcpy(filename, shpname );

  append= true;

  if (shapefileopen) {
    Close();
  }
  // by default, now, this overwrites previous contents
  if (msSHPCreateFile(&shpfile, shpname, SHP_POINT) == -1) {
  } else {
    shpfile.hDBF = msDBFCreate(dbfname);
    if (shpfile.hDBF) {
      msDBFClose(shpfile.hDBF);
    }
    Close();
  }
  Open();
}


void TopologyWriter::addPoint(double x, double y) {
  pointObj p = {x,y};

  if (shapefileopen) {
    msSHPWritePoint(shpfile.hSHP, &p);
    Close();
  }
  append = true;
  Open();

}

