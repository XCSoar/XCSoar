#include "Sizes.h"
#include "MapWindow.h"
#include "InfoBoxLayout.h"
#include "Dialogs.h"
#include "Utils.h"

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


int InfoBoxLayout::InfoBoxGeometry = 0;
int InfoBoxLayout::ControlWidth;
int InfoBoxLayout::ControlHeight;
int InfoBoxLayout::TitleHeight;


void InfoBoxLayout::GetInfoBoxPosition(int i, RECT rc,
				       int *x, int *y,
				       int *sizex, int *sizey) {
  TCHAR reggeompx[50];
  TCHAR reggeompy[50];
  TCHAR reggeomsx[50];
  TCHAR reggeomsy[50];
  DWORD Temp=0;

  wsprintf(reggeompx, TEXT("InfoBoxPositionPosX%d"), i);
  wsprintf(reggeompy, TEXT("InfoBoxPositionPosY%d"), i);
  wsprintf(reggeomsx, TEXT("InfoBoxPositionSizeX%d"), i);
  wsprintf(reggeomsy, TEXT("InfoBoxPositionSizeY%d"), i);

  GetFromRegistry(reggeompx,&Temp); *x = Temp;
  GetFromRegistry(reggeompy,&Temp); *y = Temp;
  GetFromRegistry(reggeomsx,&Temp); *sizex = Temp;
  GetFromRegistry(reggeomsy,&Temp); *sizey = Temp;

  if ((*sizex==0)||(*sizey==0)) {
    // not defined in registry so go with defaults
    // these will be saved back to registry

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

    *sizex = ControlWidth;
    *sizey = ControlHeight;

    SetToRegistry(reggeompx,*x);
    SetToRegistry(reggeompy,*y);
    SetToRegistry(reggeomsx,*sizex);
    SetToRegistry(reggeomsy,*sizey);

  };
}


void InfoBoxLayout::GetInfoBoxSizes(RECT rc) {

  TCHAR szRegistryInfoBoxGeometry[]=  TEXT("InfoBoxGeometry");
  DWORD Temp;
  GetFromRegistry(szRegistryInfoBoxGeometry,&Temp);
  InfoBoxGeometry = Temp;

  if (rc.bottom<rc.right) {
    // landscape mode
    if (InfoBoxGeometry<4) {
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

    ControlWidth = (int)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
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

    ControlWidth = (int)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
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

    ControlWidth = (int)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
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
  int xoff, yoff, sizex, sizey;

  GetInfoBoxSizes(rc);

  // create infobox windows

  for(i=0;i<NUMINFOWINDOWS;i++)
    {
      GetInfoBoxPosition(i, rc, &xoff, &yoff, &sizex, &sizey);

      hWndInfoWindow[i] =
	CreateWindow(TEXT("STATIC"),TEXT("\0"),
		     WS_VISIBLE|WS_CHILD|WS_TABSTOP
		     |SS_CENTER|SS_NOTIFY
		     |WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     xoff, yoff+TitleHeight,
		     sizex, sizey-TitleHeight,
		     hWndMainWindow,NULL,hInst,NULL);

      hWndTitleWindow[i] =
	CreateWindow(TEXT("STATIC"), TEXT("\0"),
		     WS_VISIBLE|WS_CHILD|WS_TABSTOP
		     |SS_CENTER|SS_NOTIFY
		     |WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     xoff, yoff,
		     sizex, TitleHeight,
		     hWndMainWindow,NULL,hInst,NULL);

    }

}


///////////////////////

HWND ButtonLabel::hWndButtonWindow[NUMBUTTONLABELS];

int ButtonLabel::ButtonLabelGeometry = 0; // unused currently


void ButtonLabel::GetButtonPosition(int i, RECT rc,
				    int *x, int *y,
				    int *sizex, int *sizey) {

  TCHAR reggeompx[50];
  TCHAR reggeompy[50];
  TCHAR reggeomsx[50];
  TCHAR reggeomsy[50];
  DWORD Temp=0;

  wsprintf(reggeompx, TEXT("ScreenButtonPosX%d"), i);
  wsprintf(reggeompy, TEXT("ScreenButtonPosY%d"), i);
  wsprintf(reggeomsx, TEXT("ScreenButtonSizeX%d"), i);
  wsprintf(reggeomsy, TEXT("ScreenButtonSizeY%d"), i);

  GetFromRegistry(reggeompx,&Temp); *x = Temp;
  GetFromRegistry(reggeompy,&Temp); *y = Temp;
  GetFromRegistry(reggeomsx,&Temp); *sizex = Temp;
  GetFromRegistry(reggeomsy,&Temp); *sizey = Temp;

  if ((*sizex==0)||(*sizey==0)) {
    // not defined in registry so go with defaults
    // these will be saved back to registry
    int hwidth = (rc.right-rc.left)/4;

    if (i==0) {
      *x = rc.left+3;
      *y = (rc.bottom-20-InfoBoxLayout::ControlHeight-20);
      *sizex = 52;
      *sizey = 20;
    } else {
      *x = rc.left+3+hwidth*(i-1);
      *y = (rc.bottom-20-InfoBoxLayout::ControlHeight);
      *sizex = 52;
      *sizey = 20;
    }

    SetToRegistry(reggeompx,*x);
    SetToRegistry(reggeompy,*y);
    SetToRegistry(reggeomsx,*sizex);
    SetToRegistry(reggeomsy,*sizey);

  };

}


void ButtonLabel::CreateButtonLabels(RECT rc) {
  int i;
  int x, y, xsize, ysize;

  for (i=0; i<NUMBUTTONLABELS; i++) {
    hWndButtonWindow[i] =
      CreateWindow(TEXT("STATIC"), TEXT("\0"),
		   WS_VISIBLE|WS_CHILD|WS_TABSTOP
		   |SS_CENTER|SS_NOTIFY
		   |WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		   rc.left, rc.top,
		   50, 15,
		   hWndMainWindow, NULL, hInst, NULL);
    GetButtonPosition(i, rc, &x, &y, &xsize, &ysize);

    SetWindowPos(hWndButtonWindow[i],HWND_TOP,
		 x, y,
		 xsize, ysize, SWP_SHOWWINDOW);

    SetLabelText(i,NULL);
    SetWindowLong(hWndButtonWindow[i], GWL_USERDATA, 4);
  }
}


void ButtonLabel::Destroy() {
  int i;
  for (i=0; i<NUMBUTTONLABELS; i++) {
    DestroyWindow(hWndButtonWindow[i]);
  }
}


void ButtonLabel::SetLabelText(int index, TCHAR *text) {
  if (index>= NUMBUTTONLABELS) {
    // error!
    return;
  }
  if ((text==NULL)||(*text==_T('\0'))) {
    ShowWindow(hWndButtonWindow[index], SW_HIDE);
  } else {
    SetWindowText(hWndButtonWindow[index], gettext(text));
    ShowWindow(hWndButtonWindow[index], SW_SHOW);
  }

}

#include "InputEvents.h"

void ButtonLabel::CheckButtonPress(HWND pressedwindow) {
  int i;
  for (i=0; i<NUMBUTTONLABELS; i++) {
    if (hWndButtonWindow[i]== pressedwindow) {
      InputEvents::processButton(i);
    }
  }
}
