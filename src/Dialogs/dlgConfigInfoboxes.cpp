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

#include "Dialogs/dlgConfigInfoboxes.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/TextEntry.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "DataField/Enum.hpp"
#include "Compiler.h"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "Compiler.h"

#include <assert.h>
#include <cstdio>
#include <algorithm>

class InfoBoxPreview : public PaintWindow {
protected:
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_double(int x, int y);
  virtual void on_paint(Canvas &canvas);
};

static InfoBoxPanelConfig data;
static WndForm *wf = NULL;
static InfoBoxPanelConfig clipboard;
static unsigned clipboard_size;
static InfoBoxLayout::Layout info_box_layout;
static InfoBoxPreview previews[InfoBoxPanelConfig::MAX_INFOBOXES];
static unsigned current_preview;

static WndButton *buttonPanelName;
static WndProperty *edit_select;
static WndProperty *edit_content;
static WndButton *buttonPaste;

static void
RefreshPasteButton()
{
  buttonPaste->set_enabled(clipboard_size > 0);
}

static void
RefreshEditContent()
{
  DataFieldEnum &df = *(DataFieldEnum *)edit_content->GetDataField();
  df.Set(data.infoBoxID[current_preview]);
  edit_content->RefreshDisplay();
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnCopy(gcc_unused WndButton &button)
{
  clipboard = data;
  clipboard_size = InfoBoxPanelConfig::MAX_INFOBOXES;

  RefreshPasteButton();
}

static void
OnPaste(gcc_unused WndButton &button)
{
  if (clipboard_size == 0)
    return;

  if(MessageBoxX(_("Overwrite?"), _("InfoBox paste"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  for (unsigned item = 0; item < clipboard_size; item++) {
    unsigned content = clipboard.infoBoxID[item];
    if (content >= InfoBoxFactory::NUM_TYPES)
      continue;

    data.infoBoxID[item] = content;
    previews[item].invalidate();
  }

  RefreshEditContent();
}

static void
SetCurrentInfoBox(unsigned _current_preview)
{
  assert(_current_preview < info_box_layout.count);

  if (_current_preview == current_preview)
    return;

  previews[current_preview].invalidate();
  current_preview = _current_preview;
  previews[current_preview].invalidate();

  DataFieldEnum &df = *(DataFieldEnum *)edit_select->GetDataField();
  df.Set(current_preview);
  edit_select->RefreshDisplay();

  RefreshEditContent();
}

static void
OnSelectAccess(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &dfe = (const DataFieldEnum &)*Sender;

  SetCurrentInfoBox(dfe.GetAsInteger());
}

static void
OnContentAccess(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &dfe = (const DataFieldEnum &)*Sender;

  data.infoBoxID[current_preview] = dfe.GetAsInteger();
  previews[current_preview].invalidate();
}

bool
InfoBoxPreview::on_mouse_down(int x, int y)
{
  SetCurrentInfoBox(this - previews);
  return true;
}

bool
InfoBoxPreview::on_mouse_double(int x, int y)
{
  edit_content->BeginEditing();
  return true;
}

void
InfoBoxPreview::on_paint(Canvas &canvas)
{
  const unsigned i = this - previews;
  const bool is_current = i == current_preview;

  if (is_current)
    canvas.clear(Color::BLACK);
  else
    canvas.clear_white();

  canvas.hollow_brush();
  canvas.black_pen();
  canvas.rectangle(0, 0, canvas.get_width() - 1, canvas.get_height() - 1);

  unsigned type = data.infoBoxID[i];
  const TCHAR *caption = type < InfoBoxFactory::NUM_TYPES
    ? InfoBoxFactory::GetCaption(type)
    : NULL;
  if (caption == NULL)
    caption = _("Invalid");

  canvas.select(Fonts::Title);
  canvas.background_transparent();
  canvas.set_text_color(is_current ? Color::WHITE : Color::BLACK);
  canvas.text(2, 2, caption);
}

static void
OnContentHelp(WindowControl *Sender)
{
  WndProperty *wp = (WndProperty*)Sender;
  unsigned type = wp->GetDataField()->GetAsInteger();
  if (type >= InfoBoxFactory::NUM_TYPES)
    return;

  const TCHAR *name = InfoBoxFactory::GetName(type);
  if (name == NULL)
    return;

  TCHAR caption[100];
  _stprintf(caption, _T("%s: %s"), _("InfoBox"), gettext(name));

  const TCHAR *text = InfoBoxFactory::GetDescription(type);
  if (text == NULL)
    text = _("No help available on this item");
  else
    text = gettext(text);

  dlgHelpShowModal(wf->GetMainWindow(), caption, text);
}


static void
UpdatePanelName()
{
  const unsigned BUFFER_LENGTH = InfoBoxPanelConfig::MAX_PANEL_NAME_LENGTH + 32;
  TCHAR caption[BUFFER_LENGTH];

  _sntprintf(caption, BUFFER_LENGTH, _T("%s: %s"), _("Name"), data.name);
  buttonPanelName->SetCaption(caption);
}


static void
OnNameAccess(WndButton &button)
{
  if (buttonPanelName) {
    TCHAR buffer[InfoBoxPanelConfig::MAX_PANEL_NAME_LENGTH];
    _tcscpy(buffer, data.name);
    if (dlgTextEntryShowModal(buffer, InfoBoxPanelConfig::MAX_PANEL_NAME_LENGTH)) {
      _tcscpy(data.name, buffer);
      UpdatePanelName();
    }
  }
}


bool
dlgConfigInfoboxesShowModal(SingleWindow &parent,
                            InfoBoxLayout::Geometry geometry,
                            InfoBoxPanelConfig &data_r,
                            bool allow_name_change)
{
  current_preview = 0;
  data = data_r;

  PixelRect rc = parent.get_client_rect();
  wf = new WndForm(parent, rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top);

  ContainerWindow &client_area = wf->GetClientAreaWindow();
  rc = client_area.get_client_rect();

  InflateRect(&rc, Layout::FastScale(-2), Layout::FastScale(-2));
  info_box_layout = InfoBoxLayout::Calculate(rc, geometry);

  WindowStyle preview_style;
  preview_style.enable_double_clicks();
  for (unsigned i = 0; i < info_box_layout.count; ++i) {
    rc = info_box_layout.positions[i];
    previews[i].set(client_area, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top,
                    preview_style);
  }

  rc = info_box_layout.remaining;

  WindowStyle style;
  style.control_parent();

  EditWindowStyle edit_style;
  edit_style.tab_stop();

  if (is_embedded() || Layout::scale_1024 < 2048)
    /* sunken edge doesn't fit well on the tiny screen of an
       embedded device */
    edit_style.border();
  else
    edit_style.sunken_edge();

  const Color background_color = wf->GetBackColor();
  const int x = rc.left;
  const unsigned width = rc.right - rc.left - Layout::FastScale(2);
  const unsigned height = Layout::Scale(22);
  const unsigned caption_width = Layout::Scale(60);

  int y = rc.top;

  ButtonWindowStyle button_style;
  button_style.tab_stop();

  buttonPanelName =
    new WndButton(client_area, _T(""),
                  x, y, width, height, button_style, OnNameAccess);
  buttonPanelName->set_enabled(allow_name_change);
  UpdatePanelName();

  y += height;

  edit_select = new WndProperty(client_area, _("InfoBox"),
                                x, y, width, height, caption_width,
                                background_color, style, edit_style,
                                NULL);

  DataFieldEnum *dfe = new DataFieldEnum(OnSelectAccess);
  for (unsigned i = 0; i < info_box_layout.count; ++i) {
    TCHAR label[32];
    _stprintf(label, _T("%u"), i + 1);
    dfe->addEnumText(label, i);
  }

  edit_select->SetDataField(dfe);

  y += height;

  edit_content = new WndProperty(client_area, _("Content"),
                                 x, y, width, height, caption_width,
                                 background_color, style, edit_style,
                                 NULL);

  dfe = new DataFieldEnum(OnContentAccess);
  for (unsigned i = 0; i < InfoBoxFactory::NUM_TYPES; ++i) {
    const TCHAR *name = InfoBoxFactory::GetName(i);
    if (name != NULL)
      dfe->addEnumText(name, i, InfoBoxFactory::GetDescription(i));
  }

  dfe->Sort(0);

  edit_content->SetDataField(dfe);
  edit_content->SetOnHelpCallback(OnContentHelp);

  RefreshEditContent();

  const unsigned button_width = Layout::Scale(60);
  const unsigned button_height = Layout::Scale(28);
  const int button_y = rc.bottom - button_height;
  int button_x = rc.left;
  WndButton *close_button =
    new WndButton(client_area, _("Close"),
                  button_x, button_y, button_width, button_height,
                  button_style, OnCloseClicked);
  button_x += button_width + Layout::Scale(2);
  WndButton *copy_button =
    new WndButton(client_area, _("Copy"),
                  button_x, button_y, button_width, button_height,
                  button_style, OnCopy);
  button_x += button_width + Layout::Scale(2);
  buttonPaste =
    new WndButton(client_area, _("Paste"),
                  button_x, button_y, button_width, button_height,
                  button_style, OnPaste);

  RefreshPasteButton();

  int result = wf->ShowModal();

  delete wf;
  delete buttonPanelName;
  delete edit_select;
  delete edit_content;
  delete close_button;
  delete copy_button;
  delete buttonPaste;

  bool changed = false;
  if (result == mrOK) {
    for (unsigned i = 0; i < InfoBoxPanelConfig::MAX_INFOBOXES; ++i)
      if (data.infoBoxID[i] != data_r.infoBoxID[i])
        changed = true;
    changed |= (_tcscmp(data.name, data_r.name) != 0);

    if (changed)
      data_r = data;
  }

  return changed;
}
