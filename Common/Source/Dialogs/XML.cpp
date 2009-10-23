/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "xmlParser.h"
#include "LocalPath.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/FileReader.hpp"
#include "DataField/Float.hpp"
#include "DataField/Integer.hpp"
#include "DataField/String.hpp"
#include "UtilsSystem.hpp"
#include "Screen/Fonts.hpp"
#include "WindowControls.h"
#include "Interface.hpp"

#include <tchar.h>
#include <limits.h>

static inline int
mmin(const int t1, const int t2)
{
  return t1 < t2 ? t1 : t2;
}

static long
StringToIntDflt(const TCHAR *String, long Default)
{
  if (String == NULL || String[0] == '\0')
    return(Default);
  return(_tcstol(String, NULL, 0));
}

static double
StringToFloatDflt(const TCHAR *String, double Default)
{
  if (String == NULL || String[0] == '\0')
    return(Default);
  return(_tcstod(String, NULL));
}

static const TCHAR *
StringToStringDflt(const TCHAR *String, const TCHAR *Default)
{
  if (String == NULL || String[0] == '\0')
    return(Default);
  return(String);
}

static void
GetDefaultWindowControlProps(XMLNode *Node, TCHAR *Name, int *X, int *Y,
                             int *Width, int *Height, int *Font,
                             TCHAR *Caption)
{
  *X = DLGSCALE(StringToIntDflt(Node->getAttribute(TEXT("X")), 0));
  *Y = StringToIntDflt(Node->getAttribute(TEXT("Y")), 0);
  if (*Y>=0) { // not -1
    (*Y) = DLGSCALE(*Y);
  }
  *Width = DLGSCALE(StringToIntDflt(Node->getAttribute(TEXT("Width")), 50));
  *Height = StringToIntDflt(Node->getAttribute(TEXT("Height")), 50);
  if (*Height>=0) {
    (*Height) = DLGSCALE(*Height);
  }
  *Font = StringToIntDflt(Node->getAttribute(TEXT("Font")), -1);
  _tcscpy(Name, StringToStringDflt(Node->getAttribute(TEXT("Name")), TEXT("")));
  _tcscpy(Caption, StringToStringDflt(Node->getAttribute(TEXT("Caption")), TEXT("")));
  // TODO code: Temporary double handling to fix "const unsigned short
  // *" to "unsigned short *" problem
  _tcscpy(Caption,gettext(Caption));
}

static void *
CallBackLookup(CallBackTableEntry_t *LookUpTable, TCHAR *Name)
{
  int i;

  if (LookUpTable!=NULL && Name!=NULL && Name[0]!= '\0')
    for (i=0; LookUpTable[i].Ptr != NULL; i++){
      if (_tcscmp(LookUpTable[i].Name, Name) == 0){
        return(LookUpTable[i].Ptr);
      }
    }

  return(NULL);
}

static void
LoadChildsFromXML(WindowControl *Parent, CallBackTableEntry_t *LookUpTable,
                  XMLNode *Node, int Font);

static Font *FontMap[5] = {
  &TitleWindowFont,
  &MapWindowFont,
  &MapWindowBoldFont,
  &CDIWindowFont,
  &InfoWindowFont
};

#ifdef WIN32

