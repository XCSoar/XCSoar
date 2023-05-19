// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgConfigInfoboxes.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"
#include "util/StaticArray.hxx"

#include <cassert>

using namespace UI;

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
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};

class InfoBoxesConfigWidget final
  : public RowFormWidget, DataFieldListener {

  enum Controls {
    SET_NAME, SPACER, BOX_POS, BOX_NAME, BOX_DESCR
  };

  struct Layout {
    InfoBoxLayout::Layout info_boxes;

    PixelRect form;

    PixelRect swap_chkbox, copy_button, paste_button, close_button;

    Layout(PixelRect rc, InfoBoxSettings::Geometry geometry);
  };

  WndForm &dialog;
  const InfoBoxLook &look;

  InfoBoxSettings::Panel &data;
  const bool allow_name_change;
  bool changed;

  const InfoBoxSettings::Geometry geometry;

  StaticArray<InfoBoxPreview, InfoBoxSettings::Panel::MAX_CONTENTS> previews;
  unsigned current_preview;

  Button copy_button, paste_button, close_button;
  CheckBoxControl swap_chkbox;

public:
  InfoBoxesConfigWidget(WndForm &_dialog,
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
  void OnSwap(bool);

  void SetCurrentInfoBox(unsigned _current_preview);

  unsigned GetCurrentInfoBox() const {
    return current_preview;
  }

  InfoBoxFactory::Type GetContents(unsigned i) const {
    return data.contents[i];
  }

  void BeginEditing() {
    GetControl(BOX_NAME).BeginEditing();
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  bool Save(bool &changed) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    const Layout layout(rc, geometry);

    RowFormWidget::Show(layout.form);

    copy_button.MoveAndShow(layout.copy_button);
    paste_button.MoveAndShow(layout.paste_button);
    close_button.MoveAndShow(layout.close_button);
    swap_chkbox.MoveAndShow(layout.swap_chkbox);

    for (unsigned i = 0; i < previews.size(); ++i)
      previews[i].MoveAndShow(layout.info_boxes.positions[i]);
  }

  void Hide() noexcept override {
    RowFormWidget::Hide();

    copy_button.Hide();
    paste_button.Hide();
    close_button.Hide();
    swap_chkbox.Hide();

    for (auto &i : previews)
      i.Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    const Layout layout(rc, geometry);

    RowFormWidget::Move(layout.form);

    copy_button.Move(layout.copy_button);
    paste_button.Move(layout.paste_button);
    close_button.Move(layout.close_button);
    swap_chkbox.Move(layout.swap_chkbox);
  }

  bool SetFocus() noexcept override {
    GetGeneric(BOX_POS).SetFocus();
    return true;
  }

private:
  /* virtual methods from class DataFieldListener */

  void OnModified(DataField &df) noexcept override {
    if (IsDataField(BOX_POS, df)) {
      const DataFieldEnum &dfe = (const DataFieldEnum &)df;
      SetCurrentInfoBox(dfe.GetValue());
    } else if (IsDataField(BOX_NAME, df)) {
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
};

InfoBoxesConfigWidget::Layout::Layout(PixelRect rc,
                                      InfoBoxSettings::Geometry geometry)
{
  info_boxes = InfoBoxLayout::Calculate(rc, geometry);

  form = info_boxes.remaining;
  auto buttons = form.CutTopSafe(::Layout::GetMaximumControlHeight());

  swap_chkbox = copy_button = paste_button = close_button = buttons;

  int L = buttons.left;
  int W = buttons.right-L;

  swap_chkbox.left   = L;
  swap_chkbox.right  = copy_button.left  = L + (1*W)/4;
  copy_button.right  = paste_button.left = L + (2*W)/4;
  paste_button.right = close_button.left = L + (3*W)/4;
  close_button.right = L + W;
}

void
InfoBoxesConfigWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  /**
   * This set up the modal dialogue that infoboxes are assigned to a
   * panel.
   */
  const Layout layout(rc, geometry);

  AddText(_("Set"), nullptr,
          allow_name_change ? (const TCHAR *)data.name : gettext(data.name));
  SetReadOnly(SET_NAME, !allow_name_change);

  DataFieldEnum *dfe = new DataFieldEnum(this);
  for (unsigned i = 0; i < layout.info_boxes.count; ++i) {
    TCHAR label[32];
    _stprintf(label, _T("%u"), i + 1);
    dfe->addEnumText(label, i);
  }

  AddSpacer();
  Add(_("Position"), nullptr, dfe)->SetReadOnly();

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
  AddRemaining(std::make_unique<WndFrame>(form_parent, GetLook(), rc));

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  const auto &button_look = GetLook().button;
  copy_button.Create(parent, button_look, _("Copy set"), layout.copy_button,
                     button_style, [this](){ OnCopy(); });
  paste_button.Create(parent, button_look, _("Paste set"), layout.paste_button,
                      button_style, [this](){ OnPaste(); });
  close_button.Create(parent, button_look, _("Close"), layout.close_button,
                      button_style, dialog.MakeModalResultCallback(mrOK));

  const DialogLook &chbox_look = GetLook();
  swap_chkbox.Create(parent, chbox_look, _("Swap"), layout.swap_chkbox,
                        button_style, [this](bool s) { OnSwap(s); } );

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
InfoBoxesConfigWidget::Save(bool &changed_r) noexcept
{
  if (allow_name_change) {
    const auto *new_name = GetValueString(InfoBoxesConfigWidget::SET_NAME);
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
  DataFieldEnum &df = (DataFieldEnum &)GetDataField(BOX_NAME);
  WndFrame &description = (WndFrame &)GetRow(BOX_DESCR);
  description.SetText(df.GetHelp() != nullptr ? df.GetHelp() : _T(""));
}

void
InfoBoxesConfigWidget::RefreshEditContent()
{
  LoadValueEnum(BOX_NAME, data.contents[current_preview]);
}

void
InfoBoxesConfigWidget::OnSwap(bool checked)
{
  WndFrame &description = (WndFrame &)GetRow(BOX_DESCR);
  if ( checked ) {
     description.SetText(_T("Swap two Infoboxes. Moves selected InfoBox to the position you select next. The first position remains current selection for next move."));
   }
   else {
     description.SetText(_T("Swap finished"));
   }
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

  if (_current_preview == current_preview) {
    swap_chkbox.SetState(false);
    return;
  }

  bool is_checked = swap_chkbox.GetState();

  previews[current_preview].Invalidate();
  if ( is_checked ) {
    // Swap selected Infobox with the previous one.
    InfoBoxFactory::Type targetBox = data.contents[_current_preview];
    data.contents[_current_preview] = data.contents[current_preview];
    data.contents[current_preview] = targetBox;
    changed = true;
  }
  else {
    // Make selected InfoBox current.
    current_preview = _current_preview;
  }
  previews[current_preview].Invalidate();

  LoadValueEnum(BOX_POS, current_preview);

  RefreshEditContent();
  RefreshEditContentDescription();
}

bool
InfoBoxPreview::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  parent->SetCurrentInfoBox(i);
  return true;
}

bool
InfoBoxPreview::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  parent->BeginEditing();
  return true;
}

void
InfoBoxPreview::OnPaint(Canvas &canvas) noexcept
{
  const bool is_current = i == parent->GetCurrentInfoBox();

  if (is_current)
    canvas.Clear(COLOR_BLACK);
  else
    canvas.ClearWhite();

  canvas.SelectHollowBrush();
  canvas.SelectBlackPen();
  canvas.DrawRectangle(PixelRect{PixelSize{canvas.GetWidth() - 1, canvas.GetHeight() - 1}});

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
  canvas.DrawText({2, 2}, caption);
}

bool
dlgConfigInfoboxesShowModal(SingleWindow &parent,
                            const DialogLook &dialog_look,
                            const InfoBoxLook &_look,
                            InfoBoxSettings::Geometry geometry,
                            InfoBoxSettings::Panel &data_r,
                            bool allow_name_change)
{
  TWidgetDialog<InfoBoxesConfigWidget> dialog(WidgetDialog::Full{}, parent,
                                              dialog_look, nullptr);
  dialog.SetWidget(dialog, dialog_look, _look,
                   data_r, allow_name_change, geometry);

  dialog.ShowModal();
  return dialog.GetChanged();
}
