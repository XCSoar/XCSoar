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
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/FileReader.hpp"
#include "DataField/Float.hpp"
#include "DataField/Integer.hpp"
#include "DataField/String.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/Draw.hpp"
#include "Form/List.hpp"
#include "Form/Tabbed.hpp"
#include "Form/Panel.hpp"
#include "Form/Keyboard.hpp"
#include "Form/CheckBox.hpp"
#include "StringUtil.hpp"
#include "ResourceLoader.hpp"

#include <stdio.h>    // for _stprintf
#include <tchar.h>
#include <limits.h>

// used when stretching dialog and components
static int dialog_width_scale = 1024;

// to full width of screen
DialogStyle DialogStyleSetting = dsFullWidth;

/**
 * Callback type for the "Custom" element, attribute "OnCreate".
 */
typedef Window *(*CreateWindowCallback_t)(ContainerWindow &parent,
                                          int left, int top,
                                          unsigned width, unsigned height,
                                          const WindowStyle style);

static void
LoadChildrenFromXML(WndForm &form, ContainerControl &parent,
                    CallBackTableEntry *LookUpTable,
                    XMLNode *Node, const DialogStyle eDialogStyle);

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
  if (String == NULL || string_is_empty(String))
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
  if (String == NULL || string_is_empty(String))
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
  if (String == NULL || string_is_empty(String))
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
static DialogStyle
GetDialogStyle(const XMLNode &xNode)
{
  const TCHAR* popup = xNode.getAttribute(_T("Popup"));
  if ((popup == NULL) || string_is_empty(popup))
    return DialogStyleSetting;
  else
    return (DialogStyle)StringToIntDflt(popup, 0);
}

static int 
ScaleWidth(const int x, const DialogStyle eDialogStyle) 
{
  if (eDialogStyle == dsFullWidth)
    // stretch width to fill screen horizontally
    return x * dialog_width_scale / 1024;
  else
    return x;
}

static const TCHAR*
GetName(const XMLNode &Node)
{
  return StringToStringDflt(Node.getAttribute(_T("Name")), _T(""));
}

static const TCHAR*
GetCaption(const XMLNode &Node)
{
  const TCHAR* tmp =
      StringToStringDflt(Node.getAttribute(_T("Caption")), _T(""));

  const TCHAR *translated = gettext(tmp);
  if (translated != tmp)
    return translated;

  return tmp;
}

static POINT
GetPosition(const XMLNode &Node, const RECT rc)
{
  POINT pt;

  // Calculate x- and y-Coordinate
  pt.x = StringToIntDflt(Node.getAttribute(_T("X")), 0);
  pt.y = StringToIntDflt(Node.getAttribute(_T("Y")), -1);

  if (Layout::ScaleSupported()) {
    pt.x = Layout::Scale(pt.x);
    if (pt.y != -1)
      pt.y = Layout::Scale(pt.y);
  }

  if (pt.x < -1)
    pt.x += rc.right;
  if (pt.y < -1)
    pt.y += rc.bottom;

  return pt;
}

static POINT
SetPositionCentered(const POINT original, const RECT rc, const SIZE size)
{
  POINT pt = original;
  // center horizontally in parent RECT
  pt.x = (rc.right + rc.left - size.cx) / 2;
  return pt;
}

static SIZE
GetSize(const XMLNode &Node, const RECT rc, const POINT pos)
{
  SIZE sz;

  // Calculate width and height
  sz.cx = StringToIntDflt(Node.getAttribute(_T("Width")), 0);
  sz.cy = StringToIntDflt(Node.getAttribute(_T("Height")), 0);

  if (Layout::ScaleSupported()) {
    sz.cx = Layout::Scale(sz.cx);
    sz.cy = Layout::Scale(sz.cy);
  }

  if (sz.cx <= 0)
    sz.cx += rc.right - pos.x;
  if (sz.cy <= 0)
    sz.cy += rc.bottom - pos.y;

  return sz;
}

static void *
CallBackLookup(CallBackTableEntry *LookUpTable, const TCHAR *Name)
{
  if (LookUpTable != NULL && Name != NULL && !string_is_empty(Name))
    for (unsigned i = 0; LookUpTable[i].Ptr != NULL; i++)
      if (_tcscmp(LookUpTable[i].Name, Name) == 0)
        return LookUpTable[i].Ptr;

  return NULL;
}

