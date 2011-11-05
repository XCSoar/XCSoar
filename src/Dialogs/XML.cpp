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
#include "Language/Language.hpp"
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
#include "Form/TabMenu.hpp"
#include "Form/Panel.hpp"
#include "Form/Keyboard.hpp"
#include "Form/CheckBox.hpp"
#include "StringUtil.hpp"
#include "ResourceLoader.hpp"

#include <stdio.h>    // for _stprintf
#include <assert.h>
#include <tchar.h>
#include <limits.h>

static const DialogLook *xml_dialog_look;

void
SetXMLDialogLook(const DialogLook &_dialog_look)
{
  xml_dialog_look = &_dialog_look;
}

// used when stretching dialog and components
static int dialog_width_scale = 1024;

// to full width of screen
DialogStyle dialog_style_setting = dsFullWidth;

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
                                          PixelScalar left, PixelScalar top,
                                          UPixelScalar width,
                                          UPixelScalar height,
                                          const WindowStyle style);

static Window *
LoadChild(WndForm &form, ContainerWindow &parent,
          const CallBackTableEntry *lookup_table, XMLNode node,
          const DialogStyle dialog_style, int bottom_most = 0);

static void
LoadChildrenFromXML(WndForm &form, ContainerWindow &parent,
                    const CallBackTableEntry *lookup_table,
                    const XMLNode *node,
                    const DialogStyle dialog_style);

/**
 * Converts a String into an Integer and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Integer value
 */
static long
StringToIntDflt(const TCHAR *string, long _default)
{
  if (string == NULL || string_is_empty(string))
    return _default;
  return _tcstol(string, NULL, 0);
}

/**
 * Converts a String into a Float and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Float value
 */
static double
StringToFloatDflt(const TCHAR *string, double _default)
{
  if (string == NULL || string_is_empty(string))
    return _default;
  return _tcstod(string, NULL);
}

/**
 * Returns the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The output String
 */
static const TCHAR *
StringToStringDflt(const TCHAR *string, const TCHAR *_default)
{
  if (string == NULL || string_is_empty(string))
    return _default;
  return string;
}

/**
 * Converts a String into a Color and sets
 * a default value if String = NULL
 * @param String The String to parse
 * @param color The color (output)
 */
