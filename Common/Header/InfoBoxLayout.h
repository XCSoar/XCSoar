#ifndef INFOBOXLAYOUT_H
#define INFOBOXLAYOUT_H

#include "stdafx.h"
#include "Sizes.h"

class InfoBoxLayout {
 public:
  static void CreateInfoBoxes(RECT rc);
  static int InfoBoxGeometry;
  static int ControlWidth, ControlHeight, TitleHeight;
 private:
  static void GetInfoBoxPosition(int i, RECT rc, int *x, int *y);
  static void GetInfoBoxSizes(RECT rc);
};


class ButtonLabel {
 public:
  static int ButtonLabelGeometry;
  static HWND hWndButtonWindow[NUMBUTTONLABELS];
  static void CreateButtonLabels(RECT rc);
  static void Destroy();
  static void SetLabelText(int index, TCHAR *text);
  static void CheckButtonPress(HWND pressedwindow);

};

#endif