static void *
GetCallBack(CallBackTableEntry *LookUpTable,
            const XMLNode &node, const TCHAR* attribute)
{
  return CallBackLookup(LookUpTable,
                        StringToStringDflt(node.getAttribute(attribute), NULL));
}

static void
ShowXMLError(const TCHAR* msg = _T("Error in loading XML dialog"))
{
  MessageBoxX(msg, _T("Dialog error"),
              MB_OK | MB_ICONEXCLAMATION);
}

static XMLNode
xmlLoadFromResource(const TCHAR* lpName, XMLResults *pResults)
{
  ResourceLoader::Data data = ResourceLoader::Load(lpName, _T("XMLDialog"));
  if (data.first == NULL) {
    ShowXMLError(_T("Can't find resource"));

    // unable to find the resource
    return XMLNode::emptyXMLNode;
  }

  const char *buffer = (const char *)data.first;

#ifdef _UNICODE
  int length = data.second;
  TCHAR *buffer2 = new TCHAR[length + 1];
  length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buffer, length,
                               buffer2, length);
  buffer2[length] = _T('\0');
#else
  const char *buffer2 = buffer;
#endif

  XMLNode x = XMLNode::parseString(buffer2, pResults);

#ifdef _UNICODE
  delete[] buffer2;
#endif

  return x;
}

/**
 * Tries to load an XML file from the resources
 * @param lpszXML The resource name
 * @return The parsed XMLNode
 */
