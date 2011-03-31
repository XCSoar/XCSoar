/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "xmlParser.hpp"
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
#include "Form/TabBar.hpp"
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

// used when loading stand-alone XML file with no form control
static DialogStyle dialog_style_last = dsFullWidth;

struct ControlSize: public PixelSize
{
  bool no_scaling;
};

struct ControlPosition: public RasterPoint
{
  bool no_scaling;
};

/**
 * Callback type for the "Custom" element, attribute "OnCreate".
 */
typedef Window *(*CreateWindowCallback_t)(ContainerWindow &parent,
                                          int left, int top,
                                          unsigned width, unsigned height,
                                          const WindowStyle style);

static Window *
LoadChild(WndForm &form, ContainerWindow &parent, Color background_color,
          CallBackTableEntry *LookUpTable,
          XMLNode node, const DialogStyle eDialogStyle,
          int bottom_most=0);

static void
LoadChildrenFromXML(WndForm &form, ContainerWindow &parent,
                    Color background_color,
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
  if (eDialogStyle == dsFullWidth || eDialogStyle == dsScaledBottom)
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

  // don't translate empty strings, it would query gettext metadata
  if (tmp[0] == _T('\0'))
    return tmp;

  size_t length = _tcslen(tmp);
  assert(length > 0);
  if (length < 256 && tmp[length - 1] == _T('*')) {
    /* remove the trailing asterisk before the translation, and
       re-append it to the translated string */
    static TCHAR buffer[256];
    _tcscpy(buffer, tmp);
    buffer[length - 1] = _T('\0');
    const TCHAR *translated = gettext(buffer);
    if (translated == buffer)
      return tmp;

    length = _tcslen(translated);
    if (length >= 255)
      return translated;

    _tcscpy(buffer, translated);
    _tcscat(buffer, _T("*"));
    return buffer;
  }

  return gettext(tmp);
}

static ControlPosition
GetPosition(const XMLNode &Node, const PixelRect rc, int bottom_most=-1)
{
  ControlPosition pt;

  // Calculate x- and y-Coordinate
  pt.x = StringToIntDflt(Node.getAttribute(_T("X")), 0);
  pt.y = StringToIntDflt(Node.getAttribute(_T("Y")), -1);
  pt.no_scaling = false;

  if (Layout::ScaleSupported()) {
    pt.x = Layout::Scale(pt.x);
    if (pt.y != -1)
      pt.y = Layout::Scale(pt.y);
  }

  if (pt.y == -1)
    pt.y = bottom_most;

  if (pt.x < -1) {
    pt.x += rc.right;
    pt.no_scaling = true;
  }
  if (pt.y < -1)
    pt.y += rc.bottom;

  // Move inside target rc (e.g. if parent != targetRect -> usually, rc.left == rc.top == 0, so no moving takes place).
  pt.x += rc.left;
  pt.y += rc.top;

  return pt;
}

static ControlPosition
SetPositionCentered(const ControlPosition original, const PixelRect rc,
                    const ControlSize size)
{
  ControlPosition pt = original;
  // center horizontally in parent PixelRect
  pt.x = (rc.right + rc.left - size.cx) / 2;
  return pt;
}

