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
#include "Screen/SingleWindow.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/EventButton.hpp"
#include "Form/Draw.hpp"
#include "Form/List.hpp"
#include "Form/Tabbed.hpp"
#include "Form/Panel.hpp"
#include "StringUtil.hpp"

#include <stdio.h>    // for _stprintf
#include <tchar.h>
#include <limits.h>

#include <algorithm>

// used when stretching dialog and components
double g_ddlgScaleWidth = 1.0;
// to full width of screen
DialogStyle_t g_eDialogStyle = eDialogFullWidth;

using std::min;

/**
 * Converts a String into an Integer and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Integer value
 */
static long
StringToIntDflt(const TCHAR *String, long Default)
{
  if (String == NULL || String[0] == '\0')
    return Default;
  return _tcstol(String, NULL, 0);
}

/**
 * Converts a String into a Float and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Float value
 */
static double
StringToFloatDflt(const TCHAR *String, double Default)
{
  if (String == NULL || String[0] == '\0')
    return Default;
  return _tcstod(String, NULL);
}

/**
 * Returns the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The output String
 */
static const TCHAR *
StringToStringDflt(const TCHAR *String, const TCHAR *Default)
{
  if (String == NULL || String[0] == '\0')
    return Default;
  return String;
}

/**
 * Converts a String into a Color and sets
 * a default value if String = NULL
 * @param String The String to parse
 * @param color The color (output)
 */