static XMLNode
xmlOpenResourceHelper(const TCHAR *resource)
{
  XMLResults pResults;

  // Reset errors
  pResults.error = eXMLErrorNone;
  XMLNode::GlobalError = false;

  // Load and parse the resource
  XMLNode xnode = xmlLoadFromResource(resource, &pResults);

  // Show errors if they exist
  if (pResults.error != eXMLErrorNone) {
    XMLNode::GlobalError = true;
    TCHAR errortext[100];
    _stprintf(errortext,_T("%s %i %i"), XMLNode::getError(pResults.error),
              pResults.nLine, pResults.nColumn);

    ShowXMLError(errortext);
  }

  return xnode;
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
InitScaleWidth(const SIZE size, const RECT rc, const DialogStyle eDialogStyle)
{
  // No need to calculate the scale factor on platforms that don't scale
  if (!Layout::ScaleSupported())
    return;

  if (eDialogStyle == dsFullWidth)
    dialog_width_scale = (rc.right - rc.left) * 1024 / size.cx;
  else
    dialog_width_scale = 1024;
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
LoadDialog(CallBackTableEntry *LookUpTable, SingleWindow &Parent,
               const TCHAR* resource)
{
  WndForm *form = NULL;

  // Find XML file or resource and load XML data out of it
  XMLNode node = xmlOpenResourceHelper(resource);

  // TODO code: put in error checking here and get rid of exits in xmlParser
  // If XML error occurred -> Error messagebox + cancel
  if (node.isEmpty()) {
    ShowXMLError();
    return NULL;
  }

  // If the main XMLNode is of type "Form"
  if (_tcsicmp(node.getName(), _T("Form")) != 0)
    // Get the first child node of the type "Form"
    // and save it as the dialog node
    node = node.getChildNode(_T("Form"));

  // If Node does not exists -> Error messagebox + cancel
  if (node.isEmpty()) {
    ShowXMLError();
    return NULL;
  }

  // Determine the dialog style of the dialog
  DialogStyle dialog_style = GetDialogStyle(node);

  // Determine the dialog size
  const TCHAR* Caption = GetCaption(node);
  const RECT rc = Parent.get_client_rect();
  POINT pos = GetPosition(node, rc);
  SIZE size = GetSize(node, rc, pos);

  InitScaleWidth(size, rc, dialog_style);

  // Correct dialog size and position for dialog style
  switch (dialog_style) {
  case dsFullWidth:
    pos.x = rc.left;
    pos.y = rc.top;
    size.cx = rc.right - rc.left; // stretch form to full width of screen
    size.cy = rc.bottom - rc.top;
    break;
  case dsScaledCentered:
    pos = SetPositionCentered(pos, rc, size);
    break;
  }

  // Create the dialog
  WindowStyle style;
  style.hide();
  style.control_parent();

  form = new WndForm(Parent, pos.x, pos.y, size.cx, size.cy, Caption, style);

  // Set fore- and background colors
  LoadColors(*form, node);

  // Load the children controls
  LoadChildrenFromXML(*form, *form, LookUpTable, &node, dialog_style);

  // If XML error occurred -> Error messagebox + cancel
  if (XMLNode::GlobalError) {
    ShowXMLError();
    delete form;
    return NULL;
  }

  // Return the created form
  return form;
}

static DataField *
LoadDataField(const XMLNode &node, CallBackTableEntry *LookUpTable,
              const DialogStyle eDialogStyle)
{
  TCHAR DataType[32];
  TCHAR DisplayFmt[32];
  TCHAR EditFormat[32];
  double Step;
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

  fixed Min = fixed(StringToFloatDflt(node.getAttribute(_T("Min")), INT_MIN));
  fixed Max = fixed(StringToFloatDflt(node.getAttribute(_T("Max")), INT_MAX));
  Step = StringToFloatDflt(node.getAttribute(_T("Step")), 1);
  Fine = StringToIntDflt(node.getAttribute(_T("Fine")), 0);

  DataField::DataAccessCallback_t callback = (DataField::DataAccessCallback_t)
    CallBackLookup(LookUpTable,
                   StringToStringDflt(node.getAttribute(_T("OnDataAccess")),
                                      NULL));

  if (_tcsicmp(DataType, _T("enum")) == 0)
    return new DataFieldEnum(false, callback);

  if (_tcsicmp(DataType, _T("filereader")) == 0)
    return new DataFieldFileReader(callback);

  if (_tcsicmp(DataType, _T("boolean")) == 0)
    return new DataFieldBoolean(false, _T("ON"), _T("OFF"), callback);

  if (_tcsicmp(DataType, _T("double")) == 0)
    return new DataFieldFloat(EditFormat, DisplayFmt, Min, Max,
                              fixed_zero, fixed(Step), fixed(Fine),
                              callback);

  if (_tcsicmp(DataType, _T("integer")) == 0)
    return new DataFieldInteger(EditFormat, DisplayFmt, Min, Max,
                                0, (int)Step, callback);

  if (_tcsicmp(DataType, _T("string")) == 0)
    return new DataFieldString(_T(""), callback);

  return NULL;
}

/**
 * Creates a control from the given XMLNode as a child of the given parent
 * ContainerControl.
 *
 * @param form the WndForm object
 * @param Parent The parent ContainerControl
 * @param LookUpTable The parent CallBackTable
 * @param node The XMLNode that represents the control
 * @param eDialogStyle The parent's dialog style
 */
static Window *
LoadChild(WndForm &form, ContainerControl &Parent,
          CallBackTableEntry *LookUpTable,
          XMLNode node, const DialogStyle eDialogStyle)
{
  Window *window = NULL;

  // Determine name, coordinates, width, height
  // and caption of the control
  const TCHAR* Name = GetName(node);
  const TCHAR* Caption = GetCaption(node);
  RECT rc = Parent.GetClientAreaWindow().get_client_rect();
  POINT pos = GetPosition(node, rc);
  pos.x = ScaleWidth(pos.x, eDialogStyle);
  SIZE size = GetSize(node, rc, pos);
  size.cx = ScaleWidth(size.cx, eDialogStyle);

  WindowStyle style;

  if (!StringToIntDflt(node.getAttribute(_T("Visible")), 1))
    style.hide();

  if (StringToIntDflt(node.getAttribute(_T("Border")), 0))
    style.border();

  bool advanced = _tcschr(Caption, _T('*')) != NULL;

  // PropertyControl (WndProperty)
  if (_tcscmp(node.getName(), _T("Edit")) == 0) {
    WndProperty *W;
    int CaptionWidth;
    bool ReadOnly;
    bool MultiLine;

    // Determine the width of the caption field
    CaptionWidth = StringToIntDflt(node.getAttribute(_T("CaptionWidth")), 0);

    if (Layout::ScaleSupported())
      CaptionWidth = Layout::Scale(CaptionWidth);

    CaptionWidth = ScaleWidth(CaptionWidth, eDialogStyle);

    // Determine whether the control is multiline or readonly
    MultiLine = StringToIntDflt(node.getAttribute(_T("MultiLine")), 0);
    ReadOnly = StringToIntDflt(node.getAttribute(_T("ReadOnly")), 0);

    // Load the event callback properties
    WndProperty::DataChangeCallback_t DataNotifyCallback =
      (WndProperty::DataChangeCallback_t)
      GetCallBack(LookUpTable, node, _T("OnDataNotify"));

    WindowControl::OnHelpCallback_t OnHelpCallback =
      (WindowControl::OnHelpCallback_t)
      GetCallBack(LookUpTable, node, _T("OnHelp"));

    // Create the Property Control
    style.control_parent();

    EditWindowStyle edit_style;
    if (ReadOnly)
      edit_style.read_only();
    else
      edit_style.tab_stop();

    if (is_embedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      edit_style.border();
    else
      edit_style.sunken_edge();

    if (MultiLine) {
      edit_style.multiline();
      edit_style.vscroll();
    }

    window = W = new WndProperty(Parent, Caption, pos.x, pos.y, size.cx, size.cy,
                                 CaptionWidth, style, edit_style,
                                 DataNotifyCallback);

    // Set the fore- and background color
    LoadColors(*W, node);

    // Set the help function event callback
    W->SetOnHelpCallback(OnHelpCallback);

    // Load the help text
    W->SetHelpText(StringToStringDflt(node.getAttribute(_T("Help")), _T("")));

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
  } else if (_tcscmp(node.getName(), _T("Button")) == 0) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback_t ClickCallback =
      (WndButton::ClickNotifyCallback_t)
      GetCallBack(LookUpTable, node, _T("OnClick"));

    // Create the ButtonControl

    style.tab_stop();

    window = new WndButton(Parent.GetClientAreaWindow(), Caption,
                           pos.x, pos.y, size.cx, size.cy,
                           style,
                           ClickCallback);

  } else if (_tcscmp(node.getName(), _T("CheckBox")) == 0) {
    // Determine ClickCallback function
    CheckBoxControl::ClickNotifyCallback_t ClickCallback =
      (CheckBoxControl::ClickNotifyCallback_t)
      GetCallBack(LookUpTable, node, _T("OnClick"));

    // Create the CheckBoxControl

    style.tab_stop();

    window = new CheckBoxControl(Parent.GetClientAreaWindow(), Caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 style,
                                 ClickCallback);

  // SymbolButtonControl (WndSymbolButton) not used yet
  } else if (_tcscmp(node.getName(), _T("SymbolButton")) == 0) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback_t ClickCallback =
      (WndButton::ClickNotifyCallback_t)
      GetCallBack(LookUpTable, node, _T("OnClick"));

    // Create the SymbolButtonControl

    style.tab_stop();

    window = new WndSymbolButton(Parent.GetClientAreaWindow(), Caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 style, Parent.GetBackColor(),
                                 ClickCallback);

  // PanelControl (WndPanel)
  } else if (_tcscmp(node.getName(), _T("Panel")) == 0) {
    // Create the PanelControl

    style.control_parent();

    PanelControl *frame = new PanelControl(Parent, pos.x, pos.y, size.cx, size.cy, style);

    // Set the fore- and background color
    LoadColors(*frame, node);

    window = frame;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(form, *frame, LookUpTable, &node, eDialogStyle);

  // KeyboardControl
  } else if (_tcscmp(node.getName(), _T("Keyboard")) == 0) {
    KeyboardControl::OnCharacterCallback_t CharacterCallback =
      (KeyboardControl::OnCharacterCallback_t)
      GetCallBack(LookUpTable, node, _T("OnCharacter"));

    // Create the KeyboardControl
    KeyboardControl *kb =
      new KeyboardControl(Parent.GetClientAreaWindow(),
                          pos.x, pos.y, size.cx, size.cy, Parent.GetBackColor(),
                          CharacterCallback, style);

    window = kb;
  // DrawControl (WndOwnerDrawFrame)
  } else if (_tcscmp(node.getName(), _T("Canvas")) == 0) {
    // Determine DrawCallback function
    WndOwnerDrawFrame::OnPaintCallback_t PaintCallback =
      (WndOwnerDrawFrame::OnPaintCallback_t)
      GetCallBack(LookUpTable, node, _T("OnPaint"));

    // Create the DrawControl
    WndOwnerDrawFrame* canvas =
        new WndOwnerDrawFrame(Parent, pos.x, pos.y, size.cx, size.cy,
                              style, PaintCallback);

    // Set the fore- and background color
    LoadColors(*canvas, node);

    window = canvas;

  // FrameControl (WndFrame)
  } else if (_tcscmp(node.getName(), _T("Label")) == 0){
    // Create the FrameControl
    WndFrame* frame = new WndFrame(Parent, pos.x, pos.y, size.cx, size.cy, style);

    // Set the fore- and background color
    LoadColors(*frame, node);

    // Set the caption
    frame->SetCaption(Caption);

    window = frame;

  // ListBoxControl (WndListFrame)
  } else if (_tcscmp(node.getName(), _T("List")) == 0){
    // Determine ItemHeight of the list items
    unsigned item_height =
      Layout::Scale(StringToIntDflt(node.getAttribute(_T("ItemHeight")), 18));

    // Create the ListBoxControl

    style.tab_stop();

    if (is_embedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      style.border();
    else
      style.sunken_edge();

    window = new WndListFrame(Parent.GetClientAreaWindow(),
                              pos.x, pos.y, size.cx, size.cy,
                              style,
                              item_height);

  // TabControl (Tabbed)
  } else if (_tcscmp(node.getName(), _T("Tabbed")) == 0) {
    // Create the TabControl

    style.control_parent();

    TabbedControl *tabbed = new TabbedControl(Parent,
                                              pos.x, pos.y, size.cx, size.cy, style);

    // Set the fore- and background color
    LoadColors(*tabbed, node);

    window = tabbed;

    const unsigned n = node.nChildNode();
    for (unsigned i = 0; i < n; ++i) {
      // Load each child control from the child nodes
      Window *child = LoadChild(form, *tabbed, LookUpTable,
                                node.getChildNode(i), eDialogStyle);
      if (child != NULL)
        tabbed->AddClient(child);
        continue;
    }
  } else if (_tcscmp(node.getName(), _T("Custom")) == 0) {
    // Create a custom Window object with a callback
    CreateWindowCallback_t create =
        (CreateWindowCallback_t)GetCallBack(LookUpTable, node, _T("OnCreate"));
    if (create == NULL)
      return NULL;

    window = create(Parent.GetClientAreaWindow(),
                    pos.x, pos.y, size.cx, size.cy, style);
  }

  if (window != NULL) {
    if (!string_is_empty(Name))
      form.AddNamed(Name, window);

    if (advanced)
      form.AddAdvanced(window);

    form.AddDestruct(window);
  }

  return window;
}

/**
 * Loads the Parent's children Controls from the given XMLNode
 *
 * @param form the WndForm object
 * @param Parent The parent control
 * @param LookUpTable The parents CallBackTable
 * @param Node The XMLNode that represents the parent control
 * @param eDialogStyle The parent's dialog style
 */
static void
LoadChildrenFromXML(WndForm &form, ContainerControl &Parent,
                    CallBackTableEntry *LookUpTable,
                    XMLNode *Node, const DialogStyle eDialogStyle)
{
  // Get the number of childnodes
  int Count = Node->nChildNode();

  unsigned bottom_most = 0;

  // Iterate through the childnodes
  for (int i = 0; i < Count; i++) {
    // Load each child control from the child nodes
    Window *window = LoadChild(form, Parent, LookUpTable,
                               Node->getChildNode(i), eDialogStyle);
    if (window == NULL)
      continue;

    // If the client doesn't know where to go
    // -> move it below the previous one
    if (window->get_position().top == -1)
      window->move(window->get_position().left, bottom_most);

    bottom_most = window->get_position().bottom;
  }
}
