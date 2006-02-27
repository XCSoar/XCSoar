/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

}
*/

#include "stdafx.h"
#include <limits.h>

#include "WindowControls.h"
#include "dlgTools.h"
#include "XMLParser.h"
#include "InfoBoxLayout.h"


extern HWND   hWndMainWindow;

extern HFONT  TitleWindowFont;
extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;
extern HFONT  CDIWindowFont;
extern HFONT  InfoWindowFont;

/*
extern "C" {
int
WINAPI
MessageBoxW(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType);
}
*/


// Message Box Replacement
/*
    MessageBox(hWndMapWindow,
      gettext(TEXT("Too many waypoints in task!")),
      gettext(TEXT("Insert Waypoint")),
      MB_OK|MB_ICONEXCLAMATION);
*/

static void OnButtonClick(WindowControl * Sender){
  ((WndForm *)Sender->GetOwner()->GetOwner())->SetModalResult(Sender->GetTag());
}

int WINAPI MessageBoxX(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType){

  WndForm *wf=NULL;
  WndFrame *wText=NULL;
  int X, Y, Width, Height;
  WndButton *wButtons[10];
  int ButtonCount = 0;
  int i,x,y,d,w,h,res,dY;
  RECT rc;

  // todo
  hWnd = hWndMainWindow;

  ASSERT(hWnd == hWndMainWindow);
  ASSERT(lpText != NULL);
  ASSERT(lpCaption != NULL);

  GetClientRect(hWnd, &rc);

  Width = 200;
  Height = 150;

  X = ((rc.right-rc.left) - Width)/2;
  Y = ((rc.bottom-rc.top) - Height)/2;

  y = 100;
  w = 60;
  h = 32;

  wf = new WndForm(hWnd, TEXT("frmXcSoarMessageDlg"), (TCHAR*)lpCaption, X, Y, Width, Height);
  wf->SetFont(MapWindowBoldFont);
  wf->SetTitleFont(MapWindowBoldFont);
  wf->SetBackColor(RGB(0xDA, 0xDB, 0xAB));

  wText = new WndFrame(wf, TEXT("frmMessageDlgText"), 0, 5, Width, Height);
  wText->SetCaption((TCHAR*)lpText);
  wText->SetFont(MapWindowBoldFont);
  wText->SetCaptionStyle(
        DT_EXPANDTABS
      | DT_CENTER
      | DT_NOCLIP
      | DT_WORDBREAK
      | DT_VCENTER
  );

  /* TODO, dont work
  dY = wText->GetLastDrawTextHeight() - Height;
  */
  dY = -40;
  // wText->SetHeight(wText->GetLastDrawTextHeight()+5);
  wf->SetHeight(wf->GetHeight() + dY);

  y += dY;

  uType = uType & 0x000f;

  if (uType == MB_OK
      || uType == MB_OKCANCEL

  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("OK"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDOK);
    ButtonCount++;
  }

  if (uType == MB_YESNO
      || uType == MB_YESNOCANCEL
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("Yes"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDYES);
    ButtonCount++;
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("No"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDNO);
    ButtonCount++;
  }

  if (uType == MB_ABORTRETRYIGNORE
      || uType == MB_RETRYCANCEL
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("Retry"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDRETRY);
    ButtonCount++;
  }

  if (uType == MB_OKCANCEL
      || uType == MB_RETRYCANCEL
      || uType == MB_YESNOCANCEL
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("Cancel"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDCANCEL);
    ButtonCount++;
  }

  if (uType == MB_ABORTRETRYIGNORE
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("Abort"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDABORT);
    ButtonCount++;
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), TEXT("Ignore"), 0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDIGNORE);
    ButtonCount++;
  }

  d = Width / (ButtonCount);
  x = d/2-w/2;

  for (i=0; i<ButtonCount; i++){
    wButtons[i]->SetLeft(x);
    x += d;
  }

  res = wf->ShowModal();

  delete wf;

  return(res);

}





long StringToIntDflt(const TCHAR *String, long Default){
  if (String == NULL || String[0] == '\0')
    return(Default);
  return(_tcstol(String, NULL, 0));        
}

double StringToFloatDflt(const TCHAR *String, double Default){
  if (String == NULL || String[0] == '\0')
    return(Default);
  return(_tcstod(String, NULL));
}

const TCHAR *StringToStringDflt(const TCHAR *String, TCHAR *Default){
  if (String == NULL || String[0] == '\0')
    return(Default);
  return(String);
}

