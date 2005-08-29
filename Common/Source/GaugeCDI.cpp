#include "GaugeCDI.h"

HWND hWndCDIWindow = NULL; //CDI Window
extern HWND hWndMainWindow; // Main Windows
extern HWND hWndMenuButton;
extern HINSTANCE hInst;      // The current instance

#include "InfoBoxLayout.h"

extern HFONT CDIWindowFont; // New

void GaugeCDI::Create() {
  // start of new code for displaying CDI window
  RECT rc;
  GetClientRect(hWndMainWindow, &rc);

  hWndCDIWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN
			       | WS_CLIPSIBLINGS,
                               0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);

  SendMessage(hWndCDIWindow,WM_SETFONT,
              (WPARAM)CDIWindowFont,MAKELPARAM(TRUE,0));
  SetWindowPos(hWndCDIWindow,hWndMenuButton,
               (int)(InfoBoxLayout::ControlWidth*0.6),
	       (int)(InfoBoxLayout::ControlHeight+1),
               (int)(InfoBoxLayout::ControlWidth*2.8),
	       (int)(InfoBoxLayout::TitleHeight*1.4),
	       SWP_SHOWWINDOW);

  // end of new code for drawing CDI window (see below for destruction of objects)

  ShowWindow(hWndCDIWindow, SW_HIDE);

}


void GaugeCDI::Destroy() {
  DestroyWindow(hWndCDIWindow);
}