static bool
StringToColor(const TCHAR *String, Color &color)
{
  long value = StringToIntDflt(String, -1);
  if (value & ~0xffffff)
    return false;

  color = Color((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
  return true;
}

/**
 * Returns the dialog style property ("Popup") of the given node
 * @param xNode The node to check
 * @return Dialog style (DialogStyle_t), Default = FullWidth
 */
static DialogStyle_t
GetDialogStyle(XMLNode *xNode) 
{
  const TCHAR* popup = xNode->getAttribute(_T("Popup"));
  if ((popup == NULL) || string_is_empty(popup))
    return g_eDialogStyle;
  else
    return (DialogStyle_t)StringToIntDflt(popup, 0);
}

static int 
Scale_Dlg_Width(const int x, const DialogStyle_t eDialogStyle) 
{
  if (!Layout::ScaleSupported())
    // todo: return x; ?!
    return Layout::Scale(x);

  if (eDialogStyle == eDialogFullWidth)
    // stretch width to fill screen horizontally
    return (int)(x * g_ddlgScaleWidth);
  else
    return Layout::Scale(x);
}

/**
 * This function reads the following parameters from the XML Node and
 * saves them as Control properties:
 * Name, x-, y-Coordinate, Width, Height, Font and Caption
 * @param Node The XML Node that represents the Control
 * @param Name Name of the Control (pointer)
 * @param X x-Coordinate of the Control (pointer)
 * @param Y y-Coordinate of the Control (pointer)
 * @param Width Width of the Control (pointer)
 * @param Height Height of the Control (pointer)
 * @param Font Font of the Control (pointer)
 * @param Caption Caption of the Control (pointer)
 * @param eDialogStyle Dialog style of the Form
 */
static void
GetDefaultWindowControlProps(XMLNode *Node, TCHAR *Name, int *X, int *Y,
                             int *Width, int *Height, int *Font,
                             TCHAR *Caption, const DialogStyle_t eDialogStyle)
{
  // Calculate x- and y-Coordinate
  *X = Scale_Dlg_Width(StringToIntDflt(Node->getAttribute(_T("X")), 0),
                       eDialogStyle);
  *Y = StringToIntDflt(Node->getAttribute(_T("Y")), 0);
  if (*Y >= 0) { // not -1
    (*Y) = Layout::Scale(*Y);
  }

  // Calculate width and height
  *Width = Scale_Dlg_Width(StringToIntDflt(Node->getAttribute(_T("Width")), 50),
                           eDialogStyle);
  *Height = StringToIntDflt(Node->getAttribute(_T("Height")), 50);
  if (*Height >= 0) {
    (*Height) = Layout::Scale(*Height);
  }

  // Determine font style
  *Font = StringToIntDflt(Node->getAttribute(_T("Font")), -1);

  // Determine name and caption
  _tcscpy(Name, StringToStringDflt(Node->getAttribute(_T("Name")), _T("")));
  _tcscpy(Caption, StringToStringDflt(Node->getAttribute(_T("Caption")), _T("")));

  // TODO code: Temporary double handling to
  // fix "const unsigned short*" to "unsigned short *" problem

  // Translate caption
  _tcscpy(Caption, gettext(Caption));
}

static void *
CallBackLookup(CallBackTableEntry_t *LookUpTable, TCHAR *Name)
{
  int i;

  if (LookUpTable != NULL && Name != NULL && Name[0] != '\0')
    for (i = 0; LookUpTable[i].Ptr != NULL; i++) {
      if (_tcscmp(LookUpTable[i].Name, Name) == 0) {
        return LookUpTable[i].Ptr;
      }
    }

  return NULL;
}

static void
LoadChildrenFromXML(ContainerControl *Parent, CallBackTableEntry_t *LookUpTable,
                    XMLNode *Node, int Font, const DialogStyle_t eDialogStyle);

static Font *FontMap[5] = {
  &TitleWindowFont,
  &MapWindowFont,
  &MapWindowBoldFont,
  &CDIWindowFont,
  &InfoWindowFont
};

#ifdef WIN32

static XMLNode
xmlLoadFromResource(const TCHAR* lpName, XMLResults *pResults)
{
  LPTSTR lpRes;
  HRSRC hResInfo;
  HGLOBAL hRes;
  int l, len;

  // Find the xml resource.
  hResInfo = FindResource(XCSoarInterface::hInst, lpName, _T("XMLDialog"));

  if (hResInfo == NULL) {
    MessageBoxX(gettext(_T("Can't find resource")), gettext(_T("Dialog error")),
                MB_OK | MB_ICONEXCLAMATION);

    // unable to find the resource
    return XMLNode::emptyXMLNode;
  }

  // Load the wave resource.
  hRes = LoadResource(XCSoarInterface::hInst, hResInfo);

  if (hRes == NULL) {
    MessageBoxX(gettext(_T("Can't load resource")), gettext(_T("Dialog error")),
                MB_OK | MB_ICONEXCLAMATION);

    // unable to load the resource
    return XMLNode::emptyXMLNode;
  }

  // Lock the wave resource and do something with it.
  lpRes = (LPTSTR)LockResource(hRes);

  if (lpRes) {
    l = SizeofResource(XCSoarInterface::hInst, hResInfo);
    if (l > 0) {
      char *buf = (char*)malloc(l + 2);
      if (!buf) {
        MessageBoxX(gettext(_T("Can't allocate memory")), gettext(
            _T("Dialog error")), MB_OK | MB_ICONEXCLAMATION);
        // unable to allocate memory
        return XMLNode::emptyXMLNode;
      }
      strncpy(buf, (char*)lpRes, l);
      buf[l] = 0; // need to explicitly null-terminate.
      buf[l + 1] = 0;
      len = l;

#if defined(WIN32) || defined(UNDER_CE)
#ifdef _UNICODE
#if !defined(UNDER_CE) && !defined(WINDOWSPC)
      if (!IsTextUnicode(buf, min(l, 10000), NULL)) {
#endif
        LPTSTR b2 = (LPTSTR)malloc(l * 2 + 2);
        MultiByteToWideChar(CP_ACP,          // code page
                            MB_PRECOMPOSED,  // character-type options
                            buf,             // string to map
                            l,               // number of bytes in string
                            b2,              // wide-character buffer
                            l * 2 + 2);      // size of buffer
        free(buf);
        buf = (char*)b2;
        buf[l * 2] = 0;
        buf[l * 2 + 1] = 0;
#if !defined(UNDER_CE) && !defined(WINDOWSPC)
      }
#endif
#else
      if (IsTextUnicode(buf, min(l, 10000), NULL)) {
        l >>= 1;
        LPTSTR b2 = (LPTSTR)malloc(l + 2);
        WideCharToMultiByte(CP_ACP,                      // code page
                            0,                           // performance and mapping flags
                            (const WCHAR*)buf,           // wide-character string
                            l,                           // number of chars in string
                            b2,                          // buffer for new string
                            l + 2,                       // size of buffer
                            NULL,                        // default for unmappable chars
                            NULL                         // set when default char used
                            );
        free(buf);
        buf = (char*)b2;
      }
#endif
#endif

      XMLNode x = XMLNode::parseString((LPTSTR)buf, pResults);

      free(buf);
      return x;
    }
  }

  MessageBoxX(gettext(_T("Can't lock resource")), gettext(_T("Dialog error")),
              MB_OK | MB_ICONEXCLAMATION);

  return XMLNode::emptyXMLNode;
}

/**
 * Tries to load an XML file from the resources
 * @param lpszXML The resource name
 * @param tag (?)
 * @return The parsed XMLNode
 */
static XMLNode
xmlOpenResourceHelper(const TCHAR *lpszXML)
{
  XMLResults pResults;

  pResults.error = eXMLErrorNone;
  XMLNode::GlobalError = false;
  XMLNode xnode = xmlLoadFromResource(lpszXML, &pResults);
  if (pResults.error != eXMLErrorNone) {
    XMLNode::GlobalError = true;
    TCHAR errortext[100];
    _stprintf(errortext,_T("%s %i %i"), XMLNode::getError(pResults.error),
              pResults.nLine, pResults.nColumn);

    MessageBoxX(errortext, gettext(_T("Dialog error")),
                MB_OK | MB_ICONEXCLAMATION);
  }
  return xnode;
}

#endif /* WIN32 */

/**
 * This function searches for the given (file)name and if not found
 * resource and returns the main XMLNode
 * @param name File to search for
 * @param resource Resource to search for
 * @return The main XMLNode
 */
static const XMLNode
load_xml_file_or_resource(const TCHAR *name, const TCHAR* resource)
{
  XMLNode xMainNode;

  // Get filepath
  char FileName[MAX_PATH];
  LocalPathS(FileName, name);

  // If file exists -> Load XML from file
  if (FileExistsA(FileName))
    xMainNode = XMLNode::openFileHelper(FileName);

#ifdef WIN32

  // If XML file hasn't been loaded
  if (xMainNode.isEmpty())
    // and resource exists
    if (resource)
      // -> Load XML from resource
      xMainNode = xmlOpenResourceHelper(resource);

#endif

  return xMainNode;
}

/**
 * Loads the color information from the XMLNode and sets the fore- and
 * background color of the given WindowControl
 * @param wc The WindowControl
 * @param node The XMLNode
 */
static void
LoadColors(WindowControl &wc, const XMLNode &node)
{
  Color color;

  if (StringToColor(node.getAttribute(_T("BackColor")), color))
    wc.SetBackColor(color);

  if (StringToColor(node.getAttribute(_T("ForeColor")), color))
    wc.SetForeColor(color);
}

static void
CalcWidthStretch(XMLNode *xNode, const RECT rc, const DialogStyle_t eDialogStyle)
{
  int Width = StringToIntDflt(xNode->getAttribute(_T("Width")), 50);

  if ((eDialogStyle == eDialogFullWidth) && Layout::ScaleSupported())
    g_ddlgScaleWidth = (double)(rc.right - rc.left) / (double)Width;
  else
    g_ddlgScaleWidth = Layout::Scale(1); // retain dialog geometry
}

/**
 * This function returns a WndForm created either from the ressources or
 * from the XML file in XCSoarData(if found)
 * @param LookUpTable The CallBackTable
 * @param FileName The XML filename to search for in XCSoarData
 * @param Parent The parent window (e.g. XCSoarInterface::main_window)
 * @param resource The resource to look for
 * @return The WndForm object
 */
WndForm *
dlgLoadFromXML(CallBackTableEntry_t *LookUpTable, const TCHAR *FileName,
               SingleWindow &Parent, const TCHAR* resource)
{

  WndForm *theForm = NULL;

  // assert(main_window == Parent);  // Airspace warning has MapWindow as parent,
  // ist that ok?  JMW: No, I think that it is better to use main UI thread for
  // everything.  See changes regarding RequestAirspaceDialog in AirspaceWarning.cpp

  // Find XML file or resource and load XML data out of it
  XMLNode xMainNode = load_xml_file_or_resource(FileName, resource);

  // TODO code: put in error checking here and get rid of exits in xmlParser
  // If XML error occurred -> Error messagebox + cancel
  if (xMainNode.isEmpty()) {
    MessageBoxX(gettext(_T("Error in loading XML dialog")),
                gettext(_T("Dialog error")), MB_OK | MB_ICONEXCLAMATION);

    return NULL;
  }

  XMLNode xNode;

  // If the main XMLNode is of type "WndForm"
  if (_tcsicmp(xMainNode.getName(), _T("WndForm")) == 0)
    // -> save it as the dialog node
    xNode = xMainNode;
  else
    // Get the first child node of the type "WndForm"
    // and save it as the dialog node
    xNode = xMainNode.getChildNode(_T("WndForm"));

  FontMap[0] = &TitleWindowFont;
  FontMap[1] = &MapWindowFont;
  FontMap[2] = &MapWindowBoldFont;
  FontMap[3] = &CDIWindowFont;
  FontMap[4] = &InfoWindowFont;

  // If Node does not exists -> Error messagebox + cancel
  if (xNode.isEmpty()) {
    MessageBoxX(gettext(_T("Error in loading XML dialog")),
                gettext(_T("Dialog error")), MB_OK | MB_ICONEXCLAMATION);

    return NULL;
  }

  int X, Y, Width, Height, Font;
  TCHAR sTmp[128];
  TCHAR Name[64];

  // todo: this dialog style stuff seems a little weird...

  // Determine the dialog style of the dialog
  DialogStyle_t eDialogStyle = GetDialogStyle(&xNode);

  // Determine the dialog size
  const RECT rc = Parent.get_client_rect();
  CalcWidthStretch(&xNode, rc, eDialogStyle);

  GetDefaultWindowControlProps(&xNode, Name, &X, &Y, &Width, &Height, &Font,
                               sTmp, eDialogStyle);

  // Correct dialog size and position for dialog style
  switch (eDialogStyle) {
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

  // Create the dialog
  WindowStyle style;
  style.hide();

  theForm = new WndForm(Parent, Name, sTmp, X, Y, Width, Height, style);

  // Sets Fonts
  if (Font != -1)
    theForm->SetTitleFont(*FontMap[Font]);

  if (Font != -1)
    theForm->SetFont(*FontMap[Font]);

  // Set fore- and background colors
  LoadColors(*theForm, xNode);

  // Load the children controls
  LoadChildrenFromXML(theForm, LookUpTable, &xNode, Font, eDialogStyle);

  // If XML error occurred -> Error messagebox + cancel
  if (XMLNode::GlobalError) {
    MessageBoxX(gettext(_T("Error in loading XML dialog")),
                gettext(_T("Dialog error")), MB_OK | MB_ICONEXCLAMATION);

    delete theForm;
    return NULL;
  }

  // Return the created form
  return theForm;
}

static DataField *
LoadDataField(XMLNode node, CallBackTableEntry_t *LookUpTable, 
              const DialogStyle_t eDialogStyle)
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

/**
 * Creates a control from the given XMLNode as a child of the given parent
 * ContainerControl.
 * @param Parent The parent ContainerControl
 * @param LookUpTable The parent CallBackTable
 * @param node The XMLNode that represents the control
 * @param ParentFont The parent's font array index
 * @param eDialogStyle The parent's dialog style
 */
static void
LoadChild(ContainerControl *Parent, CallBackTableEntry_t *LookUpTable,
          XMLNode node, int ParentFont, const DialogStyle_t eDialogStyle)
{
  int X, Y, Width, Height, Font;
  TCHAR Caption[128];
  TCHAR Name[64];
  bool Visible;
  int Border;

  WindowControl *WC = NULL;

  // Determine name, coordinates, width, height,
  // font and caption of the control
  GetDefaultWindowControlProps(&node, Name, &X, &Y, &Width, &Height,
                               &Font, Caption, eDialogStyle);

  // Determine whether the control is visible on startup (default = visible)
  Visible = StringToIntDflt(node.getAttribute(_T("Visible")), 1) == 1;

  // Determine the control's font (default = parent's font)
  Font = StringToIntDflt(node.getAttribute(_T("Font")), ParentFont);

  // Determine the control's border kind (default = 0)
  Border = StringToIntDflt(node.getAttribute(_T("Border")), 0);

  // PropertyControl (WndProperty)
  if (_tcscmp(node.getName(), _T("WndProperty")) == 0) {
    WndProperty *W;
    int CaptionWidth;
    TCHAR DataNotifyCallback[128];
    TCHAR OnHelpCallback[128];
    int ReadOnly;
    int MultiLine;

    // Determine the width of the caption field
    CaptionWidth = 
      Scale_Dlg_Width(StringToIntDflt(node.getAttribute(_T("CaptionWidth")), 0),
                      eDialogStyle);

    // Determine whether the control is multiline or readonly
    MultiLine = StringToIntDflt(node.getAttribute(_T("MultiLine")), 0);
    ReadOnly = StringToIntDflt(node.getAttribute(_T("ReadOnly")), 0);

    // Load the event callback properties
    _tcscpy(DataNotifyCallback,
            StringToStringDflt(node.getAttribute(_T("OnDataNotify")), _T("")));

    _tcscpy(OnHelpCallback,
            StringToStringDflt(node.getAttribute(_T("OnHelp")), _T("")));

    // Load the caption
    // todo: duplicate?!
    _tcscpy(Caption,
            StringToStringDflt(node.getAttribute(_T("Caption")), _T("")));

    // TODO code: Temporary double handling to fix "const unsigned
    // short *" to "unsigned short *" problem
    // Translate the caption
    _tcscpy(Caption, gettext(Caption));

    // Create the Property Control

    EditWindowStyle edit_style;
    edit_style.border();

    if (MultiLine) {
      edit_style.multiline();
      edit_style.vscroll();
    }

    WC = W = new WndProperty(Parent, Name, Caption, X, Y, Width, Height,
                             CaptionWidth,
                             WindowStyle(),
                             edit_style,
                             (WndProperty::DataChangeCallback_t)
                             CallBackLookup(LookUpTable, DataNotifyCallback));

    // Set the help function event callback
    W->SetOnHelpCallback((WindowControl::OnHelpCallback_t)
                         CallBackLookup(LookUpTable, OnHelpCallback));

    // Load the help text
    W->SetHelpText(StringToStringDflt(node.getAttribute(_T("Help")), _T("")));

    Caption[0] = '\0';

    // Set the control as readonly, if wanted
    W->SetReadOnly(ReadOnly != 0);

    // If the control has (at least) one DataField child control
    if (node.nChildNode(_T("DataField")) > 0){
      // -> Load the first DataField control
      DataField *data_field =
        LoadDataField(node.getChildNode(_T("DataField"), 0),
                      LookUpTable, eDialogStyle);

      if (data_field != NULL)
        // Tell the Property control about the DataField control
        W->SetDataField(data_field);
    }

  // ButtonControl (WndButton)
  } else if (_tcscmp(node.getName(), _T("WndButton")) == 0) {
    // Determine ClickCallback function
    TCHAR ClickCallback[128];
    _tcscpy(ClickCallback,
            StringToStringDflt(node.getAttribute(_T("OnClickNotify")), _T("")));

    // Create the ButtonControl
    WC = new WndButton(Parent, Name, Caption, X, Y, Width, Height,
                       WindowStyle(),
                       (WndButton::ClickNotifyCallback_t)
                       CallBackLookup(LookUpTable, ClickCallback));

    Caption[0] = '\0';

#ifndef ALTAIRSYNC
  // EventButtonControl (WndEventButton) not used yet
  } else if (_tcscmp(node.getName(), _T("WndEventButton")) == 0) {
    TCHAR iename[100];
    TCHAR ieparameters[100];
    _tcscpy(iename,
            StringToStringDflt(node.getAttribute(_T("InputEvent")), _T("")));
    _tcscpy(ieparameters,
            StringToStringDflt(node.getAttribute(_T("Parameters")), _T("")));

    // Create the EventButtonControl
    WC = new WndEventButton(Parent, Name, Caption, X, Y, Width, Height,
                            WindowStyle(),
                            iename, ieparameters);

    Caption[0] = '\0';
#endif

  // PanelControl (WndPanel)
  } else if (_tcscmp(node.getName(), _T("Panel")) == 0) {
    // Create the PanelControl
    PanelControl *frame = new PanelControl(Parent, Name, X, Y, Width, Height);
    WC = frame;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(frame, LookUpTable, &node, ParentFont, eDialogStyle);

  // DrawControl (WndOwnerDrawFrame)
  } else if (_tcscmp(node.getName(), _T("WndOwnerDrawFrame")) == 0) {
    // Determine DrawCallback function
    TCHAR PaintCallback[128];
    _tcscpy(PaintCallback,
            StringToStringDflt(node.getAttribute(_T("OnPaint")), _T("")));

    // Create the DrawControl
    WC = new WndOwnerDrawFrame(Parent, Name, X, Y, Width, Height,
                               WindowStyle(),
                               (WndOwnerDrawFrame::OnPaintCallback_t)
                               CallBackLookup(LookUpTable, PaintCallback));

  // FrameControl (WndFrame)
  } else if (_tcscmp(node.getName(), _T("WndFrame")) == 0){
    // Create the FrameControl
    WC = new WndFrame(Parent, Name, X, Y, Width, Height,
                      WindowStyle());

  // ListBoxControl (WndListFrame)
  } else if (_tcscmp(node.getName(), _T("WndListFrame")) == 0){
    // Determine ItemHeight of the list items
    unsigned item_height =
      Layout::Scale(StringToIntDflt(node.getAttribute(_T("ItemHeight")), 18));

    // Create the ListBoxControl
    WC = new WndListFrame(Parent, Name, X, Y, Width, Height,
                          WindowStyle(),
                          item_height);

  // TabControl (Tabbed)
  } else if (_tcscmp(node.getName(), _T("Tabbed")) == 0) {
    // Create the TabControl
    TabbedControl *tabbed = new TabbedControl(Parent, Name, X, Y, Width, Height);
    WC = tabbed;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(tabbed, LookUpTable, &node, ParentFont, eDialogStyle);
  }

  // If WindowControl has been created
  if (WC != NULL) {
    // Set the font style
    if (Font != -1)
      WC->SetFont(FontMap[Font]);

    // Set the fore- and background color
    LoadColors(*WC, node);

    // If control is invisible -> hide it
    if (!Visible)
      WC->hide();

    // If caption hasn't been set -> set it
    if (Caption[0] != '\0')
      WC->SetCaption(Caption);

    // Set the border kind
    if (Border != 0)
      WC->SetBorderKind(Border);
  }
}

/**
 * Loads the Parent's children Controls from the given XMLNode
 * @param Parent The parent control
 * @param LookUpTable The parents CallBackTable
 * @param Node The XMLNode that represents the parent control
 * @param ParentFont The parent's font array index
 * @param eDialogStyle The parent's dialog style
 */
static void
LoadChildrenFromXML(ContainerControl *Parent, CallBackTableEntry_t *LookUpTable,
                    XMLNode *Node, int ParentFont, const DialogStyle_t eDialogStyle)
{
  // Get the number of childnodes
  int Count = Node->nChildNode();

  // Iterate through the childnodes
  for (int i = 0; i < Count; i++)
    // Load each child control from the child nodes
    LoadChild(Parent, LookUpTable, Node->getChildNode(i), ParentFont,
              eDialogStyle);
}
