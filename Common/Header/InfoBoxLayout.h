#ifndef INFOBOXLAYOUT_H
#define INFOBOXLAYOUT_H

#include "stdafx.h"
#include "Sizes.h"

class InfoBoxLayout {
 public:
  static bool fullscreen;
  static bool landscape;
  static bool square;
  static int scale;
  static void CreateInfoBoxes(RECT rc);
  static void DestroyInfoBoxes(void);
  static int InfoBoxGeometry;
  static int ControlWidth, ControlHeight, TitleHeight;
  static void ScreenGeometry(RECT rc);
  static void Paint(void);
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
  static bool ButtonVisible[NUMBUTTONLABELS];
  static bool ButtonDisabled[NUMBUTTONLABELS];
  static void CreateButtonLabels(RECT rc);
  static void AnimateButton(int i);
  static void SetFont(HFONT Font);
  static void Destroy();
  static void SetLabelText(int index, TCHAR *text);
  static bool CheckButtonPress(HWND pressedwindow);
  static void GetButtonPosition(int i, RECT rc, 
				int *x, int *y,
				int *sizex, int *sizey);
};

#endif
