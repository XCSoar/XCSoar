/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "Form/EventButton.hpp"
#include "Form/Draw.hpp"
#include "Form/List.hpp"

#include <stdio.h>    // for _stprintf
#include <tchar.h>
#include <limits.h>

#include <algorithm>


typedef enum {
    eDialogFullWidth=0,  // cover screen, stretch controls horizontally
    eDialogScaled=1,  // stretch only frame to maintain aspect ratio
    eDialogScaledCentered=2, // like 1 but center dialog in screen
    eDialogFixed=3 // don't adjust at all (same as !Layout::ScaleSupported())
} DialogStyle_t;

double g_ddlgScaleWidth = 1.0; // used when stretching dialog and components
DialogStyle_t g_eDialogStyle=eDialogFullWidth;       // to full width of screen

int Scale_Dlg_Width(int x) {
  if (!Layout::ScaleSupported()) {
    return Layout::Scale(x);
  }
  if (g_eDialogStyle == eDialogFullWidth) {
    return (int) ((x)*g_ddlgScaleWidth); // stretch width to fill screen horizontally
  }
  else {
    return Layout::Scale(x);
  }
}

using std::min;

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

static bool
StringToColor(const TCHAR *String, Color &color)
{
  long value = StringToIntDflt(String, -1);
  if (value & ~0xffffff)
    return false;

  color = Color((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
  return true;
}

// Popup=1 says don't stretch to cover entire screen
static void
GetDefaultWindowControlProps(XMLNode *Node, TCHAR *Name, int *X, int *Y,
                             int *Width, int *Height, int *Font,
                             TCHAR *Caption)
{
  *X = Scale_Dlg_Width(StringToIntDflt(Node->getAttribute(_T("X")), 0));
  *Y = StringToIntDflt(Node->getAttribute(_T("Y")), 0);
  if (*Y>=0) { // not -1
    (*Y) = Layout::Scale(*Y);
  }
  *Width = Scale_Dlg_Width(StringToIntDflt(Node->getAttribute(_T("Width")), 50));
  *Height = StringToIntDflt(Node->getAttribute(_T("Height")), 50);
  if (*Height>=0) {
    (*Height) = Layout::Scale(*Height);
  }
  *Font = StringToIntDflt(Node->getAttribute(_T("Font")), -1);
  _tcscpy(Name, StringToStringDflt(Node->getAttribute(_T("Name")), _T("")));
  _tcscpy(Caption, StringToStringDflt(Node->getAttribute(_T("Caption")), _T("")));
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
  hResInfo = FindResource (XCSoarInterface::hInst, lpName, _T("XMLDialog"));

  if (hResInfo == NULL) {
    MessageBoxX(
      gettext(_T("Can't find resource")),
      gettext(_T("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    // unable to find the resource
    return XMLNode::emptyXMLNode;
  }

  // Load the wave resource.
  hRes = LoadResource (XCSoarInterface::hInst, hResInfo);

  if (hRes == NULL) {
    MessageBoxX(
      gettext(_T("Can't load resource")),
      gettext(_T("Dialog error")),
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
        MessageBoxX(gettext(_T("Can't allocate memory")),
                    gettext(_T("Dialog error")),
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
      if (!IsTextUnicode(buf, min(l, 10000), NULL))
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
      if (IsTextUnicode(buf, min(l, 10000), NULL))
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
  MessageBoxX(gettext(_T("Can't lock resource")),
              gettext(_T("Dialog error")),
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
      _stprintf(errortext,_T("%s %i %i"), XMLNode::getError(pResults.error),
                pResults.nLine, pResults.nColumn);

      MessageBoxX(errortext,
                  gettext(_T("Dialog error")),
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
  xMainNode=XMLNode::openFileHelper(FileName ,_T("PMML"));
#else
  char winname[200];
  sprintf(winname,"C:\\XCSoar%s",FileName);
  xMainNode=XMLNode::openFileHelper(winname ,_T("PMML"));
#endif
*/

  char FileName[MAX_PATH];
  LocalPathS(FileName, name);

  if (FileExistsA(FileName))   //sgi use window API cals to check if
                               //file exists, this will supress
                               //CodeGurad warnings on callinf
                               //fopen(<unexisting file>)
    xMainNode=XMLNode::openFileHelper(FileName ,_T("PMML"));

#ifdef WIN32
  if (xMainNode.isEmpty()) {
    if (resource) {
      xMainNode =xmlOpenResourceHelper(resource,
                                       _T("PMML"));
    }
  }
#endif /* WIN32 */

  return xMainNode;
}

static void
LoadColors(WindowControl &wc, const XMLNode &node)
{
    Color color;

    if (StringToColor(node.getAttribute(_T("BackColor")), color))
      wc.SetBackColor(color);

    if (StringToColor(node.getAttribute(_T("ForeColor")), color))
      wc.SetForeColor(color);
}

static
void
CalcWidthStretch(XMLNode *xNode, const RECT rc)
{
  if (!Layout::ScaleSupported()) {
    g_eDialogStyle = eDialogFixed;
  } else {
    g_eDialogStyle = (DialogStyle_t)StringToIntDflt(xNode->getAttribute(
        TEXT("Popup")), 0);
  }

  int Width = StringToIntDflt(xNode->getAttribute(TEXT("Width")), 50);
  if (g_eDialogStyle == eDialogFullWidth) {
    g_ddlgScaleWidth = (double)(rc.right - rc.left) / (double)Width;
  } else {
    g_ddlgScaleWidth = Layout::Scale(1); // retain dialog geometry
  }
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
      gettext(_T("Error in loading XML dialog")),
      gettext(_T("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    return NULL;
  }

  XMLNode xNode=xMainNode.getChildNode(_T("WndForm"));

  FontMap[0] = &TitleWindowFont;
  FontMap[1] = &MapWindowFont;
  FontMap[2] = &MapWindowBoldFont;
  FontMap[3] = &CDIWindowFont;
  FontMap[4] = &InfoWindowFont;

  if (!xNode.isEmpty()){
    int X,Y,Width,Height,Font;
    TCHAR sTmp[128];
    TCHAR Name[64];

    const RECT rc = Parent.get_client_rect();
    CalcWidthStretch(&xNode, rc);

    GetDefaultWindowControlProps(&xNode, Name, &X, &Y, &Width, &Height, &Font,
        sTmp);

    switch (g_eDialogStyle) {
    case eDialogFullWidth:
      X = rc.top;
      Y = rc.bottom;
      Width = rc.right - rc.left; // stretch form to full width of screen
      Height = rc.bottom - rc.top;
      X = 0;
      Y = 0;
      break;
    case eDialogScaled:
      break;
    case eDialogScaledCentered:
      X = (rc.right - rc.left) / 2; // center form horizontally on screen
      break;
    case eDialogFixed:
      break;
    }

    theForm = new WndForm(&Parent, Name, sTmp, X, Y, Width, Height);

    if (Font != -1)
      theForm->SetTitleFont(*FontMap[Font]);

    if (Font != -1)
      theForm->SetFont(*FontMap[Font]);

    LoadColors(*theForm, xNode);

    LoadChildsFromXML(theForm, LookUpTable, &xNode, Font);

    if (XMLNode::GlobalError) {
      MessageBoxX(
                 gettext(_T("Error in loading XML dialog")),
                 gettext(_T("Dialog error")),
                 MB_OK|MB_ICONEXCLAMATION);

      delete theForm;
      return NULL;
    }
  } else {
    MessageBoxX(
      gettext(_T("Error in loading XML dialog")),
      gettext(_T("Dialog error")),
      MB_OK|MB_ICONEXCLAMATION);

    return NULL;
  }

  return(theForm);
}

static DataField *
LoadDataField(XMLNode node, CallBackTableEntry_t *LookUpTable)
{
  TCHAR DataType[32];
  TCHAR DisplayFmt[32];
  TCHAR EditFormat[32];
  TCHAR OnDataAccess[64];
  double Min, Max, Step;
  int Fine;

  _tcscpy(DataType,
          StringToStringDflt(node.getAttribute(_T("DataType")),
                             _T("")));
  _tcscpy(DisplayFmt,
          StringToStringDflt(node. getAttribute(_T("DisplayFormat")),
                             _T("")));
  _tcscpy(EditFormat,
          StringToStringDflt(node.getAttribute(_T("EditFormat")),
                             _T("")));
  _tcscpy(OnDataAccess,
          StringToStringDflt(node.getAttribute(_T("OnDataAccess")),
                             _T("")));

  Min = StringToIntDflt(node.getAttribute(_T("Min")), INT_MIN);
  Max = StringToIntDflt(node.getAttribute(_T("Max")), INT_MAX);
  Step = StringToFloatDflt(node.getAttribute(_T("Step")), 1);
  Fine = StringToIntDflt(node.getAttribute(_T("Fine")), 0);

  DataField::DataAccessCallback_t callback = (DataField::DataAccessCallback_t)
    CallBackLookup(LookUpTable, OnDataAccess);

  if (_tcsicmp(DataType, _T("enum")) == 0)
    return new DataFieldEnum(EditFormat, DisplayFmt, false, callback);

  if (_tcsicmp(DataType, _T("filereader")) == 0)
    return new DataFieldFileReader(EditFormat, DisplayFmt, callback);

  if (_tcsicmp(DataType, _T("boolean")) == 0)
    return new DataFieldBoolean(EditFormat, DisplayFmt, false,
                                _T("ON"), _T("OFF"), callback);

  if (_tcsicmp(DataType, _T("double")) == 0)
    return new DataFieldFloat(EditFormat, DisplayFmt, Min, Max, 0, Step, Fine,
                              callback);

  if (_tcsicmp(DataType, _T("integer")) == 0)
    return new DataFieldInteger(EditFormat, DisplayFmt, (int)Min, (int)Max,
                                (int)0, (int)Step, callback);

  if (_tcsicmp(DataType, _T("string")) == 0)
    return new DataFieldString(EditFormat, DisplayFmt, _T(""), callback);

  return NULL;
}

static void
LoadChild(WindowControl *Parent, CallBackTableEntry_t *LookUpTable,
          XMLNode node, int ParentFont)
{
  int X,Y,Width,Height,Font;
  TCHAR Caption[128];
  TCHAR Name[64];
  bool Visible;
  int Border;

  WindowControl *WC=NULL;

  GetDefaultWindowControlProps(&node,
                               Name,
                               &X, &Y,
                               &Width, &Height,
                               &Font, Caption);

  Visible = StringToIntDflt(node.getAttribute(_T("Visible")), 1) == 1;

  Font = StringToIntDflt(node.getAttribute(_T("Font")), ParentFont);
  Border = StringToIntDflt(node.getAttribute(_T("Border")), 0);

  if (_tcscmp(node.getName(), _T("WndProperty")) == 0) {
    WndProperty *W;
    int CaptionWidth;
    TCHAR DataNotifyCallback[128];
    TCHAR OnHelpCallback[128];
    int ReadOnly;
    int MultiLine;

    CaptionWidth =
        Scale_Dlg_Width(StringToIntDflt(node.getAttribute(_T("CaptionWidth")),
                                    0));
    MultiLine = StringToIntDflt(node.getAttribute(_T("MultiLine")), 0);
    ReadOnly = StringToIntDflt(node.getAttribute(_T("ReadOnly")), 0);

    _tcscpy(DataNotifyCallback,
            StringToStringDflt(node.getAttribute(_T("OnDataNotify")),
                               _T("")));

    _tcscpy(OnHelpCallback,
            StringToStringDflt(node.getAttribute(_T("OnHelp")),
                               _T("")));

    _tcscpy(Caption,
            StringToStringDflt(node.getAttribute(_T("Caption")),
                               _T("")));

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

    W->SetHelpText(StringToStringDflt(node.getAttribute(_T("Help")),
                                      _T("")));

    Caption[0] = '\0';
    W->SetReadOnly(ReadOnly != 0);

    if (node.nChildNode(_T("DataField")) > 0){
      DataField *data_field =
        LoadDataField(node.getChildNode(_T("DataField"), 0),
                      LookUpTable);
      if (data_field != NULL)
        W->SetDataField(data_field);
    }
  } else if (_tcscmp(node.getName(), _T("WndButton")) == 0){
    TCHAR ClickCallback[128];
    _tcscpy(ClickCallback, StringToStringDflt(node.getAttribute(_T("OnClickNotify")), _T("")));

    WC = new WndButton(Parent, Name, Caption, X, Y, Width, Height,
                       (WndButton::ClickNotifyCallback_t)
                       CallBackLookup(LookUpTable, ClickCallback));

    Caption[0] = '\0';
#ifndef ALTAIRSYNC
  } else if (_tcscmp(node.getName(), _T("WndEventButton")) == 0){
    TCHAR iename[100];
    TCHAR ieparameters[100];
    _tcscpy(iename,
            StringToStringDflt(node.
                               getAttribute(_T("InputEvent")),
                               _T("")));
    _tcscpy(ieparameters,
            StringToStringDflt(node.
                               getAttribute(_T("Parameters")),
                               _T("")));

    WC = new WndEventButton(Parent, Name, Caption, X, Y, Width, Height,
                            iename, ieparameters);

    Caption[0] = '\0';
#endif
  } else if (_tcscmp(node.getName(), _T("WndOwnerDrawFrame")) == 0){
    TCHAR PaintCallback[128];
    _tcscpy(PaintCallback,
            StringToStringDflt(node.getAttribute(_T("OnPaint")), _T("")));
    WC = new WndOwnerDrawFrame(Parent, Name, X, Y, Width, Height,
                               (WndOwnerDrawFrame::OnPaintCallback_t)
                               CallBackLookup(LookUpTable, PaintCallback));
  } else if (_tcscmp(node.getName(), _T("WndFrame")) == 0){
    WC = new WndFrame(Parent, Name, X, Y, Width, Height);

    // recursivly create dialog
    LoadChildsFromXML(WC, LookUpTable, &node, ParentFont);
  } else if (_tcscmp(node.getName(), _T("WndListFrame")) == 0){
    unsigned item_height =
      Layout::Scale(StringToIntDflt(node.getAttribute(_T("ItemHeight")), 18));
    WC = new WndListFrame(Parent, Name, X, Y, Width, Height, item_height);

    // recursivly create dialog
    LoadChildsFromXML(WC, LookUpTable, &node, ParentFont);
  }

  if (WC != NULL){
    if (Font != -1)
      WC->SetFont(FontMap[Font]);

    LoadColors(*WC, node);

    if (!Visible)
      WC->hide();

    if (Caption[0] != '\0')
      WC->SetCaption(Caption);

    if (Border != 0)
      WC->SetBorderKind(Border);
  }
}

static void
LoadChildsFromXML(WindowControl *Parent, CallBackTableEntry_t *LookUpTable,
                  XMLNode *Node, int ParentFont)
{
  int Count = Node->nChildNode();

  for (int i = 0; i < Count; i++)
    LoadChild(Parent, LookUpTable,
              Node->getChildNode(i), ParentFont);
}

