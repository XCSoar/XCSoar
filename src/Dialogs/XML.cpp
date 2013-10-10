/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "XML/Node.hpp"
#include "XML/Parser.hpp"
#include "Form/DataField/Float.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/LargeTextWindow.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/Draw.hpp"
#include "Form/List.hpp"
#include "Form/Panel.hpp"
#include "Form/CheckBox.hpp"
#include "Widget/DockWindow.hpp"
#include "Util/StringUtil.hpp"
#include "Util/ConvertString.hpp"
#include "Util/NumberParser.hpp"
#include "ResourceLoader.hpp"
#include "Look/DialogLook.hpp"
#include "Inflate.hpp"

#include <stdio.h>    // for _stprintf
#include <assert.h>
#include <tchar.h>
#include <limits.h>

// used when stretching dialog and components
static int dialog_width_scale = 1024;

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
                                          PixelRect rc,
                                          const WindowStyle style);

static Window *
LoadChild(SubForm &form, ContainerWindow &parent, const PixelRect &parent_rc,
          const CallBackTableEntry *lookup_table, XMLNode node,
          int bottom_most = 0,
          WindowStyle style=WindowStyle());

static void
LoadChildrenFromXML(SubForm &form, ContainerWindow &parent,
                    const CallBackTableEntry *lookup_table,
                    const XMLNode *node);

/**
 * Converts a String into an Integer and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Integer value
 */
static int
StringToIntDflt(const TCHAR *string, int _default)
{
  if (string == NULL || StringIsEmpty(string))
    return _default;
  return ParseInt(string, NULL, 0);
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
  if (string == NULL || StringIsEmpty(string))
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
  if (string == NULL || StringIsEmpty(string))
    return _default;
  return string;
}

static int 
ScaleWidth(const int x)
{
  const DialogStyle dialog_style = UIGlobals::GetDialogSettings().dialog_style;
  if (dialog_style == dsFullWidth || dialog_style == dsScaledBottom)
    // stretch width to fill screen horizontally
    return x * dialog_width_scale / 1024;
  else
    return x;
}

static const TCHAR*
GetName(const XMLNode &node)
{
  return StringToStringDflt(node.GetAttribute(_T("Name")), _T(""));
}

static const TCHAR*
GetCaption(const XMLNode &node)
{
  const TCHAR* tmp =
      StringToStringDflt(node.GetAttribute(_T("Caption")), _T(""));

  // don't translate empty strings, it would query gettext metadata
  if (tmp[0] == _T('\0'))
    return tmp;

  return gettext(tmp);
}

static ControlPosition
GetPosition(const XMLNode &node, const PixelRect rc, int bottom_most = -1)
{
  ControlPosition pt;

  // Calculate x- and y-Coordinate
  pt.x = StringToIntDflt(node.GetAttribute(_T("X")), 0);
  pt.y = StringToIntDflt(node.GetAttribute(_T("Y")), -1);
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
  sz.cx = StringToIntDflt(node.GetAttribute(_T("Width")), 0);
  sz.cy = StringToIntDflt(node.GetAttribute(_T("Height")), 0);
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
  assert(name != NULL);
  assert(!StringIsEmpty(name));
  assert(lookup_table != NULL);

  for (const CallBackTableEntry *p = lookup_table;; ++p) {
    assert(p->name != NULL);
    assert(p->callback != NULL);

    if (StringIsEqual(p->name, name))
      return p->callback;
  }
}

static void *
GetCallBack(const CallBackTableEntry *lookup_table,
            const XMLNode &node, const TCHAR* attribute)
{
  const TCHAR *name = node.GetAttribute(attribute);
  if (name == NULL)
    return NULL;

  assert(!StringIsEmpty(name));

  return CallBackLookup(lookup_table, name);
}