static ControlSize
GetSize(const XMLNode &Node, const PixelRect rc, const RasterPoint &pos)
{
  ControlSize sz;

  // Calculate width and height
  sz.cx = StringToIntDflt(Node.getAttribute(_T("Width")), 0);
  sz.cy = StringToIntDflt(Node.getAttribute(_T("Height")), 0);
  sz.no_scaling = false;

  if (Layout::ScaleSupported()) {
    sz.cx = Layout::Scale(sz.cx);
    sz.cy = Layout::Scale(sz.cy);
  }

  if (sz.cx <= 0) {
    sz.cx += rc.right - pos.x;
    sz.no_scaling = true;
  }
  if (sz.cy <= 0)
    sz.cy += rc.bottom - pos.y;

  assert(sz.cx > 0);
  assert(sz.cy > 0);

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
InitScaleWidth(const PixelSize size, const PixelRect rc,
               const DialogStyle eDialogStyle)
{
  // No need to calculate the scale factor on platforms that don't scale
  if (!Layout::ScaleSupported())
    return;

  if (eDialogStyle == dsFullWidth || eDialogStyle == dsScaledBottom)
    dialog_width_scale = (rc.right - rc.left) * 1024 / size.cx;
  else
    dialog_width_scale = 1024;
}

/**
 * Loads a stand-alone XML file as a single top-level XML node
 * into an existing WndForm object and sets its parent to the parent parameter
 * Ignores additional top-level XML nodes.
 * Scales based on the DialogStyle of the last XML form loaded by XCSoar.
 * The Window is destroyed by its Form's destructor
 *
 * @param LookUpTable The CallBackTable
 * @param form The WndForm into which the Window is added
 * @param parent The parent window of the control being created
 *    set parent to "form-get_client_rect()" to make top level control
 *    or to a PanelControl to add it to a tab window
 * @param FileName The XML filename
 * @return the pointer to the Window added to the form
 */
Window *
LoadWindow(CallBackTableEntry *LookUpTable,
              WndForm *form, ContainerWindow &parent,
              const TCHAR* resource)
{
  if (!form)
    return NULL;

  XMLNode node = xmlOpenResourceHelper(resource);

  if (node.isEmpty()) {
    ShowXMLError();
    return NULL;
  }

  if (node.isEmpty()) {
    ShowXMLError();
    return NULL;
  }

  // use style of last form loaded
  DialogStyle dialog_style = dialog_style_last;

  // load only one top-level control.
  Window *window = LoadChild(*form, parent, form->GetBackColor(), LookUpTable,
                             node, dialog_style,
                             0);

  if (XMLNode::GlobalError) {
    ShowXMLError();
    delete form;
    return NULL;
  }
  return window;
}

/**
 * This function returns a WndForm created either from the ressources or
 * from the XML file in XCSoarData(if found)
 * @param LookUpTable The CallBackTable
 * @param FileName The XML filename to search for in XCSoarData
 * @param Parent The parent window (e.g. XCSoarInterface::main_window)
 * @param resource The resource to look for
 * @param targetRect The area where to move the dialog if not parent
 * @return The WndForm object
 */
WndForm *
LoadDialog(CallBackTableEntry *LookUpTable, SingleWindow &Parent,
               const TCHAR* resource, const PixelRect *targetRect)
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
  dialog_style_last = dialog_style;

  // Determine the dialog size
  const TCHAR* Caption = GetCaption(node);
  const PixelRect rc = targetRect ? *targetRect : Parent.get_client_rect();
  ControlPosition pos = GetPosition(node, rc, 0);
  ControlSize size = GetSize(node, rc, pos);

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

  case dsScaled:
  case dsFixed:
    break;

  case dsScaledBottom:
    size.cx = rc.right - rc.left; // stretch form to full width of screen
    pos.y = rc.bottom - size.cy; // position at bottom of screen
    break;
  }

  // Create the dialog
  WindowStyle style;
  style.hide();
  style.control_parent();

  form = new WndForm(Parent, pos.x, pos.y, size.cx, size.cy, Caption, style);

  // Set fore- and background colors
  Color color;
  if (StringToColor(node.getAttribute(_T("BackColor")), color))
    form->SetBackColor(color);

  // Load the children controls
  LoadChildrenFromXML(*form, form->GetClientAreaWindow(), form->GetBackColor(),
                      LookUpTable, &node, dialog_style);

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
    return new DataFieldEnum(callback);

  if (_tcsicmp(DataType, _T("filereader")) == 0)
    return new DataFieldFileReader(callback);

  if (_tcsicmp(DataType, _T("boolean")) == 0)
    return new DataFieldBoolean(false, _("On"), _("Off"), callback);

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
 * Creates a control from the given XMLNode as a child of the given
 * parent.
 *
 * @param form the WndForm object
 * @param LookUpTable The parent CallBackTable
 * @param node The XMLNode that represents the control
 * @param eDialogStyle The parent's dialog style
 */
