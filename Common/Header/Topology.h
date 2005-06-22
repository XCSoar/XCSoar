#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "stdafx.h"
#include "mapshape.h"
#include "MapWindow.h"


class XShape {
 public:
  XShape();
  ~XShape();

  virtual void load(shapefileObj* shpfile, int i);
  virtual void clear();
  virtual void renderSpecial(HDC hdc, int x, int y) {};

  shapeObj shape;

};


class XShapeLabel: public XShape {
 public:
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

  void updateCache(rectObj thebounds);
  void Paint(HDC hdc, RECT rc);
  
  shapefileObj shpfile;
  bool shapefileopen;
  bool triggerUpdateCache;

  XShape** shpCache;

  bool checkVisible(shapeObj* shape, rectObj *screenRect);

  HPEN hPen;
  HBRUSH hbBrush;
  double scaleThreshold;
  HBITMAP hBitmap;
  
  void loadBitmap(int);

  char* filename;

  virtual void removeShape(int i);
  virtual XShape* addShape(int i);

};


class TopologyWriter: public Topology {
 public:
  TopologyWriter(char *shpname, COLORREF thecolor);

  void addPoint(double x, double y);
};



class TopologyLabel: public Topology {
 public:
  TopologyLabel(char* shpname, COLORREF thecolor);
    ~TopologyLabel();
  virtual XShape* addShape(int i);
  void setField(int i);
  int field;

};


#endif
