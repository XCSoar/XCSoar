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

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "Screen/shapelib/mapshape.h"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Bitmap.hpp"

class Canvas;
class MapWindowProjection;
class MapWindow;
class LabelBlock;

class XShape {
 public:
  XShape();
  virtual ~XShape();

  virtual void load(shapefileObj* shpfile, int i);
  virtual void clear();
  virtual void renderSpecial(Canvas &canvas, LabelBlock &label_block, int x, int y) {
    (void)canvas; (void)x; (void)y;
  };

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
  virtual void renderSpecial(Canvas &canvas, LabelBlock &label_block, int x, int y);
};


class Topology {

 public:
  Topology(const char* shpname, const Color thecolor, bool doappend=false);
  Topology() {};

  ~Topology();

  void Open();
  void Close();
  void TriggerIfScaleNowVisible(MapWindowProjection &map_projection);

  void updateCache(MapWindowProjection &map_projection,
		   rectObj thebounds, bool purgeonly=false);
  void Paint(Canvas &canvas, MapWindow &m_window, const RECT rc);

  double scaleThreshold;
  void loadBitmap(const int);
  bool triggerUpdateCache;
  int getNumVisible() {
    return shapes_visible_count;
  }
 private:

  bool CheckScale(double map_scale);

  int shapes_visible_count;

  XShape** shpCache;

  bool checkVisible(shapeObj& shape, rectObj &screenRect);

  virtual void removeShape(const int i);
  virtual XShape* addShape(const int i);

 protected:
  char filename[MAX_PATH];

  void flushCache();

  bool append;
  bool in_scale;
  Pen hPen;
  Brush hbBrush;
  Bitmap hBitmap;
  shapefileObj shpfile;
  bool shapefileopen;

};


class TopologyWriter: public Topology {
 public:
  TopologyWriter(const char *shpname, const Color thecolor);
  ~TopologyWriter();

  void addPoint(double x, double y);
  void Reset(void);
  void CreateFiles(void);
  void DeleteFiles(void);
};



class TopologyLabel: public Topology {
 public:
  TopologyLabel(const char* shpname, const Color thecolor, INT field1);
  ~TopologyLabel();
  virtual XShape* addShape(const int i);
  void setField(int i);
  int field;

};

#endif