static XMLNode *
LoadXMLFromResource(const TCHAR* resource, XML::Results *xml_results)
{
  ResourceLoader::Data data = ResourceLoader::Load(resource, _T("XMLDialog"));
  assert(!data.IsNull());

  char *buffer = InflateToString(data.data, data.size);
  assert(buffer != nullptr);

  UTF8ToWideConverter buffer2(buffer);
  assert(buffer2.IsValid());

  XMLNode *x = XML::ParseString(buffer2, xml_results);

  delete[] buffer;

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
  XMLNode *node = LoadXMLFromResource(resource, nullptr);
  assert(node != nullptr);

  return node;
}

static void
InitScaleWidth(const PixelSize size, const PixelRect rc)
{
  // No need to calculate the scale factor on platforms that don't scale
  if (!Layout::ScaleSupported())
    return;

  const DialogStyle dialog_style = UIGlobals::GetDialogSettings().dialog_style;
  if (dialog_style == dsFullWidth || dialog_style == dsScaledBottom)
    dialog_width_scale = (rc.right - rc.left) * 1024 / size.cx;
  else
    dialog_width_scale = 1024;
}

Window *
LoadWindow(const CallBackTableEntry *lookup_table, SubForm *form,
           ContainerWindow &parent, const PixelRect &rc,
           const TCHAR *resource, WindowStyle style)
{
  if (!form)
    return NULL;

  XMLNode *node = LoadXMLFromResource(resource);
  assert(node != NULL);

  // load only one top-level control.
  Window *window = LoadChild(*form, parent, rc, lookup_table, *node, 0, style);
  delete node;

  return window;
}

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
  assert(StringIsEqual(node->GetName(), _T("Form")));

  // Determine the dialog size
  const TCHAR* caption = GetCaption(*node);
  const PixelRect rc = target_rc ? *target_rc : parent.GetClientRect();
  ControlPosition pos = GetPosition(*node, rc, 0);
  ControlSize size = GetSize(*node, rc, pos);

  InitScaleWidth(size, rc);

  // Correct dialog size and position for dialog style
  switch (UIGlobals::GetDialogSettings().dialog_style) {
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
  style.Hide();
  style.ControlParent();

  PixelRect form_rc;
  form_rc.left = pos.x;
  form_rc.top = pos.y;
  form_rc.right = form_rc.left + size.cx;
  form_rc.bottom = form_rc.top + size.cy;

  form = new WndForm(parent, UIGlobals::GetDialogLook(),
                     form_rc, caption, style);

  // Load the children controls
  LoadChildrenFromXML(*form, form->GetClientAreaWindow(),
                      lookup_table, node);
  delete node;

  // Return the created form
  return form;
}

static DataField *
LoadDataField(const XMLNode &node, const CallBackTableEntry *LookUpTable)
{
  const TCHAR *data_type =
    StringToStringDflt(node.GetAttribute(_T("DataType")), _T(""));
  const TCHAR *display_format =
    StringToStringDflt(node. GetAttribute(_T("DisplayFormat")), _T(""));
  const TCHAR *edit_format =
    StringToStringDflt(node.GetAttribute(_T("EditFormat")), _T(""));

  fixed min = fixed(StringToFloatDflt(node.GetAttribute(_T("Min")), INT_MIN));
  fixed max = fixed(StringToFloatDflt(node.GetAttribute(_T("Max")), INT_MAX));
  fixed step = fixed(StringToFloatDflt(node.GetAttribute(_T("Step")), 1));
  const bool fine = false;

  if (StringIsEqualIgnoreCase(data_type, _T("double")))
    return new DataFieldFloat(edit_format, display_format, min, max,
                              fixed(0), step, fine);

  return NULL;
}

/**
 * Creates a control from the given XMLNode as a child of the given
 * parent.
 *
 * @param form the SubForm object
 * @param LookUpTable The parent CallBackTable
 * @param node The XMLNode that represents the control
 */