static bool
StringToColor(const TCHAR *string, Color &color)
{
  long value = StringToIntDflt(string, -1);
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
GetDialogStyle(const XMLNode &node)
{
  const TCHAR* popup = node.getAttribute(_T("Popup"));
  if ((popup == NULL) || string_is_empty(popup))
    return dialog_style_setting;
  else
    return (DialogStyle)StringToIntDflt(popup, 0);
}

static int 
ScaleWidth(const int x, const DialogStyle dialog_style)
{
  if (dialog_style == dsFullWidth || dialog_style == dsScaledBottom)
    // stretch width to fill screen horizontally
    return x * dialog_width_scale / 1024;
  else
    return x;
}

static const TCHAR*
GetName(const XMLNode &node)
{
  return StringToStringDflt(node.getAttribute(_T("Name")), _T(""));
}

static const TCHAR*
GetCaption(const XMLNode &node)
{
  const TCHAR* tmp =
      StringToStringDflt(node.getAttribute(_T("Caption")), _T(""));

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
GetPosition(const XMLNode &node, const PixelRect rc, int bottom_most = -1)
{
  ControlPosition pt;

  // Calculate x- and y-Coordinate
  pt.x = StringToIntDflt(node.getAttribute(_T("X")), 0);
  pt.y = StringToIntDflt(node.getAttribute(_T("Y")), -1);
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
GetSize(const XMLNode &node, const PixelRect rc, const RasterPoint &pos)
{
  ControlSize sz;

  // Calculate width and height
  sz.cx = StringToIntDflt(node.getAttribute(_T("Width")), 0);
  sz.cy = StringToIntDflt(node.getAttribute(_T("Height")), 0);
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
CallBackLookup(const CallBackTableEntry *lookup_table, const TCHAR *name)
{
  if (lookup_table != NULL && name != NULL && !string_is_empty(name))
    for (unsigned i = 0; lookup_table[i].callback != NULL; i++)
      if (_tcscmp(lookup_table[i].name, name) == 0)
        return lookup_table[i].callback;

  return NULL;
}

static void *
GetCallBack(const CallBackTableEntry *lookup_table,
            const XMLNode &node, const TCHAR* attribute)
{
  return CallBackLookup(lookup_table,
                        StringToStringDflt(node.getAttribute(attribute), NULL));
}

static XMLNode *
LoadXMLFromResource(const TCHAR* resource, XMLResults *xml_results)
{
  ResourceLoader::Data data = ResourceLoader::Load(resource, _T("XMLDialog"));
  assert(data.first != NULL);

  const char *buffer = (const char *)data.first;

#ifdef _UNICODE
  int length = data.second;
  TCHAR *buffer2 = new TCHAR[length + 1];
  length = MultiByteToWideChar(CP_UTF8, 0, buffer, length,
                               buffer2, length);
  buffer2[length] = _T('\0');
#else
  const char *buffer2 = buffer;
#endif

  XMLNode *x = XMLNode::parseString(buffer2, xml_results);

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
static XMLNode *
LoadXMLFromResource(const TCHAR *resource)
{
  XMLResults xml_results;

  // Reset errors
  xml_results.error = eXMLErrorNone;
  XMLNode::GlobalError = false;

  // Load and parse the resource
  XMLNode *node = LoadXMLFromResource(resource, &xml_results);

  // Show errors if they exist
  assert(xml_results.error == eXMLErrorNone);

  return node;
}

static void
InitScaleWidth(const PixelSize size, const PixelRect rc,
               const DialogStyle dialog_style)
{
  // No need to calculate the scale factor on platforms that don't scale
  if (!Layout::ScaleSupported())
    return;

  if (dialog_style == dsFullWidth || dialog_style == dsScaledBottom)
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
LoadWindow(const CallBackTableEntry *lookup_table, WndForm *form,
           ContainerWindow &parent, const TCHAR *resource)
{
  if (!form)
    return NULL;

  XMLNode *node = LoadXMLFromResource(resource);
  assert(node != NULL);

  // use style of last form loaded
  DialogStyle dialog_style = dialog_style_last;

  // load only one top-level control.
  Window *window = LoadChild(*form, parent, lookup_table, *node, dialog_style, 0);
  delete node;

  assert(!XMLNode::GlobalError);

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
LoadDialog(const CallBackTableEntry *lookup_table, SingleWindow &parent,
           const TCHAR *resource, const PixelRect *target_rc)
{
  WndForm *form = NULL;

  // Find XML file or resource and load XML data out of it
  XMLNode *node = LoadXMLFromResource(resource);

  // TODO code: put in error checking here and get rid of exits in xmlParser
  // If XML error occurred -> Error messagebox + cancel
  assert(node != NULL);

  // If the main XMLNode is of type "Form"
  assert(_tcscmp(node->getName(), _T("Form")) == 0);

  // Determine the dialog style of the dialog
  DialogStyle dialog_style = GetDialogStyle(*node);
  dialog_style_last = dialog_style;

  // Determine the dialog size
  const TCHAR* caption = GetCaption(*node);
  const PixelRect rc = target_rc ? *target_rc : parent.get_client_rect();
  ControlPosition pos = GetPosition(*node, rc, 0);
  ControlSize size = GetSize(*node, rc, pos);

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

  form = new WndForm(parent, *xml_dialog_look,
                     pos.x, pos.y, size.cx, size.cy, caption, style);

  // Load the children controls
  LoadChildrenFromXML(*form, form->GetClientAreaWindow(),
                      lookup_table, node, dialog_style);
  delete node;

  // If XML error occurred -> Error messagebox + cancel
  assert(!XMLNode::GlobalError);

  // Return the created form
  return form;
}

static DataField *
LoadDataField(const XMLNode &node, const CallBackTableEntry *LookUpTable,
              const DialogStyle eDialogStyle)
{
  TCHAR data_type[32];
  TCHAR display_format[32];
  TCHAR edit_format[32];
  double step;
  int fine;

  _tcscpy(data_type,
          StringToStringDflt(node.getAttribute(_T("DataType")), _T("")));
  _tcscpy(display_format,
          StringToStringDflt(node. getAttribute(_T("DisplayFormat")), _T("")));
  _tcscpy(edit_format,
          StringToStringDflt(node.getAttribute(_T("EditFormat")), _T("")));

  fixed min = fixed(StringToFloatDflt(node.getAttribute(_T("Min")), INT_MIN));
  fixed max = fixed(StringToFloatDflt(node.getAttribute(_T("Max")), INT_MAX));
  step = StringToFloatDflt(node.getAttribute(_T("Step")), 1);
  fine = StringToIntDflt(node.getAttribute(_T("Fine")), 0);

  DataField::DataAccessCallback_t callback = (DataField::DataAccessCallback_t)
    GetCallBack(LookUpTable, node, _T("OnDataAccess"));

  if (_tcsicmp(data_type, _T("enum")) == 0)
    return new DataFieldEnum(callback);

  if (_tcsicmp(data_type, _T("filereader")) == 0)
    return new DataFieldFileReader(callback);

  if (_tcsicmp(data_type, _T("boolean")) == 0)
    return new DataFieldBoolean(false, _("On"), _("Off"), callback);

  if (_tcsicmp(data_type, _T("double")) == 0)
    return new DataFieldFloat(edit_format, display_format, min, max,
                              fixed_zero, fixed(step), fine, callback);

  if (_tcsicmp(data_type, _T("integer")) == 0)
    return new DataFieldInteger(edit_format, display_format, (int)min, (int)max,
                                0, (int)step, callback);

  if (_tcsicmp(data_type, _T("string")) == 0)
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
LoadChild(WndForm &form, ContainerWindow &parent,
          const CallBackTableEntry *lookup_table, XMLNode node,
          const DialogStyle dialog_style, int bottom_most)
{
  Window *window = NULL;

  // Determine name, coordinates, width, height
  // and caption of the control
  const TCHAR* name = GetName(node);
  const TCHAR* caption = GetCaption(node);
  PixelRect rc = parent.get_client_rect();
  ControlPosition pos = GetPosition(node, rc, bottom_most);
  if (!pos.no_scaling)
    pos.x = ScaleWidth(pos.x, dialog_style);

  ControlSize size = GetSize(node, rc, pos);
  if (!size.no_scaling)
    size.cx = ScaleWidth(size.cx, dialog_style);

  WindowStyle style;

  if (!StringToIntDflt(node.getAttribute(_T("Visible")), 1))
    style.hide();

  if (StringToIntDflt(node.getAttribute(_T("Border")), 0))
    style.border();

  bool expert = (StringToIntDflt(node.getAttribute(_T("Expert")), 0) == 1);

  // PropertyControl (WndProperty)
  if (_tcscmp(node.getName(), _T("Edit")) == 0) {
    WndProperty *property;
    int caption_width;
    bool read_only;
    bool multi_line;

    // Determine the width of the caption field
    caption_width = StringToIntDflt(node.getAttribute(_T("CaptionWidth")), 0);

    if (Layout::ScaleSupported())
      caption_width = Layout::Scale(caption_width);

    caption_width = ScaleWidth(caption_width, dialog_style);

    // Determine whether the control is multiline or readonly
    multi_line = StringToIntDflt(node.getAttribute(_T("MultiLine")), 0);
    read_only = StringToIntDflt(node.getAttribute(_T("ReadOnly")), 0);

    // Load the event callback properties
    WndProperty::DataChangeCallback_t data_notify_callback =
      (WndProperty::DataChangeCallback_t)
      GetCallBack(lookup_table, node, _T("OnDataNotify"));

    WindowControl::OnHelpCallback_t help_callback =
      (WindowControl::OnHelpCallback_t)
      GetCallBack(lookup_table, node, _T("OnHelp"));

    // Create the Property Control
    style.control_parent();

    EditWindowStyle edit_style;
    if (read_only)
      edit_style.read_only();
    else
      edit_style.tab_stop();

    if (is_embedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      edit_style.border();
    else
      edit_style.sunken_edge();

    if (multi_line) {
      edit_style.multiline();
      edit_style.vscroll();
    }

    window = property = new WndProperty(parent, *xml_dialog_look, caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 caption_width,
                                 style, edit_style,
                                 data_notify_callback);

    // Set the help function event callback
    property->SetOnHelpCallback(help_callback);

    // Load the help text
    property->SetHelpText(StringToStringDflt(node.getAttribute(_T("Help")), _T("")));

    // If the control has (at least) one DataField child control
    const XMLNode *data_field_node = node.getChildNode(_T("DataField"), 0u);
    if (data_field_node != NULL) {
      // -> Load the first DataField control
      DataField *data_field =
        LoadDataField(*data_field_node,
                      lookup_table, dialog_style);

      if (data_field != NULL)
        // Tell the Property control about the DataField control
        property->SetDataField(data_field);
    }

  // ButtonControl (WndButton)
  } else if (_tcscmp(node.getName(), _T("Button")) == 0) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback_t click_callback =
      (WndButton::ClickNotifyCallback_t)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the ButtonControl

    ButtonWindowStyle button_style(style);
    button_style.tab_stop();
    button_style.multiline();

    window = new WndButton(parent, *xml_dialog_look, caption,
                           pos.x, pos.y, size.cx, size.cy,
                           button_style, click_callback);

  } else if (_tcscmp(node.getName(), _T("CheckBox")) == 0) {
    // Determine click_callback function
    CheckBoxControl::ClickNotifyCallback_t click_callback =
      (CheckBoxControl::ClickNotifyCallback_t)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the CheckBoxControl

    style.tab_stop();

    window = new CheckBoxControl(parent, *xml_dialog_look, caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 style, click_callback);

  // SymbolButtonControl (WndSymbolButton) not used yet
  } else if (_tcscmp(node.getName(), _T("SymbolButton")) == 0) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback_t click_callback =
      (WndButton::ClickNotifyCallback_t)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the SymbolButtonControl

    style.tab_stop();

    window = new WndSymbolButton(parent, *xml_dialog_look, caption,
                                 pos.x, pos.y, size.cx, size.cy,
                                 style, click_callback);

  // PanelControl (WndPanel)
  } else if (_tcscmp(node.getName(), _T("Panel")) == 0) {
    // Create the PanelControl

    style.control_parent();

    PanelControl *frame = new PanelControl(parent, *xml_dialog_look,
                                           pos.x, pos.y, size.cx, size.cy,
                                           style);

    window = frame;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(form, *frame,
                        lookup_table, &node, dialog_style);

  // KeyboardControl
  } else if (_tcscmp(node.getName(), _T("Keyboard")) == 0) {
    KeyboardControl::OnCharacterCallback_t character_callback =
      (KeyboardControl::OnCharacterCallback_t)
      GetCallBack(lookup_table, node, _T("OnCharacter"));

    // Create the KeyboardControl
    KeyboardControl *kb =
      new KeyboardControl(parent, *xml_dialog_look,
                          pos.x, pos.y, size.cx, size.cy,
                          character_callback, style);

    window = kb;
  // DrawControl (WndOwnerDrawFrame)
  } else if (_tcscmp(node.getName(), _T("Canvas")) == 0) {
    // Determine DrawCallback function
    WndOwnerDrawFrame::OnPaintCallback_t paint_callback =
      (WndOwnerDrawFrame::OnPaintCallback_t)
      GetCallBack(lookup_table, node, _T("OnPaint"));

    // Create the DrawControl
    WndOwnerDrawFrame* canvas =
      new WndOwnerDrawFrame(parent, pos.x, pos.y, size.cx, size.cy,
                            style, paint_callback);

    window = canvas;

  // FrameControl (WndFrame)
  } else if (_tcscmp(node.getName(), _T("Label")) == 0){
    // Create the FrameControl
    WndFrame* frame = new WndFrame(parent, *xml_dialog_look,
                                   pos.x, pos.y, size.cx, size.cy,
                                   style);

    // Set the caption
    frame->SetCaption(caption);
    // Set caption color
    Color color;
    if (StringToColor(node.getAttribute(_T("CaptionColor")), color))
      frame->SetCaptionColor(color);

    window = frame;

  // ListBoxControl (WndListFrame)
  } else if (_tcscmp(node.getName(), _T("List")) == 0){
    // Determine ItemHeight of the list items
    UPixelScalar item_height =
      Layout::Scale(StringToIntDflt(node.getAttribute(_T("ItemHeight")), 18));

    // Create the ListBoxControl

    style.tab_stop();

    if (is_embedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      style.border();
    else
      style.sunken_edge();

    window = new WndListFrame(parent, *xml_dialog_look,
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

    for (XMLNode::const_iterator i = node.begin(), end = node.end();
         i != end; ++i) {
      // Load each child control from the child nodes
      Window *child = LoadChild(form, *tabbed,
                                lookup_table,
                                *i, dialog_style);
      if (child != NULL)
        tabbed->AddClient(child);
    }
  // TabBarControl (TabBar)
  } else if (_tcscmp(node.getName(), _T("TabBar")) == 0) {
    // Create the TabBarControl

    bool flip_orientation = false;
    if ( (Layout::landscape && StringToIntDflt(node.getAttribute(_T("Horizontal")), 0)) ||
         (!Layout::landscape && StringToIntDflt(node.getAttribute(_T("Vertical")), 0) ) )
      flip_orientation = true;

    style.control_parent();
    TabBarControl *tabbar = new TabBarControl(parent, *xml_dialog_look,
                                              pos.x, pos.y, size.cx, size.cy,
                                              style, flip_orientation);
    window = tabbar;

    // TabMenuControl (TabMenu)
    } else if (_tcscmp(node.getName(), _T("TabMenu")) == 0) {
      // Create the TabMenuControl

      style.control_parent();
      TabMenuControl *tabmenu = new TabMenuControl(parent, form,
                                                   lookup_table,
                                                   *xml_dialog_look, caption,
                                                   pos.x, pos.y, size.cx, size.cy,
                                                   style);
      window = tabmenu;

  } else if (_tcscmp(node.getName(), _T("Custom")) == 0) {
    // Create a custom Window object with a callback
    CreateWindowCallback_t create_callback =
        (CreateWindowCallback_t)GetCallBack(lookup_table, node, _T("OnCreate"));
    if (create_callback == NULL)
      return NULL;

    window = create_callback(parent, pos.x, pos.y, size.cx, size.cy, style);
  }

  if (window != NULL) {
    if (!string_is_empty(name))
      form.AddNamed(name, window);

    if (expert)
      form.AddExpert(window);

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
                    const CallBackTableEntry *lookup_table,
                    const XMLNode *node,
                    const DialogStyle dialog_style)
{
  unsigned bottom_most = 0;

  // Iterate through the childnodes
  for (XMLNode::const_iterator i = node->begin(), end = node->end();
       i != end; ++i) {
    // Load each child control from the child nodes
    Window *window = LoadChild(form, parent, lookup_table,
                               *i, dialog_style,
                               bottom_most);
    if (window == NULL)
      continue;

    bottom_most = window->get_position().bottom;
  }
}
