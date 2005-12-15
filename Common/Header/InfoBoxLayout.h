#ifndef INFOBOXLAYOUT_H
#define INFOBOXLAYOUT_H

#include "stdafx.h"
#include "Sizes.h"

class InfoBoxLayout {
 public:
  static bool landscape;
  static bool square;
  static void CreateInfoBoxes(RECT rc);
  #if NEWINFOBOX > 0
  static void DestroyInfoBoxes(void);
  #endif
  static int InfoBoxGeometry;
  static int ControlWidth, ControlHeight, TitleHeight;
  static void ScreenGeometry(RECT rc);
 private:
  static void GetInfoBoxPosition(int i, RECT rc, 
				 int *x, int *y,
				 int *sizex, int *sizey);
  static void GetInfoBoxSizes(RECT rc);
};


class ButtonLabel {
 public:
  static int ButtonLabelGeometry;
  static HWND hWndButtonWindow[NUMBUTTONLABELS];
  static void CreateButtonLabels(RECT rc);
  #if NEWINFOBOX > 0
  static void SetFont(HFONT Font);
  #endif
  static void Destroy();
  static void SetLabelText(int index, TCHAR *text);
  static bool CheckButtonPress(HWND pressedwindow);
  static void GetButtonPosition(int i, RECT rc, 
				int *x, int *y,
				int *sizex, int *sizey);

};

#endif