static XMLNode
xmlLoadFromResource(const TCHAR* lpName, LPCTSTR tag, XMLResults *pResults)
{
  LPTSTR lpRes;
  HRSRC hResInfo;
  HGLOBAL hRes;
  int l, len;

  // Find the xml resource.
  hResInfo = FindResource (XCSoarInterface::hInst, lpName, TEXT("XMLDialog"));

  if (hResInfo == NULL) {
    MessageBoxX(
      gettext(TEXT("Can't find resource")),
      gettext(TEXT("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    // unable to find the resource
    return XMLNode::emptyXMLNode;
  }

  // Load the wave resource.
  hRes = LoadResource (XCSoarInterface::hInst, hResInfo);

  if (hRes == NULL) {
    MessageBoxX(
      gettext(TEXT("Can't load resource")),
      gettext(TEXT("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    // unable to load the resource
    return XMLNode::emptyXMLNode;
  }

  // Lock the wave resource and do something with it.
  lpRes = (LPTSTR)LockResource (hRes);

  if (lpRes) {
    l = SizeofResource(XCSoarInterface::hInst,hResInfo);
    if (l>0) {
      char *buf= (char*)malloc(l+2);
      if (!buf) {
        MessageBoxX(gettext(TEXT("Can't allocate memory")),
                    gettext(TEXT("Dialog error")),
                    MB_OK|MB_ICONEXCLAMATION);
        // unable to allocate memory
        return XMLNode::emptyXMLNode;
      }
      strncpy(buf,(char*)lpRes,l);
      buf[l]=0; // need to explicitly null-terminate.
      buf[l+1]=0;
      len = l;

#if defined(WIN32) || defined(UNDER_CE)
#ifdef _UNICODE
#if !defined(UNDER_CE) && !defined(WINDOWSPC)
      if (!IsTextUnicode(buf,mmin(l,10000),NULL))
        {
#endif
          LPTSTR b2=(LPTSTR)malloc(l*2+2);
          MultiByteToWideChar(CP_ACP,          // code page
                              MB_PRECOMPOSED,  // character-type options
                              buf,             // string to map
                              l,               // number of bytes in string
                              b2,              // wide-character buffer
                              l*2+2);          // size of buffer
          free(buf);
          buf=(char*)b2;
          buf[l*2]= 0;
          buf[l*2+1]= 0;
#if !defined(UNDER_CE) && !defined(WINDOWSPC)
        }
#endif
#else
      if (IsTextUnicode(buf,mmin(l,10000),NULL))
        {
          l>>=1;
          LPTSTR b2=(LPTSTR)malloc(l+2);
          WideCharToMultiByte(CP_ACP,                      // code page
                              0,                           // performance and mapping flags
                              (const WCHAR*)buf,           // wide-character string
                              l,                           // number of chars in string
                              b2,                          // buffer for new string
                              l+2,                         // size of buffer
                              NULL,                        // default for unmappable chars
                              NULL                         // set when default char used
                              );
          free(buf);
          buf=(char*)b2;
        }
#endif
#endif

      XMLNode x=XMLNode::parseString((LPTSTR)buf,tag,pResults);

      free(buf);
      return x;
    }
  }
  MessageBoxX(gettext(TEXT("Can't lock resource")),
              gettext(TEXT("Dialog error")),
              MB_OK|MB_ICONEXCLAMATION);
  return XMLNode::emptyXMLNode;
}



static XMLNode xmlOpenResourceHelper(const TCHAR *lpszXML, LPCTSTR tag)
{
    XMLResults pResults;

    pResults.error = eXMLErrorNone;
    XMLNode::GlobalError = false;
    XMLNode xnode=xmlLoadFromResource(lpszXML, tag, &pResults);
    if (pResults.error != eXMLErrorNone)
    {
      XMLNode::GlobalError = true;
      TCHAR errortext[100];
      _stprintf(errortext,TEXT("%s %i %i"), XMLNode::getError(pResults.error),
                pResults.nLine, pResults.nColumn);

      MessageBoxX(errortext,
                  gettext(TEXT("Dialog error")),
                  MB_OK|MB_ICONEXCLAMATION);
        // was exit(255);

    }
    return xnode;
}

#endif /* WIN32 */

static const XMLNode
load_xml_file_or_resource(const TCHAR *name, const TCHAR* resource)
{
  XMLNode xMainNode;

/* -> filename is allready localized
#ifndef WINDOWSPC
  xMainNode=XMLNode::openFileHelper(FileName ,TEXT("PMML"));
#else
  char winname[200];
  sprintf(winname,"C:\\XCSoar%s",FileName);
  xMainNode=XMLNode::openFileHelper(winname ,TEXT("PMML"));
#endif
*/

  char FileName[MAX_PATH];
  LocalPathS(FileName, name);

  if (FileExistsA(FileName))   //sgi use window API cals to check if
                               //file exists, this will supress
                               //CodeGurad warnings on callinf
                               //fopen(<unexisting file>)
    xMainNode=XMLNode::openFileHelper(FileName ,TEXT("PMML"));

#ifdef WIN32
  if (xMainNode.isEmpty()) {
    if (resource) {
      xMainNode =xmlOpenResourceHelper(resource,
                                       TEXT("PMML"));
    }
  }
#endif /* WIN32 */

  return xMainNode;
}

WndForm *dlgLoadFromXML(CallBackTableEntry_t *LookUpTable,
                        const TCHAR *FileName,
                        ContainerWindow &Parent,
                        const TCHAR* resource) {

  WndForm *theForm = NULL;
  //  TCHAR sFileName[128];

  // assert(main_window == Parent);  // Airspace warning has MapWindow as parent,
  // ist that ok?  JMW: No, I think that it is better to use main UI thread for
  // everything.  See changes regarding RequestAirspaceDialog in AirspaceWarning.cpp

  // this open and parse the XML file:

  XMLNode xMainNode = load_xml_file_or_resource(FileName, resource);

  // TODO code: put in error checking here and get rid of exits in xmlParser
  if (xMainNode.isEmpty()) {
    MessageBoxX(
      gettext(TEXT("Error in loading XML dialog")),
      gettext(TEXT("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    return NULL;
  }

  XMLNode xNode=xMainNode.getChildNode(TEXT("WndForm"));

  FontMap[0] = &TitleWindowFont;
  FontMap[1] = &MapWindowFont;
  FontMap[2] = &MapWindowBoldFont;
  FontMap[3] = &CDIWindowFont;
  FontMap[4] = &InfoWindowFont;

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

    theForm = new WndForm(&Parent, Name, sTmp, X, Y, Width, Height);

    if (Font != -1)
      theForm->SetTitleFont(*FontMap[Font]);

    if (Font != -1)
      theForm->SetFont(*FontMap[Font]);
#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    if (BackColor != 0xffffffff){
      BackColor = Color((BackColor>>16)&0xff,
                      (BackColor>>8)&0xff,
                      (BackColor>>0)&0xff);
      theForm->SetBackColor(BackColor);
    }
    if (ForeColor != 0xffffffff){
      ForeColor = Color((ForeColor>>16)&0xff,
                      (ForeColor>>8)&0xff,
                      (ForeColor>>0)&0xff);
      theForm->SetForeColor(ForeColor);
    }
#endif /* !ENABLE_SDL */

    LoadChildsFromXML(theForm, LookUpTable, &xNode, Font);

    if (XMLNode::GlobalError) {
      MessageBoxX(
                 gettext(TEXT("Error in loading XML dialog")),
                 gettext(TEXT("Dialog error")),
                 MB_OK|MB_ICONEXCLAMATION);

      delete theForm;
      return NULL;
    }
  } else {
    MessageBoxX(
      gettext(TEXT("Error in loading XML dialog")),
      gettext(TEXT("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    return NULL;
  }

  return(theForm);
}

static void
LoadChildsFromXML(WindowControl *Parent, CallBackTableEntry_t *LookUpTable,
                  XMLNode *Node, int ParentFont)
{
  int X,Y,Width,Height,Font;
  TCHAR Caption[128];
  TCHAR Name[64];
  COLORREF BackColor;
  COLORREF ForeColor;
  bool Visible;
  int Border;

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
#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    if (BackColor != 0xffffffff){
      BackColor = Color((BackColor>>16)&0xff,
                      (BackColor>>8)&0xff,
                      (BackColor>>0)&0xff);
    }
    if (ForeColor != 0xffffffff){
      ForeColor = Color((ForeColor>>16)&0xff,
                      (ForeColor>>8)&0xff,
                      (ForeColor>>0)&0xff);
    }
#endif /* !ENABLE_SDL */

    Font = StringToIntDflt(childNode.getAttribute(TEXT("Font")), ParentFont);
    Border = StringToIntDflt(childNode.getAttribute(TEXT("Border")), 0);

    if (_tcscmp(childNode.getName(), TEXT("WndProperty")) == 0) {

      WndProperty *W;
      int CaptionWidth;
      TCHAR DataNotifyCallback[128];
      TCHAR OnHelpCallback[128];
      int ReadOnly;
      int MultiLine;

      CaptionWidth =
        DLGSCALE(StringToIntDflt(childNode.getAttribute(TEXT("CaptionWidth")),
                        0));
      MultiLine =
        StringToIntDflt(childNode.getAttribute(TEXT("MultiLine")),
                        0);
      ReadOnly = \
        StringToIntDflt(childNode.getAttribute(TEXT("ReadOnly")),
                        0);

      _tcscpy(DataNotifyCallback,
              StringToStringDflt(childNode.getAttribute(TEXT("OnDataNotify")),
                                 TEXT("")));

      _tcscpy(OnHelpCallback,
              StringToStringDflt(childNode.getAttribute(TEXT("OnHelp")),
                                 TEXT("")));

      _tcscpy(
		  Caption,
			StringToStringDflt(childNode.getAttribute(TEXT("Caption")), TEXT(""))
		);
      // TODO code: Temporary double handling to fix "const unsigned
      // short *" to "unsigned short *" problem
	  _tcscpy(Caption, gettext(Caption));

      WC = W =
        new WndProperty(Parent, Name, Caption, X, Y,
                        Width, Height, CaptionWidth,
                        (WndProperty::DataChangeCallback_t)
                        CallBackLookup(LookUpTable, DataNotifyCallback),
                        MultiLine);

      W->SetOnHelpCallback((WindowControl::OnHelpCallback_t)
                           CallBackLookup(LookUpTable, OnHelpCallback));

      W->SetHelpText(StringToStringDflt(
                     childNode.getAttribute(TEXT("Help")),
                     TEXT("")));

      Caption[0] = '\0';
      W->SetReadOnly(ReadOnly != 0);

      if (childNode.nChildNode(TEXT("DataField")) > 0){
        TCHAR DataType[32];
        TCHAR DisplayFmt[32];
        TCHAR EditFormat[32];
        TCHAR OnDataAccess[64];
        double Min, Max, Step;
        int Fine;

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

	Fine = StringToIntDflt(dataFieldNode.
			       getAttribute(TEXT("Fine")), 0);

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
			  new DataFieldFloat(EditFormat, DisplayFmt, Min, Max, 0, Step, Fine,
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
    } else if (_tcscmp(childNode.getName(), TEXT("WndButton")) == 0){
      TCHAR ClickCallback[128];
       _tcscpy(ClickCallback, StringToStringDflt(childNode.getAttribute(TEXT("OnClickNotify")), TEXT("")));

      WC = new WndButton(Parent, Name, Caption, X, Y, Width, Height,
               (WndButton::ClickNotifyCallback_t)
                         CallBackLookup(LookUpTable, ClickCallback));

      Caption[0] = '\0';
    } else

#ifndef ALTAIRSYNC

    if (_tcscmp(childNode.getName(), TEXT("WndEventButton")) == 0){
      TCHAR iename[100];
      TCHAR ieparameters[100];
      _tcscpy(iename,
              StringToStringDflt(childNode.
                                 getAttribute(TEXT("InputEvent")),
                                 TEXT("")));
      _tcscpy(ieparameters,
              StringToStringDflt(childNode.
                                 getAttribute(TEXT("Parameters")),
                                 TEXT("")));

      WC = new WndEventButton(Parent, Name, Caption, X, Y, Width, Height,
                              iename, ieparameters);

      Caption[0] = '\0';
    } else

#endif

    if (_tcscmp(childNode.getName(), TEXT("WndOwnerDrawFrame")) == 0){
      TCHAR PaintCallback[128];
      _tcscpy(PaintCallback, StringToStringDflt(childNode.getAttribute(TEXT("OnPaint")), TEXT("")));
      WC = new WndOwnerDrawFrame(Parent, Name, X, Y, Width, Height,
               (WndOwnerDrawFrame::OnPaintCallback_t) CallBackLookup(LookUpTable, PaintCallback));
    } else if (_tcscmp(childNode.getName(), TEXT("WndFrame")) == 0){
      WndFrame *W;
      WC = W = new WndFrame(Parent, Name, X, Y, Width, Height);
      LoadChildsFromXML(W, LookUpTable, &childNode, ParentFont);  // recursivly create dialog
    } else if (_tcscmp(childNode.getName(), TEXT("WndListFrame")) == 0){
      TCHAR ListCallback[128];
      _tcscpy(ListCallback, StringToStringDflt(childNode.getAttribute(TEXT("OnListInfo")), TEXT("")));
      WC = new WndListFrame(Parent, Name, X, Y, Width, Height,
               (WndListFrame::OnListCallback_t) CallBackLookup(LookUpTable, ListCallback));
      LoadChildsFromXML(WC, LookUpTable, &childNode, ParentFont);  // recursivly create dialog
    }

    if (WC != NULL){
      if (Font != -1)
        WC->SetFont(FontMap[Font]);

#ifdef ENABLE_SDL
      // XXX
#else /* !ENABLE_SDL */
      if (BackColor != 0xffffffff){
        WC->SetBackColor(BackColor);
      }

      if (ForeColor != 0xffffffff){
        WC->SetForeColor(ForeColor);
      }
#endif /* !ENABLE_SDL */

      if (!Visible){
        WC->SetVisible(Visible);
      }

      if (Caption[0] != '\0'){
        WC->SetCaption(Caption);
      }

      if (Border != 0){
        WC->SetBorderKind(Border);
      }
    }
  }
}