void GetDefaultWindowControlProps(XMLNode *Node, TCHAR *Name, int *X, int *Y, int *Width, int *Height, int *Font, TCHAR *Caption){

  *X = StringToIntDflt(Node->getAttribute(TEXT("X")), 0)
    *InfoBoxLayout::scale;
  *Y = StringToIntDflt(Node->getAttribute(TEXT("Y")), 0);
  if (*Y>=0) {
    (*Y) *= InfoBoxLayout::scale;
  }
  *Width = StringToIntDflt(Node->getAttribute(TEXT("Width")), 50)
    *InfoBoxLayout::scale;
  *Height = StringToIntDflt(Node->getAttribute(TEXT("Height")), 50)
    *InfoBoxLayout::scale;
  *Font = StringToIntDflt(Node->getAttribute(TEXT("Font")), -1);
  _tcscpy(Name, StringToStringDflt(Node->getAttribute(TEXT("Name")), TEXT("")));
  _tcscpy(Caption, StringToStringDflt(Node->getAttribute(TEXT("Caption")), TEXT("")));

}

void *CallBackLookup(CallBackTableEntry_t *LookUpTable, TCHAR *Name){

  int i;

  if (LookUpTable!=NULL && Name!=NULL && Name[0]!= '\0')
    for (i=0; LookUpTable[i].Ptr != NULL; i++){
      if (_tcscmp(LookUpTable[i].Name, Name) == 0){
        return(LookUpTable[i].Ptr);
      }
    }

  return(NULL);

}

void LoadChildsFromXML(WindowControl *Parent, CallBackTableEntry_t *LookUpTable, XMLNode *Node, int Font);

static HFONT FontMap[5] = {
    TitleWindowFont,
    MapWindowFont,
    MapWindowBoldFont,
    CDIWindowFont,
    InfoWindowFont
  };


#include <stdio.h>

WndForm *dlgLoadFromXML(CallBackTableEntry_t *LookUpTable, char *FileName, HWND Parent){

  WndForm *theForm = NULL;
  //  TCHAR sFileName[128];

  ASSERT(hWndMainWindow == Parent);

// T:\\Project\\WINCE\\TNAV\\XCSoar\\

  // this open and parse the XML file:

#if (WINDOWSPC<1)
  XMLNode xMainNode=XMLNode::openFileHelper(FileName ,TEXT("PMML"));
#else
  char winname[200];
  sprintf(winname,"C:\\XCSoar%s",FileName);
  XMLNode xMainNode=XMLNode::openFileHelper(winname ,TEXT("PMML"));
#endif

  // JMW TODO: put in error checking here and get rid of exits in xmlParser
  if (xMainNode.isEmpty()) {
    return NULL;
  }

  XMLNode xNode=xMainNode.getChildNode(TEXT("WndForm"));

  FontMap[0] = TitleWindowFont;
  FontMap[1] = MapWindowFont;
  FontMap[2] = MapWindowBoldFont;
  FontMap[3] = CDIWindowFont;
  FontMap[4] = InfoWindowFont;

  if (!xNode.isEmpty()){
    int X,Y,Width,Height,Font;
    TCHAR sTmp[128];
    TCHAR Name[64];

    COLORREF BackColor;
    COLORREF ForeColor;

    GetDefaultWindowControlProps(&xNode, Name, &X, &Y, &Width, &Height, 
				 &Font, sTmp);
    BackColor = StringToIntDflt(xNode.getAttribute(TEXT("BackColor")), 
				0xffffffff);
    ForeColor = StringToIntDflt(xNode.getAttribute(TEXT("ForeColor")), 
				0xffffffff);

    theForm = new WndForm(Parent, Name, sTmp, X, Y, Width, Height);

    if (Font != -1)
      theForm->SetTitleFont(FontMap[Font]);

    if (Font != -1)
      theForm->SetFont(FontMap[Font]);
    if (BackColor != 0xffffffff){
      BackColor = RGB((BackColor>>16)&0xff,
		      (BackColor>>8)&0xff,
		      (BackColor>>0)&0xff);
      theForm->SetBackColor(BackColor);
    }
    if (ForeColor != 0xffffffff){
      ForeColor = RGB((ForeColor>>16)&0xff,
		      (ForeColor>>8)&0xff,
		      (ForeColor>>0)&0xff);
      theForm->SetForeColor(ForeColor);
    }

    LoadChildsFromXML(theForm, LookUpTable, &xNode, Font);

    if (XMLNode::GlobalError) {
      delete theForm;
      return NULL;
    }

  } else {
    return NULL;
  }

  return(theForm);

}



