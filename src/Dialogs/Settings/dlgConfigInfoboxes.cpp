// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgConfigInfoboxes.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
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
#include "Panels/LayoutConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"

#include <cassert>

using namespace UI;

static constexpr unsigned GEOMETRY_INHERIT_ID =
    InfoBoxSettings::Panel::INHERIT_GEOMETRY;

std::vector<StaticEnumChoice> CreateInfoBoxGeometryListWithInherit() {
  std::vector<StaticEnumChoice> list;
  list.push_back({ GEOMETRY_INHERIT_ID, _("Inherit from global settings"), nullptr });
  for (const StaticEnumChoice* e = info_box_geometry_list; e && e->display_string != nullptr; ++e) {
    list.push_back(*e);
  }
  list.emplace_back(nullptr);
  return list;
}

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
    NAME, PAGE_GEOMETRY, INFOBOX, CONTENT, DESCRIPTION
  };

  struct Layout {
    InfoBoxLayout::Layout info_boxes;

    PixelRect form;

    PixelRect copy_button, paste_button, close_button;

    Layout(PixelRect rc, InfoBoxSettings::Geometry geometry);
  };

  WndForm &dialog;
  const InfoBoxLook &look;

  InfoBoxSettings::Panel &data;
  const bool allow_name_change;
  bool changed;

  InfoBoxSettings::Geometry geometry;

  StaticArray<InfoBoxPreview, InfoBoxSettings::Panel::MAX_CONTENTS> previews;
  unsigned current_preview;

  Button copy_button, paste_button, close_button;

  // Cached for re-layout when geometry changes
  ContainerWindow *parent_container = nullptr;
  WindowStyle preview_style;

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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  bool Save(bool &changed) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    const Layout layout(rc, geometry);

    RowFormWidget::Show(layout.form);

    copy_button.MoveAndShow(layout.copy_button);
    paste_button.MoveAndShow(layout.paste_button);
    close_button.MoveAndShow(layout.close_button);

    for (unsigned i = 0; i < previews.size(); ++i)
      previews[i].MoveAndShow(layout.info_boxes.positions[i]);
  }

  void Hide() noexcept override {
    RowFormWidget::Hide();

    copy_button.Hide();
    paste_button.Hide();
    close_button.Hide();

    for (auto &i : previews)
      i.Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    if (parent_container == nullptr) {
      RowFormWidget::Move(rc);
      return;
    }

    UpdateLayout(rc);
  }

  bool SetFocus() noexcept override {
    GetGeneric(INFOBOX).SetFocus();
    return true;
  }