static Window *
LoadChild(WndForm &form, ContainerWindow &parent, Color background_color,
          CallBackTableEntry *LookUpTable,
          XMLNode node, const DialogStyle eDialogStyle,
          int bottom_most)
{
  Window *window = NULL;

  // Determine name, coordinates, width, height
  // and caption of the control
  const TCHAR* Name = GetName(node);
  const TCHAR* Caption = GetCaption(node);
  PixelRect rc = parent.get_client_rect();
  ControlPosition pos = GetPosition(node, rc, bottom_most);
  if (!pos.no_scaling)
    pos.x = ScaleWidth(pos.x, eDialogStyle);

  ControlSize size = GetSize(node, rc, pos);
  if (!size.no_scaling)
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

    window = W = new WndProperty(parent, Caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 CaptionWidth, background_color,
                                 style, edit_style,
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
        LoadDataField(node.getChildNode(_T("DataField"), 0u),
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

    ButtonWindowStyle bstyle(style);
    bstyle.tab_stop();
    bstyle.multiline();

    window = new WndButton(parent, Caption,
                           pos.x, pos.y, size.cx, size.cy,
                           bstyle, ClickCallback);

  } else if (_tcscmp(node.getName(), _T("CheckBox")) == 0) {
    // Determine ClickCallback function
    CheckBoxControl::ClickNotifyCallback_t ClickCallback =
      (CheckBoxControl::ClickNotifyCallback_t)
      GetCallBack(LookUpTable, node, _T("OnClick"));

    // Create the CheckBoxControl

    style.tab_stop();

    window = new CheckBoxControl(parent, Caption,
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

    window = new WndSymbolButton(parent, Caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 style, background_color,
                                 ClickCallback);

  // PanelControl (WndPanel)
  } else if (_tcscmp(node.getName(), _T("Panel")) == 0) {
    // Create the PanelControl

    style.control_parent();

    PanelControl *frame = new PanelControl(parent,
                                           pos.x, pos.y, size.cx, size.cy,
                                           background_color, style);

    window = frame;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(form, *frame, background_color,
                        LookUpTable, &node, eDialogStyle);

  // KeyboardControl
  } else if (_tcscmp(node.getName(), _T("Keyboard")) == 0) {
    KeyboardControl::OnCharacterCallback_t CharacterCallback =
      (KeyboardControl::OnCharacterCallback_t)
      GetCallBack(LookUpTable, node, _T("OnCharacter"));

    // Create the KeyboardControl
    KeyboardControl *kb =
      new KeyboardControl(parent,
                          pos.x, pos.y, size.cx, size.cy, background_color,
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
      new WndOwnerDrawFrame(parent,
                            pos.x, pos.y, size.cx, size.cy,
                              style, PaintCallback);

    window = canvas;

  // FrameControl (WndFrame)
  } else if (_tcscmp(node.getName(), _T("Label")) == 0){
    // Create the FrameControl
    WndFrame* frame = new WndFrame(parent,
                                   pos.x, pos.y, size.cx, size.cy,
                                   background_color, style);

    // Set the caption
    frame->SetCaption(Caption);
    // Set caption color
    Color color;
    if (StringToColor(node.getAttribute(_T("CaptionColor")), color))
      frame->SetCaptionColor(color);

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

    window = new WndListFrame(parent,
                              pos.x, pos.y, size.cx, size.cy,
                              style,
                              item_height);

  // TabControl (Tabbed)
  } else if (_tcscmp(node.getName(), _T("Tabbed")) == 0) {
    // Create the TabControl

    style.control_parent();

    TabbedControl *tabbed = new TabbedControl(parent,
                                              pos.x, pos.y, size.cx, size.cy,
                                              style);

    window = tabbed;

    const unsigned n = node.nChildNode();
    for (unsigned i = 0; i < n; ++i) {
      // Load each child control from the child nodes
      Window *child = LoadChild(form, *tabbed, background_color,
                                LookUpTable,
                                node.getChildNode(i), eDialogStyle);
      if (child != NULL)
        tabbed->AddClient(child);
        continue;
    }
  // TabBarControl (TabBar)
  } else if (_tcscmp(node.getName(), _T("TabBar")) == 0) {
    // Create the TabBarControl

    bool flipOrientation = false;
    if ( (Layout::landscape && StringToIntDflt(node.getAttribute(_T("Horizontal")), 0)) ||
         (!Layout::landscape && StringToIntDflt(node.getAttribute(_T("Vertical")), 0) ) )
      flipOrientation = true;

    style.control_parent();
    TabBarControl *tabbar = new TabBarControl(parent,
                                              pos.x, pos.y, size.cx, size.cy,
                                              style, flipOrientation);
    window = tabbar;

  } else if (_tcscmp(node.getName(), _T("Custom")) == 0) {
    // Create a custom Window object with a callback
    CreateWindowCallback_t create =
        (CreateWindowCallback_t)GetCallBack(LookUpTable, node, _T("OnCreate"));
    if (create == NULL)
      return NULL;

    window = create(parent,
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
LoadChildrenFromXML(WndForm &form, ContainerWindow &parent,
                    Color background_color,
                    CallBackTableEntry *LookUpTable,
                    XMLNode *Node, const DialogStyle eDialogStyle)
{
  // Get the number of childnodes
  int Count = Node->nChildNode();

  unsigned bottom_most = 0;

  // Iterate through the childnodes
  for (int i = 0; i < Count; i++) {
    // Load each child control from the child nodes
    Window *window = LoadChild(form, parent, background_color, LookUpTable,
                               Node->getChildNode(i), eDialogStyle,
                               bottom_most);
    if (window == NULL)
      continue;

    bottom_most = window->get_position().bottom;
  }
}