void LoadChildsFromXML(WindowControl *Parent, 
		       CallBackTableEntry_t *LookUpTable, 
		       XMLNode *Node, 
		       int ParentFont) {

  int X,Y,Width,Height,Font;
  TCHAR Caption[128];
  TCHAR Name[64];
  COLORREF BackColor;
  COLORREF ForeColor;
  bool Visible;

  int Count = Node->nChildNode();

  for (int i=0; i<Count; i++){

    WindowControl *WC=NULL;

    XMLNode childNode = Node->getChildNode(i);

    GetDefaultWindowControlProps(&childNode, 
				 Name, 
				 &X, &Y, 
				 &Width, &Height, 
				 &Font, Caption);

    BackColor = StringToIntDflt(childNode.getAttribute(TEXT("BackColor")), 
				0xffffffff);
    ForeColor = StringToIntDflt(childNode.getAttribute(TEXT("ForeColor")), 
				0xffffffff);
    Visible = StringToIntDflt(childNode.getAttribute(TEXT("Visible")), 1) == 1;
    if (BackColor != 0xffffffff){
      BackColor = RGB((BackColor>>16)&0xff,
		      (BackColor>>8)&0xff,
		      (BackColor>>0)&0xff);
    }
    if (ForeColor != 0xffffffff){
      ForeColor = RGB((ForeColor>>16)&0xff,
		      (ForeColor>>8)&0xff,
		      (ForeColor>>0)&0xff);
    }
    Font = StringToIntDflt(childNode.getAttribute(TEXT("Font")), ParentFont);

    if (_tcscmp(childNode.getName(), TEXT("WndProperty")) == 0){

      WndProperty *W;
      int CaptionWidth;
      TCHAR DataNotifyCallback[128];
      int ReadOnly;
      int MultiLine;

      CaptionWidth = 
	StringToIntDflt(childNode.getAttribute(TEXT("CaptionWidth")), 
			0)*InfoBoxLayout::scale;
      MultiLine = 
	StringToIntDflt(childNode.getAttribute(TEXT("MultiLine")), 
			0);
      ReadOnly = 
	StringToIntDflt(childNode.getAttribute(TEXT("ReadOnly")), 
			0);

      _tcscpy(DataNotifyCallback, 
	      StringToStringDflt(childNode.getAttribute(TEXT("OnDataNotify")),
				 TEXT("")));
      _tcscpy(Caption, 
	      StringToStringDflt(childNode.getAttribute(TEXT("Caption")), 
				 TEXT("")));

      WC = W = 
	new WndProperty(Parent, Name, Caption, X, Y, 
			Width, Height, CaptionWidth,
			(WndProperty::DataChangeCallback_t) 
			CallBackLookup(LookUpTable, DataNotifyCallback), 
			MultiLine);

      Caption[0] = '\0';
      W->SetReadOnly(ReadOnly != 0);

      if (childNode.nChildNode(TEXT("DataField")) > 0){

        TCHAR DataType[32];
        TCHAR DisplayFmt[32];
        TCHAR EditFormat[32];
        TCHAR OnDataAccess[64];
        double Min, Max, Step;

        XMLNode dataFieldNode = 
	  childNode.getChildNode(TEXT("DataField"), 0);

        _tcscpy(DataType, 
		StringToStringDflt(dataFieldNode.
				   getAttribute(TEXT("DataType")), 
				   TEXT("")));
        _tcscpy(DisplayFmt, 
		StringToStringDflt(dataFieldNode.
				   getAttribute(TEXT("DisplayFormat")), 
				   TEXT("")));
        _tcscpy(EditFormat, 
		StringToStringDflt(dataFieldNode.
				   getAttribute(TEXT("EditFormat")), 
				   TEXT("")));
        _tcscpy(OnDataAccess, 
		StringToStringDflt(dataFieldNode.
				   getAttribute(TEXT("OnDataAccess")), 
				   TEXT("")));
        ReadOnly = StringToIntDflt(dataFieldNode.
				   getAttribute(TEXT("ReadOnly")), 0);
        Min = StringToIntDflt(dataFieldNode.
			      getAttribute(TEXT("Min")), INT_MIN);
        Max = StringToIntDflt(dataFieldNode.
			      getAttribute(TEXT("Max")), INT_MAX);
        Step = StringToFloatDflt(dataFieldNode.
				 getAttribute(TEXT("Step")), 1);

        if (_tcsicmp(DataType, TEXT("enum"))==0){
          W->SetDataField(
			  new DataFieldEnum(EditFormat, DisplayFmt, false,
					    (DataField::DataAccessCallback_t) 
					    CallBackLookup(LookUpTable, 
							   OnDataAccess))
          );
        }
        if (_tcsicmp(DataType, TEXT("filereader"))==0){
          W->SetDataField(
			  new DataFieldFileReader(EditFormat, 
						  DisplayFmt,
						  (DataField::DataAccessCallback_t) 
						  CallBackLookup(LookUpTable, OnDataAccess))
          );
        }
        if (_tcsicmp(DataType, TEXT("boolean"))==0){
          W->SetDataField(
            new DataFieldBoolean(EditFormat, DisplayFmt, false, TEXT("ON"), TEXT("OFF"),
              (DataField::DataAccessCallback_t) CallBackLookup(LookUpTable, OnDataAccess))
          );
        }
        if (_tcsicmp(DataType, TEXT("double"))==0){
          W->SetDataField(
            new DataFieldFloat(EditFormat, DisplayFmt, Min, Max, 0, Step,
              (DataField::DataAccessCallback_t) CallBackLookup(LookUpTable, OnDataAccess))
          );
        }
        if (_tcsicmp(DataType, TEXT("integer"))==0){
          W->SetDataField(
			  new DataFieldInteger(EditFormat, DisplayFmt, (int)Min, (int)Max, (int)0, (int)Step,
              (DataField::DataAccessCallback_t) CallBackLookup(LookUpTable, OnDataAccess))
          );
        }
        if (_tcsicmp(DataType, TEXT("string"))==0){
          W->SetDataField(
            new DataFieldString(EditFormat, DisplayFmt, TEXT(""),
              (DataField::DataAccessCallback_t) CallBackLookup(LookUpTable, OnDataAccess))
          );
        }

      }

    }else

    if (_tcscmp(childNode.getName(), TEXT("WndButton")) == 0){

      TCHAR ClickCallback[128];

       _tcscpy(ClickCallback, StringToStringDflt(childNode.getAttribute(TEXT("OnClickNotify")), TEXT("")));

      WC = new WndButton(Parent, Name, Caption, X, Y, Width, Height,
               (WndButton::ClickNotifyCallback_t) CallBackLookup(LookUpTable, ClickCallback));

      Caption[0] = '\0';

    }else


    if (_tcscmp(childNode.getName(), TEXT("WndOwnerDrawFrame")) == 0){

      TCHAR PaintCallback[128];

      _tcscpy(PaintCallback, StringToStringDflt(childNode.getAttribute(TEXT("OnPaint")), TEXT("")));

      WC = new WndOwnerDrawFrame(Parent, Name, X, Y, Width, Height,
               (WndOwnerDrawFrame::OnPaintCallback_t) CallBackLookup(LookUpTable, PaintCallback));

    }else

    if (_tcscmp(childNode.getName(), TEXT("WndFrame")) == 0){

      WndFrame *W;

      WC = W = new WndFrame(Parent, Name, X, Y, Width, Height);

      LoadChildsFromXML(W, LookUpTable, &childNode, ParentFont);  // recursivly create dialog

    }else

    if (_tcscmp(childNode.getName(), TEXT("WndListFrame")) == 0){

      TCHAR ListCallback[128];

      _tcscpy(ListCallback, StringToStringDflt(childNode.getAttribute(TEXT("OnListInfo")), TEXT("")));

      WC = new WndListFrame(Parent, Name, X, Y, Width, Height,
               (WndListFrame::OnListCallback_t) CallBackLookup(LookUpTable, ListCallback));

      LoadChildsFromXML(WC, LookUpTable, &childNode, ParentFont);  // recursivly create dialog

    }

    if (WC != NULL){

      if (Font != -1)
        WC->SetFont(FontMap[Font]);

      if (BackColor != 0xffffffff){
        WC->SetBackColor(BackColor);
      }

      if (ForeColor != 0xffffffff){
        WC->SetForeColor(ForeColor);
      }

      if (!Visible){
        WC->SetVisible(Visible);
      }

      if (Caption[0] != '\0'){
        WC->SetCaption(Caption);
      }
    }

  }

}



