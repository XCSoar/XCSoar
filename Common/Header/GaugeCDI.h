#ifndef GAUGE_CDI_H
#define GAUGE_CDI_H
#include "StdAfx.h"

class GaugeCDI {
 public:
  static void Create();
  static void Destroy();

  static void Show();
  static void Hide();

  static void Update(double TrackBearing, double WaypointBearing);
};

#endif
