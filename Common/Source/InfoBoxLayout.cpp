#include "Sizes.h"
#include "MapWindow.h"
#include "InfoBoxLayout.h"
#include "Dialogs.h"
#include "Utils.h"
#include "externs.h"

extern HWND hWndInfoWindow[MAXINFOWINDOWS];
extern HWND hWndTitleWindow[MAXINFOWINDOWS];
extern HWND hWndMainWindow; // Main Windows
extern HINSTANCE hInst; // The current instance

// Layouts:
// 0: default, infoboxes along top and bottom, map in middle
// 1: both infoboxes along bottom
// 2: both infoboxes along top
// 3: infoboxes along both sides
// 4: infoboxes along left side
// 5: infoboxes along right side
// 6: infoboxes GNAV
/*

Screen
640x480 landscape

480/6 = 80 control height

2/3 of width is map = 420
leaving 220 = 110 control width

*/


/*

Button 0 (x,y,sx,sy)
Button 1 (x,y,sx,sy)
...

InfoBox 0 (x,y,sx,sy)

*/


int InfoBoxLayout::InfoBoxGeometry = 0;
int InfoBoxLayout::ControlWidth;
int InfoBoxLayout::ControlHeight;
int InfoBoxLayout::TitleHeight;

bool gnav = false;

bool geometrychanged = false;



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

  if ((*sizex==0)||(*sizey==0)||geometrychanged) {
    // not defined in registry so go with defaults
    // these will be saved back to registry

    switch (InfoBoxGeometry) {
    case 0:
      if (i<numInfoWindows/2) {
	*x = i*ControlWidth;
	*y = rc.top;
      } else {
	*x = (i-numInfoWindows/2)*ControlWidth;
	*y = rc.bottom-ControlHeight;
      }
      break;
    case 1:
      if (i<numInfoWindows/2) {
	*x = i*ControlWidth;
	*y = rc.bottom-ControlHeight*2;
      } else {
	*x = (i-numInfoWindows/2)*ControlWidth;
	*y = rc.bottom-ControlHeight;
      }
      break;
    case 2:
      if (i<numInfoWindows/2) {
	*x = i*ControlWidth;
	*y = rc.top;;
      } else {
	*x = (i-numInfoWindows/2)*ControlWidth;
	*y = rc.top+ControlHeight;
      }
      break;

    case 3:
      if (i<numInfoWindows/2) {
	*x = rc.left;
	*y = rc.top+ControlHeight*i;
      } else {
	*x = rc.right-ControlWidth;
	*y = rc.top+ControlHeight*(i-numInfoWindows/2);
      }
      break;
    case 4:
      if (i<numInfoWindows/2) {
	*x = rc.left;
	*y = rc.top+ControlHeight*i;
      } else {
	*x = rc.left+ControlWidth;
	*y = rc.top+ControlHeight*(i-numInfoWindows/2);
      }
      break;
    case 5:
      if (i<numInfoWindows/2) {
	*x = rc.right-ControlWidth*2;
	*y = rc.top+ControlHeight*i;
      } else {
	*x = rc.right-ControlWidth;
	*y = rc.top+ControlHeight*(i-numInfoWindows/2);
      }
      break;
    case 6:
      if (i<3) {
	*x = rc.right-ControlWidth*2;
	*y = rc.top+ControlHeight*i;
      } else {
	if (i<6) {
	  *x = rc.right-ControlWidth*2;
	  *y = rc.top+ControlHeight*(i-3)+ControlHeight*3;
	} else {
	  *x = rc.right-ControlWidth;
	  *y = rc.top+ControlHeight*(i-6)+ControlHeight*3;
	}
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


void InfoBoxLayout::ScreenGeometry(RECT rc) {

  TCHAR szRegistryInfoBoxGeometry[]=  TEXT("InfoBoxGeometry");
  DWORD Temp;
  GetFromRegistry(szRegistryInfoBoxGeometry,&Temp);
  InfoBoxGeometry = Temp;

  if (rc.bottom<rc.right) {
    // landscape mode
    if (InfoBoxGeometry<4) {
      geometrychanged = true;

      // JMW testing
      if (1) {
	InfoBoxGeometry = 6;
      } else {
	InfoBoxGeometry+= 3;
      }
    }

  } else {
    // portrait mode
    if (InfoBoxGeometry>=3) {
      InfoBoxGeometry= 0;

      geometrychanged = true;
      gnav = false;
    }
  }

  SetToRegistry(szRegistryInfoBoxGeometry,InfoBoxGeometry);

  // JMW testing
  if (InfoBoxGeometry==6) {
    gnav = true;
  }
  if (gnav) {
    numInfoWindows = 9;
  } else {
    numInfoWindows = 8;
  }

}


void InfoBoxLayout::GetInfoBoxSizes(RECT rc) {

  switch (InfoBoxGeometry) {
  case 0:
    // calculate control dimensions

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
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

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
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

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
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
    ControlHeight = (int)(2*(rc.bottom - rc.top) / numInfoWindows);
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
    ControlHeight = (int)(2*(rc.bottom - rc.top) / numInfoWindows);
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
    ControlHeight = (int)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth*2;
    break;

  case 6:
    // calculate control dimensions

    ControlWidth = (int)((rc.right - rc.left)*0.18);
    ControlHeight = (int)((rc.bottom - rc.top)/6);
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

  for(i=0;i<numInfoWindows;i++)
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

  if ((*sizex==0)||(*sizey==0)||geometrychanged) {
    // not defined in registry so go with defaults
    // these will be saved back to registry
    int hwidth = (rc.right-rc.left)/4;
    int hheight = (rc.bottom-rc.top)/4;

    switch (ButtonLabelGeometry) {
    case 0:
      if (i==0) {
	*sizex = 52;
	*sizey = 20;
	*x = rc.left+3;
	*y = (rc.bottom-(*sizey)*2-InfoBoxLayout::ControlHeight);
      } else {
	*sizex = 52;
	*sizey = 20;
	*x = rc.left+3+hwidth*(i-1);
	*y = (rc.bottom-(*sizey)-InfoBoxLayout::ControlHeight);
      }
      break;

    case 1:
      hwidth = (rc.right-rc.left)/5;
      hheight = (rc.bottom-rc.top)/(4+1);

      if (i==0) {
	*sizex = 52;
	*sizey = 20;
	*x = rc.left+3+(*sizex);
	*y = (rc.top);
      } else {
	if (i<5) {
	  *sizex = 52;
	  *sizey = 20;
	  *x = rc.left+3;
	  *y = (rc.top+hheight*(i-1+1)-(*sizey)/2);
	} else {
	  *sizex = 52;
	  *sizey = 20;
	  *x = rc.left+3+hwidth*(i-5);
	  *y = (rc.bottom-(*sizey));
	}
      }
      break;

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

  int buttonWidth = 50;
  int buttonHeight = 15;

  if (gnav) {
    ButtonLabelGeometry = 1;
  } else {
    ButtonLabelGeometry = 0;
  }

  for (i=0; i<NUMBUTTONLABELS; i++) {
    hWndButtonWindow[i] =
      CreateWindow(TEXT("STATIC"), TEXT("\0"),
		   WS_VISIBLE|WS_CHILD|WS_TABSTOP
		   |SS_CENTER|SS_NOTIFY
		   |WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		   rc.left, rc.top,
		   buttonWidth, buttonHeight,
		   hWndMainWindow, NULL, hInst, NULL);
    GetButtonPosition(i, rc, &x, &y, &xsize, &ysize);

    SetWindowPos(hWndButtonWindow[i],HWND_TOP,
		 x, y,
		 xsize, ysize, SWP_SHOWWINDOW);

    SetLabelText(i,NULL);
    SetWindowLong(hWndButtonWindow[i], GWL_USERDATA, 4);
  }

  //

  if (gnav) {
    ButtonLabel::SetLabelText(5,TEXT("skey1"));
    ButtonLabel::SetLabelText(6,TEXT("skey2"));
    ButtonLabel::SetLabelText(7,TEXT("skey3"));
    ButtonLabel::SetLabelText(8,TEXT("skey4"));
    ButtonLabel::SetLabelText(9,TEXT("skey5"));
    EnableVarioGauge = true;
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
