/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "dlgConfigInfoboxes.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Language/Language.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StaticArray.hxx"

#include <assert.h>

static InfoBoxSettings::Panel clipboard;
static unsigned clipboard_size;

class InfoBoxesConfigWidget;

class InfoBoxPreview : public PaintWindow {
  InfoBoxesConfigWidget *parent;
  unsigned i;

public:
  void SetParent(InfoBoxesConfigWidget &_parent, unsigned _i) {
    parent = &_parent;
    i = _i;
  }

protected:
  /* virtual methods from class Window */
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseDouble(PixelPoint p) override;

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

class InfoBoxesConfigWidget final
  : public RowFormWidget, DataFieldListener, ActionListener {

  enum Controls {
    NAME, INFOBOX, CONTENT, DESCRIPTION
  };

  enum Buttons {
    COPY, PASTE,
  };

  struct Layout {
    InfoBoxLayout::Layout info_boxes;

    PixelRect form;

    PixelRect copy_button, paste_button, close_button;

    Layout(PixelRect rc, InfoBoxSettings::Geometry geometry);
  };

  ActionListener &dialog;
  const InfoBoxLook &look;

  InfoBoxSettings::Panel &data;
  const bool allow_name_change;
  bool changed;

  const InfoBoxSettings::Geometry geometry;

  StaticArray<InfoBoxPreview, InfoBoxSettings::Panel::MAX_CONTENTS> previews;
  unsigned current_preview;

  Button copy_button, paste_button, close_button;

public:
  InfoBoxesConfigWidget(ActionListener &_dialog,
                        const DialogLook &dialog_look,
                        const InfoBoxLook &_look,
                        InfoBoxSettings::Panel &_data,
                        bool _allow_name_change,
                        InfoBoxSettings::Geometry _geometry)
    :RowFormWidget(dialog_look),
     dialog(_dialog),
     look(_look),
     data(_data),
     allow_name_change(_allow_name_change),
     changed(false),
     geometry(_geometry) {}

  const InfoBoxLook &GetInfoBoxLook() const {
    return look;
  }

  const InfoBoxSettings::Panel &GetData() const {
    return data;
  }

  void RefreshPasteButton() {
    paste_button.SetEnabled(clipboard_size > 0);
  }

  void RefreshEditContentDescription();
  void RefreshEditContent();

  void OnCopy();
  void OnPaste();

  void SetCurrentInfoBox(unsigned _current_preview);

  unsigned GetCurrentInfoBox() const {
    return current_preview;
  }

  InfoBoxFactory::Type GetContents(unsigned i) const {
    return data.contents[i];
  }

  void BeginEditing() {
    GetControl(CONTENT).BeginEditing();
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

  bool Save(bool &changed) override;

  void Show(const PixelRect &rc) override {
    const Layout layout(rc, geometry);

    RowFormWidget::Show(layout.form);

    copy_button.MoveAndShow(layout.copy_button);
    paste_button.MoveAndShow(layout.paste_button);
    close_button.MoveAndShow(layout.close_button);

    for (unsigned i = 0; i < previews.size(); ++i)
      previews[i].MoveAndShow(layout.info_boxes.positions[i]);
  }

  void Hide() override {
    RowFormWidget::Hide();

    copy_button.Hide();
    paste_button.Hide();
    close_button.Hide();

    for (auto &i : previews)
      i.Hide();
  }

  void Move(const PixelRect &rc) override {
    const Layout layout(rc, geometry);

    RowFormWidget::Move(layout.form);

    copy_button.Move(layout.copy_button);
    paste_button.Move(layout.paste_button);
    close_button.Move(layout.close_button);
  }

  bool SetFocus() override {
    GetGeneric(INFOBOX).SetFocus();
    return true;
  }

private:
  /* virtual methods from class DataFieldListener */

  void OnModified(DataField &df) override {
    if (IsDataField(INFOBOX, df)) {
      const DataFieldEnum &dfe = (const DataFieldEnum &)df;
      SetCurrentInfoBox(dfe.GetValue());
    } else if (IsDataField(CONTENT, df)) {
      const DataFieldEnum &dfe = (const DataFieldEnum &)df;

      auto new_value = (InfoBoxFactory::Type)dfe.GetValue();
      if (new_value == data.contents[current_preview])
        return;

      changed = true;
      data.contents[current_preview] = new_value;
      previews[current_preview].Invalidate();
      RefreshEditContentDescription();
    }
  }

  /* virtual methods from class ActionListener */
  void OnAction(int id) override {
    switch (id) {
    case COPY:
      OnCopy();
      break;

    case PASTE:
      OnPaste();
      break;
    }
  }
};

InfoBoxesConfigWidget::Layout::Layout(PixelRect rc,
                                      InfoBoxSettings::Geometry geometry)
{
  info_boxes = InfoBoxLayout::Calculate(rc, geometry);

  form = info_boxes.remaining;
  PixelRect buttons = form;
  buttons.top = form.bottom -= ::Layout::GetMaximumControlHeight();

  copy_button = paste_button = close_button = buttons;
  copy_button.right = paste_button.left =
    (2 * buttons.left + buttons.right) / 3;
  paste_button.right = close_button.left =
    (buttons.left + 2 * buttons.right) / 3;
}

void
InfoBoxesConfigWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc)
{
  const Layout layout(rc, geometry);

  AddText(_("Name"), nullptr,
          allow_name_change ? (const TCHAR *)data.name : gettext(data.name));
  SetReadOnly(NAME, !allow_name_change);

  DataFieldEnum *dfe = new DataFieldEnum(this);
  for (unsigned i = 0; i < layout.info_boxes.count; ++i) {
    TCHAR label[32];
    _stprintf(label, _T("%u"), i + 1);
    dfe->addEnumText(label, i);
  }

  Add(_("InfoBox"), nullptr, dfe);

  dfe = new DataFieldEnum(this);
  for (unsigned i = InfoBoxFactory::MIN_TYPE_VAL; i < InfoBoxFactory::NUM_TYPES; i++) {
    const TCHAR *name = InfoBoxFactory::GetName((InfoBoxFactory::Type) i);
    const TCHAR *desc = InfoBoxFactory::GetDescription((InfoBoxFactory::Type) i);
    if (name != NULL)
      dfe->addEnumText(gettext(name), i, desc != NULL ? gettext(desc) : NULL);
  }

  dfe->EnableItemHelp(true);
  dfe->Sort(0);

  Add(_("Content"), nullptr, dfe);

  ContainerWindow &form_parent = (ContainerWindow &)RowFormWidget::GetWindow();
  AddRemaining(new WndFrame(form_parent, GetLook(), rc));

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  const auto &button_look = GetLook().button;
  copy_button.Create(parent, button_look, _("Copy"), layout.copy_button,
                     button_style, *this, COPY);
  paste_button.Create(parent, button_look, _("Paste"), layout.paste_button,
                      button_style, *this, PASTE);
  close_button.Create(parent, button_look, _("Close"), layout.close_button,
                      button_style, dialog, mrOK);

  WindowStyle preview_style;
  preview_style.Hide();

  previews.resize(layout.info_boxes.count);
  for (unsigned i = 0; i < layout.info_boxes.count; ++i) {
    previews[i].SetParent(*this, i);
    previews[i].Create(parent, layout.info_boxes.positions[i],
                       preview_style);
  }

  current_preview = 0;

  RefreshEditContent();
  RefreshPasteButton();
}

bool
InfoBoxesConfigWidget::Save(bool &changed_r)
{
  if (allow_name_change) {
    const auto *new_name = GetValueString(InfoBoxesConfigWidget::NAME);
    if (!StringIsEqual(new_name, data.name)) {
      data.name = new_name;
      changed = true;
    }
  }

  changed_r = changed;
  return true;
}

void
InfoBoxesConfigWidget::RefreshEditContentDescription()
{
  DataFieldEnum &df = (DataFieldEnum &)GetDataField(CONTENT);
  WndFrame &description = (WndFrame &)GetRow(DESCRIPTION);
  description.SetText(df.GetHelp() != nullptr ? df.GetHelp() : _T(""));
}

void
InfoBoxesConfigWidget::RefreshEditContent()
{
  LoadValueEnum(CONTENT, data.contents[current_preview]);
}

void
InfoBoxesConfigWidget::OnCopy()
{
  clipboard = data;
  clipboard_size = InfoBoxSettings::Panel::MAX_CONTENTS;

  RefreshPasteButton();
}

void
InfoBoxesConfigWidget::OnPaste()
{
  if (clipboard_size == 0)
    return;

  if(ShowMessageBox(_("Overwrite?"), _("InfoBox paste"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  for (unsigned item = 0; item < clipboard_size; item++) {
    InfoBoxFactory::Type content = clipboard.contents[item];
    if (content >= InfoBoxFactory::NUM_TYPES)
      continue;

    data.contents[item] = content;

    if (item < previews.size())
      previews[item].Invalidate();
  }

  RefreshEditContent();
  changed = true;
}

void
InfoBoxesConfigWidget::SetCurrentInfoBox(unsigned _current_preview)
{
  assert(_current_preview < previews.size());

  if (_current_preview == current_preview)
    return;

  previews[current_preview].Invalidate();
  current_preview = _current_preview;
  previews[current_preview].Invalidate();

  LoadValueEnum(INFOBOX, current_preview);

  RefreshEditContent();
}

bool
InfoBoxPreview::OnMouseDown(PixelPoint p)
{
  parent->SetCurrentInfoBox(i);
  return true;
}

bool
InfoBoxPreview::OnMouseDouble(PixelPoint p)
{
  parent->BeginEditing();
  return true;
}

void
InfoBoxPreview::OnPaint(Canvas &canvas)
{
  const bool is_current = i == parent->GetCurrentInfoBox();

  if (is_current)
    canvas.Clear(COLOR_BLACK);
  else
    canvas.ClearWhite();

  canvas.SelectHollowBrush();
  canvas.SelectBlackPen();
  canvas.Rectangle(0, 0, canvas.GetWidth() - 1, canvas.GetHeight() - 1);

  InfoBoxFactory::Type type = parent->GetContents(i);
  const TCHAR *caption = type < InfoBoxFactory::NUM_TYPES
    ? InfoBoxFactory::GetCaption(type)
    : NULL;
  if (caption == NULL)
    caption = _("Invalid");
  else
    caption = gettext(caption);

  canvas.Select(parent->GetInfoBoxLook().title_font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(is_current ? COLOR_WHITE : COLOR_BLACK);
  canvas.DrawText(2, 2, caption);
}

bool
dlgConfigInfoboxesShowModal(SingleWindow &parent,
                            const DialogLook &dialog_look,
                            const InfoBoxLook &_look,
                            InfoBoxSettings::Geometry geometry,
                            InfoBoxSettings::Panel &data_r,
                            bool allow_name_change)
{
  WidgetDialog dialog(dialog_look);
  InfoBoxesConfigWidget widget(dialog, dialog_look, _look,
                               data_r, allow_name_change, geometry);
  dialog.CreateFull(parent, nullptr, &widget);

  dialog.ShowModal();
  dialog.StealWidget();

  return dialog.GetChanged();
}
