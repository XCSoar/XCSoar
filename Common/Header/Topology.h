#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "stdafx.h"
#include "mapshape.h"
#include "MapWindow.h"


class XShape {
 public:
  XShape();
  virtual ~XShape();

  virtual void load(shapefileObj* shpfile, int i);
  virtual void clear();
  virtual void renderSpecial(HDC hdc, int x, int y) {};

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
  Topology(char* shpname, COLORREF thecolor);
  Topology() {};
  
  ~Topology();
  
  void Open();
  void Close();

  bool append;

  void updateCache(rectObj thebounds, bool purgeonly=false);
  void Paint(HDC hdc, RECT rc);

  double scaleThreshold;

  bool CheckScale();
  void TriggerIfScaleNowVisible();

  bool triggerUpdateCache;
  int shapes_visible_count;

  XShape** shpCache;

  bool checkVisible(shapeObj* shape, rectObj *screenRect);

  void loadBitmap(int);

  char* filename;

  virtual void removeShape(int i);
  virtual XShape* addShape(int i);
  
 protected:

  void flushCache();

  bool in_scale;
  int getQuad(double x, double y, RECT rc);
  bool checkInside(int x, int y, int quad, RECT rc);
  int getCorner(int n, int n2);
  HPEN hPen;
  HBRUSH hbBrush;
  HBITMAP hBitmap;
  shapefileObj shpfile;
  bool shapefileopen;
 
};


class TopologyWriter: public Topology {
 public:
  TopologyWriter(char *shpname, COLORREF thecolor);
  ~TopologyWriter();
  
  void addPoint(double x, double y);
};



class TopologyLabel: public Topology {
 public:
  TopologyLabel(char* shpname, COLORREF thecolor, INT field1);
  ~TopologyLabel();
  virtual XShape* addShape(int i);
  void setField(int i);
  int field;

};


#endif
