#ifndef INFOBOXLAYOUT_H
#define INFOBOXLAYOUT_H

#include "stdafx.h"

class InfoBoxLayout {
 public:
  static void CreateInfoBoxes(RECT rc);
  static int InfoBoxGeometry;
  static int ControlWidth, ControlHeight, TitleHeight;
 private:
  static void GetInfoBoxPosition(int i, RECT rc, int *x, int *y);
  static void GetInfoBoxSizes(RECT rc);
};

#endif