private:
  void UpdateLayout(const PixelRect &rc) noexcept;
  void UpdateLayout() noexcept {
    UpdateLayout(GetCurrentClientRect());
  }
  PixelRect GetCurrentClientRect() const noexcept;
  /* virtual methods from class DataFieldListener */

  void OnModified(DataField &df) noexcept override {
    if (IsDataField(PAGE_GEOMETRY, df)) {
      const DataFieldEnum &dfe = static_cast<const DataFieldEnum &>(df);
      InfoBoxSettings::Geometry new_geometry =
          dfe.GetValue() == GEOMETRY_INHERIT_ID
              ? CommonInterface::GetUISettings().info_boxes.geometry
              : (InfoBoxSettings::Geometry)dfe.GetValue();

      if (new_geometry == geometry)
        return;

      geometry = new_geometry;
      UpdateLayout();
    } else if (IsDataField(INFOBOX, df)) {
      const DataFieldEnum &dfe = static_cast<const DataFieldEnum &>(df);
      SetCurrentInfoBox(dfe.GetValue());
    } else if (IsDataField(CONTENT, df)) {
      const DataFieldEnum &dfe = static_cast<const DataFieldEnum &>(df);

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

  copy_button = paste_button = close_button = buttons;
  copy_button.right = paste_button.left =
    (2 * buttons.left + buttons.right) / 3;
  paste_button.right = close_button.left =
    (buttons.left + 2 * buttons.right) / 3;
}

void
InfoBoxesConfigWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const Layout layout(rc, geometry);
  parent_container = &parent;

  AddText(_("Name"), nullptr,
          allow_name_change ? (const TCHAR *)data.name : gettext(data.name));
  SetReadOnly(NAME, !allow_name_change);

  const unsigned geom_id = (data.geometry == InfoBoxSettings::Panel::INHERIT_GEOMETRY)
    ? GEOMETRY_INHERIT_ID
    : static_cast<unsigned>(static_cast<InfoBoxSettings::Geometry>(data.geometry));
  DataFieldEnum *geom_dfe = new DataFieldEnum(this);
  geom_dfe->AddChoice(GEOMETRY_INHERIT_ID, _("Inherit from global settings"));
  geom_dfe->AddChoices(info_box_geometry_list);
  geom_dfe->SetValue(geom_id);
  Add(_("Page geometry"),
      _("Override InfoBox geometry only for this page. If unset, global setting is used."),
      geom_dfe);

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
  AddRemaining(std::make_unique<WndFrame>(form_parent, GetLook(), rc));

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  const auto &button_look = GetLook().button;
  copy_button.Create(parent, button_look, _("Copy Set"), layout.copy_button,
                     button_style, [this](){ OnCopy(); });
  paste_button.Create(parent, button_look, _("Paste Set"), layout.paste_button,
                      button_style, [this](){ OnPaste(); });
  close_button.Create(parent, button_look, _("Close"), layout.close_button,
                      button_style, dialog.MakeModalResultCallback(mrOK));

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

PixelRect
InfoBoxesConfigWidget::GetCurrentClientRect() const noexcept
{
  return parent_container != nullptr
    ? parent_container->GetClientRect()
    : GetWindow().GetClientRect();
}

void
InfoBoxesConfigWidget::UpdateLayout(const PixelRect &rc) noexcept
{
  if (parent_container == nullptr)
    return;

  const Layout layout(rc, geometry);

  // Update InfoBox selector to match new count
  auto &ib_selector = static_cast<DataFieldEnum &>(GetDataField(INFOBOX));
  const unsigned new_count = layout.info_boxes.count;
  if (ib_selector.Count() != new_count) {
    ib_selector.ClearChoices();
    for (unsigned i = 0; i < new_count; ++i) {
      TCHAR label[32];
      _stprintf(label, _T("%u"), i + 1);
      ib_selector.addEnumText(label, i);
    }
    if (current_preview >= new_count)
      current_preview = 0;
    LoadValueEnum(INFOBOX, current_preview);
  }

  // Ensure enough preview windows exist
  if (previews.size() < new_count) {
    const unsigned before = previews.size();
    previews.resize(new_count);
    for (unsigned i = before; i < new_count; ++i) {
      previews[i].SetParent(*this, i);
      // Create missing preview windows (create style locally for simplicity)
      WindowStyle style; style.Hide();
      previews[i].Create(*parent_container, layout.info_boxes.positions[i], style);
    }
  }

  // Move/show previews for new layout
  for (unsigned i = 0; i < new_count; ++i)
    previews[i].MoveAndShow(layout.info_boxes.positions[i]);

  // Hide any surplus previews
  for (unsigned i = new_count; i < previews.size(); ++i)
    previews[i].Hide();

  // Reposition the form and buttons
  RowFormWidget::Move(layout.form);
  copy_button.Move(layout.copy_button);
  paste_button.Move(layout.paste_button);
  close_button.Move(layout.close_button);
}

bool
InfoBoxesConfigWidget::Save(bool &changed_r) noexcept
{
  if (allow_name_change) {
    const auto *new_name = GetValueString(InfoBoxesConfigWidget::NAME);
    if (!StringIsEqual(new_name, data.name)) {
      data.name = new_name;
      changed = true;
    }
  }

  unsigned id = GetValueEnum(PAGE_GEOMETRY);
  if (id == GEOMETRY_INHERIT_ID) {
    if (data.geometry != InfoBoxSettings::Panel::INHERIT_GEOMETRY) {
      data.geometry = InfoBoxSettings::Panel::INHERIT_GEOMETRY;
      changed = true;
    }
  } else {
    auto new_geom = static_cast<InfoBoxSettings::Geometry>(id);
    if (data.geometry != static_cast<uint8_t>(new_geom)) {
      data.geometry = static_cast<uint8_t>(new_geom);
      changed = true;
    }
  }

  changed_r = changed;
  return true;
}

void
InfoBoxesConfigWidget::RefreshEditContentDescription()
{
  DataFieldEnum &df = static_cast<DataFieldEnum &>(GetDataField(CONTENT));
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

  if(ShowMessageBox(_("Overwrite all infoboxes in this set?"), _("InfoBox paste set"),
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
