/*

Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$

}
*/

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "StdAfx.h"
#include "mapshape.h"

class XShape {
 public:
  XShape();
  virtual ~XShape();

  virtual void load(shapefileObj* shpfile, int i);
  virtual void clear();
  virtual void renderSpecial(HDC hdc, int x, int y) { (void)x; (void)y; (void)hdc; };

  bool hide;
  shapeObj shape;

};


class XShapeLabel: public XShape {
 public:
  XShapeLabel() {
    label= NULL;
  }
  virtual ~XShapeLabel();
  virtual void clear();
  char *label;
  void setlabel(const char* src);
  virtual void renderSpecial(HDC hdc, int x, int y);
};


class Topology {

 public:
  Topology(const char* shpname, COLORREF thecolor, bool doappend=false);
  Topology() {};

  ~Topology();

  void Open();
  void Close();

  void updateCache(rectObj thebounds, bool purgeonly=false);
  void Paint(HDC hdc, RECT rc);

  double scaleThreshold;

  bool CheckScale();
  void TriggerIfScaleNowVisible();

  bool triggerUpdateCache;
  int shapes_visible_count;

  XShape** shpCache;

  bool checkVisible(shapeObj& shape, rectObj &screenRect);

  void loadBitmap(const int);

  char filename[MAX_PATH];

  virtual void removeShape(const int i);
  virtual XShape* addShape(const int i);

 protected:

  void flushCache();

  bool append;
  bool in_scale;
  HPEN hPen;
  HBRUSH hbBrush;
  HBITMAP hBitmap;
  shapefileObj shpfile;
  bool shapefileopen;

};


class TopologyWriter: public Topology {
 public:
  TopologyWriter(const char *shpname, COLORREF thecolor);
  ~TopologyWriter();

  void addPoint(double x, double y);
  void Reset(void);
  void CreateFiles(void);
  void DeleteFiles(void);
};



class TopologyLabel: public Topology {
 public:
  TopologyLabel(const char* shpname, COLORREF thecolor, INT field1);
  ~TopologyLabel();
  virtual XShape* addShape(const int i);
  void setField(int i);
  int field;

};

#endif