static Window *
LoadChild(SubForm &form, ContainerWindow &parent, const PixelRect &parent_rc,
          const CallBackTableEntry *lookup_table, XMLNode node,
          int bottom_most,
          WindowStyle style)
{
  Window *window = NULL;

  // Determine name, coordinates, width, height
  // and caption of the control
  const TCHAR* name = GetName(node);
  const TCHAR* caption = GetCaption(node);
  ControlPosition pos = GetPosition(node, parent_rc, bottom_most);
  if (!pos.no_scaling)
    pos.x = ScaleWidth(pos.x);

  ControlSize size = GetSize(node, parent_rc, pos);
  if (!size.no_scaling)
    size.cx = ScaleWidth(size.cx);

  if (!StringToIntDflt(node.GetAttribute(_T("Visible")), 1))
    style.Hide();

  if (StringToIntDflt(node.GetAttribute(_T("Border")), 0))
    style.Border();

  PixelRect rc;
  rc.left = pos.x;
  rc.top = pos.y;
  rc.right = rc.left + size.cx;
  rc.bottom = rc.top + size.cy;

  // PropertyControl (WndProperty)
  if (StringIsEqual(node.GetName(), _T("Edit"))) {
    // Determine the width of the caption field
    int caption_width = StringToIntDflt(node.GetAttribute(_T("CaptionWidth")), 0);

    if (Layout::ScaleSupported())
      caption_width = Layout::Scale(caption_width);

    caption_width = ScaleWidth(caption_width);

    // Determine whether the control is multiline or readonly
    bool read_only = StringToIntDflt(node.GetAttribute(_T("ReadOnly")), 0);

    // Create the Property Control
    if (!read_only)
      style.TabStop();

    WndProperty *property;
    window = property = new WndProperty(parent, UIGlobals::GetDialogLook(),
                                        caption, rc,
                                        caption_width, style);
    property->SetReadOnly(read_only);

    // Load the help text
    property->SetHelpText(StringToStringDflt(node.GetAttribute(_T("Help")),
                                             NULL));

    // If the control has (at least) one DataField child control
    const XMLNode *data_field_node = node.GetChildNode(_T("DataField"));
    if (data_field_node != NULL) {
      // -> Load the first DataField control
      DataField *data_field =
        LoadDataField(*data_field_node, lookup_table);

      if (data_field != NULL)
        // Tell the Property control about the DataField control
        property->SetDataField(data_field);
    }

  // ButtonControl (WndButton)
  } else if (StringIsEqual(node.GetName(), _T("Button"))) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback click_callback =
      (WndButton::ClickNotifyCallback)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the ButtonControl

    ButtonWindowStyle button_style(style);
    button_style.TabStop();
    button_style.multiline();

    window = new WndButton(parent, UIGlobals::GetDialogLook().button,
                           caption, rc,
                           button_style, click_callback);

  } else if (StringIsEqual(node.GetName(), _T("CloseButton"))) {
    ButtonWindowStyle button_style(style);
    button_style.TabStop();

    window = new WndButton(parent, UIGlobals::GetDialogLook().button,
                           _("Close"), rc,
                           button_style, (WndForm &)form, mrOK);
  } else if (StringIsEqual(node.GetName(), _T("CheckBox"))) {
    // Determine click_callback function
    CheckBoxControl::ClickNotifyCallback click_callback =
      (CheckBoxControl::ClickNotifyCallback)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the CheckBoxControl

    style.TabStop();

    window = new CheckBoxControl(parent, UIGlobals::GetDialogLook(),
                                 caption, rc,
                                 style, click_callback);

  // SymbolButtonControl (WndSymbolButton) not used yet
  } else if (StringIsEqual(node.GetName(), _T("SymbolButton"))) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback click_callback =
      (WndButton::ClickNotifyCallback)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the SymbolButtonControl

    style.TabStop();

    const TCHAR* original_caption =
        StringToStringDflt(node.GetAttribute(_T("Caption")), _T(""));

    window = new WndSymbolButton(parent, UIGlobals::GetDialogLook().button,
                                 original_caption, rc,
                                 style, click_callback);

  // PanelControl (WndPanel)
  } else if (StringIsEqual(node.GetName(), _T("Panel"))) {
    // Create the PanelControl

    style.ControlParent();

    PanelControl *frame = new PanelControl(parent, UIGlobals::GetDialogLook(),
                                           rc,
                                           style);

    window = frame;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(form, *frame,
                        lookup_table, &node);

  // DrawControl (WndOwnerDrawFrame)
  } else if (StringIsEqual(node.GetName(), _T("Canvas"))) {
    // Determine DrawCallback function
    WndOwnerDrawFrame::OnPaintCallback_t paint_callback =
      (WndOwnerDrawFrame::OnPaintCallback_t)
      GetCallBack(lookup_table, node, _T("OnPaint"));

    // Create the DrawControl
    WndOwnerDrawFrame *canvas = new WndOwnerDrawFrame();
    canvas->Create(parent, rc, style, paint_callback);

    window = canvas;

  // FrameControl (WndFrame)
  } else if (StringIsEqual(node.GetName(), _T("Label"))){
    // Create the FrameControl
    WndFrame* frame = new WndFrame(parent, UIGlobals::GetDialogLook(),
                                   rc, style);

    // Set the caption
    frame->SetCaption(caption);

    window = frame;

  } else if (StringIsEqual(node.GetName(), _T("LargeText"))){
    if (IsEmbedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      style.Border();
    else
      style.SunkenEdge();

    LargeTextWindow *ltw = new LargeTextWindow();
    ltw->Create(parent, rc, style);
    ltw->SetFont(*UIGlobals::GetDialogLook().text_font);

    window = ltw;

  // ListBoxControl (ListControl)
  } else if (StringIsEqual(node.GetName(), _T("List"))){
    // Determine ItemHeight of the list items
    UPixelScalar item_height =
      Layout::Scale(StringToIntDflt(node.GetAttribute(_T("ItemHeight")), 18));

    // Create the ListBoxControl

    style.TabStop();

    if (IsEmbedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      style.Border();
    else
      style.SunkenEdge();

    const PixelRect rc(pos.x, pos.y, pos.x + size.cx, pos.y + size.cy);

    window = new ListControl(parent, UIGlobals::GetDialogLook(),
                             rc, style, item_height);

  } else if (StringIsEqual(node.GetName(), _T("Custom"))) {
    // Create a custom Window object with a callback
    CreateWindowCallback_t create_callback =
        (CreateWindowCallback_t)GetCallBack(lookup_table, node, _T("OnCreate"));
    if (create_callback == NULL)
      return NULL;

    window = create_callback(parent, rc, style);
  } else if (StringIsEqual(node.GetName(), _T("Widget"))) {
    style.ControlParent();
    DockWindow *dock = new DockWindow();
    dock->Create(parent, rc, style);
    window = dock;
  }

  if (window != NULL) {
    if (!StringIsEmpty(name))
      form.AddNamed(name, window);

    form.AddDestruct(window);
  }

  return window;
}

/**
 * Loads the Parent's children Controls from the given XMLNode
 *
 * @param form the SubForm object
 * @param Parent The parent control
 * @param LookUpTable The parents CallBackTable
 * @param Node The XMLNode that represents the parent control
 */
static void
LoadChildrenFromXML(SubForm &form, ContainerWindow &parent,
                    const CallBackTableEntry *lookup_table,
                    const XMLNode *node)
{
  unsigned bottom_most = 0;

  // Iterate through the childnodes
  for (auto i = node->begin(), end = node->end(); i != end; ++i) {
    // Load each child control from the child nodes
    Window *window = LoadChild(form, parent, parent.GetClientRect(),
                               lookup_table, *i, bottom_most);
    if (window == NULL)
      continue;

    bottom_most = window->GetPosition().bottom;
  }
}
