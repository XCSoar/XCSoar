#include "Sizes.h"
#include "MapWindow.h"
#include "InfoBoxLayout.h"

extern HWND hWndInfoWindow[NUMINFOWINDOWS];
extern HWND hWndTitleWindow[NUMINFOWINDOWS];
extern HWND hWndMainWindow; // Main Windows
extern HINSTANCE hInst; // The current instance

// Layouts:
// 0: default, infoboxes along top and bottom, map in middle
// 1: both infoboxes along bottom
// 2: both infoboxes along top
// 3: infoboxes along both sides
// 4: infoboxes along left side
// 5: infoboxes along right side


int InfoBoxLayout::InfoBoxGeometry = 1;
int InfoBoxLayout::ControlWidth;
int InfoBoxLayout::ControlHeight;
int InfoBoxLayout::TitleHeight;


void InfoBoxLayout::GetInfoBoxPosition(int i, RECT rc, int *x, int *y) {
  switch (InfoBoxGeometry) {
  case 0:
    if (i<NUMINFOWINDOWS/2) {
      *x = i*ControlWidth;
      *y = rc.top;
    } else {
      *x = (i-NUMINFOWINDOWS/2)*ControlWidth;
      *y = rc.bottom-ControlHeight;
    }
    break;
  case 1:
    if (i<NUMINFOWINDOWS/2) {
      *x = i*ControlWidth;
      *y = rc.bottom-ControlHeight*2;
    } else {
      *x = (i-NUMINFOWINDOWS/2)*ControlWidth;
      *y = rc.bottom-ControlHeight;
    }
    break;
  case 2:
    if (i<NUMINFOWINDOWS/2) {
      *x = i*ControlWidth;
      *y = rc.top;;
    } else {
      *x = (i-NUMINFOWINDOWS/2)*ControlWidth;
      *y = rc.top+ControlHeight;
    }
    break;

  case 3:
    if (i<NUMINFOWINDOWS/2) {
      *x = rc.left;
      *y = rc.top+ControlHeight*i;
    } else {
      *x = rc.right-ControlWidth;
      *y = rc.top+ControlHeight*(i-NUMINFOWINDOWS/2);
    }
    break;
  case 4:
    if (i<NUMINFOWINDOWS/2) {
      *x = rc.left;
      *y = rc.top+ControlHeight*i;
    } else {
      *x = rc.left+ControlWidth;
      *y = rc.top+ControlHeight*(i-NUMINFOWINDOWS/2);
    }
    break;
  case 5:
    if (i<NUMINFOWINDOWS/2) {
      *x = rc.right-ControlWidth*2;
      *y = rc.top+ControlHeight*i;
    } else {
      *x = rc.right-ControlWidth;
      *y = rc.top+ControlHeight*(i-NUMINFOWINDOWS/2);
    }
    break;
  };
}


void InfoBoxLayout::GetInfoBoxSizes(RECT rc) {

  if (rc.bottom<rc.right) {
    // landscape mode
    if (InfoBoxGeometry<3) {
      InfoBoxGeometry+= 3;
    }

  } else {
    // portrait mode
    if (InfoBoxGeometry>=3) {
      InfoBoxGeometry-= 3;
    }
  }

  switch (InfoBoxGeometry) {
  case 0:
    // calculate control dimensions
    
    ControlWidth = 2*(rc.right - rc.left) / NUMINFOWINDOWS;
    ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 
    
    // calculate small map screen size
    
    MapWindow::MapRect.top = rc.top+ControlHeight;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom-ControlHeight;
    MapWindow::MapRect.right = rc.right;
    break;

  case 1:
    // calculate control dimensions
    
    ControlWidth = 2*(rc.right - rc.left) / NUMINFOWINDOWS;
    ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 
    
    // calculate small map screen size
    
    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom-ControlHeight*2;
    MapWindow::MapRect.right = rc.right;
    break;

  case 2:
    // calculate control dimensions
    
    ControlWidth = 2*(rc.right - rc.left) / NUMINFOWINDOWS;
    ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 
    
    // calculate small map screen size
    
    MapWindow::MapRect.top = rc.top+ControlHeight*2;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right;
    break;

  case 3:
    // calculate control dimensions
    
    ControlWidth = (rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3;
    ControlHeight = (int)(2*(rc.bottom - rc.top) / NUMINFOWINDOWS);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 
    
    // calculate small map screen size
    
    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left+ControlWidth;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth;
    break;

  case 4:
    // calculate control dimensions
    
    ControlWidth = (rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3;
    ControlHeight = (int)(2*(rc.bottom - rc.top) / NUMINFOWINDOWS);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 
    
    // calculate small map screen size
    
    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left+ControlWidth*2;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right;
    break;

  case 5:
    // calculate control dimensions
    
    ControlWidth = (rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3;
    ControlHeight = (int)(2*(rc.bottom - rc.top) / NUMINFOWINDOWS);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 
    
    // calculate small map screen size
    
    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth*2;
    break;

  };

}


void InfoBoxLayout::CreateInfoBoxes(RECT rc) {
  int i;
  int xoff, yoff;

  GetInfoBoxSizes(rc);

  // create infobox windows

  for(i=0;i<NUMINFOWINDOWS;i++)
    {
      GetInfoBoxPosition(i, rc, &xoff, &yoff);

      hWndInfoWindow[i] = 
	CreateWindow(TEXT("STATIC"),TEXT("\0"),
		     WS_VISIBLE|WS_CHILD|WS_TABSTOP
		     |SS_CENTER|SS_NOTIFY
		     |WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     xoff, yoff+TitleHeight,
		     ControlWidth,ControlHeight-TitleHeight,
		     hWndMainWindow,NULL,hInst,NULL);

      hWndTitleWindow[i] = 
	CreateWindow(TEXT("STATIC"), TEXT("\0"),
		     WS_VISIBLE|WS_CHILD|WS_TABSTOP
		     |SS_CENTER|SS_NOTIFY 
		     |WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     xoff, yoff, 
		     ControlWidth, TitleHeight,
		     hWndMainWindow,NULL,hInst,NULL);
      
    }

}
