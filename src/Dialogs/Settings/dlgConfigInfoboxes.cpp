// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgConfigInfoboxes.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/Widget.hpp"
#include "InfoBoxes/InfoBoxArrangeWindow.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

using namespace UI;

static InfoBoxSettings::Panel clipboard;
static unsigned clipboard_size;

class InfoBoxesConfigWidget final : public NullWidget {
  struct Layout {
    InfoBoxLayout::Layout info_boxes;

    Layout() = default;
    Layout(PixelRect rc, InfoBoxSettings::Geometry geometry);
  };

  /** the InfoBoxes of this set; it reports every change back */
  class ArrangeWindow final : public InfoBoxArrangeWindow {
    InfoBoxesConfigWidget &widget;

  public:
    ArrangeWindow(InfoBoxesConfigWidget &_widget,
                  const DialogLook &_dialog_look,
                  const InfoBoxLook &_look) noexcept
      :InfoBoxArrangeWindow(_look, _dialog_look, Style::DIALOG),
       widget(_widget) {}

  protected:
    /* virtual methods from class InfoBoxArrangeWindow */
    void OnArrangeModified() noexcept override {
      widget.changed = true;
    }
  };

  WndForm &dialog;

  InfoBoxSettings::Panel &data;
  const bool allow_name_change;
  bool changed = false;

  const InfoBoxSettings::Geometry geometry;

  /** the caption of the button which renames the set */
  StaticString<64> name_caption;

  Layout layout;

  ArrangeWindow arrange;

  /** the button which pastes the clipboard */
  unsigned paste_button;

public:
  InfoBoxesConfigWidget(WndForm &_dialog,
                        const DialogLook &dialog_look,
                        const InfoBoxLook &_look,
                        InfoBoxSettings::Panel &_data,
                        bool _allow_name_change,
                        InfoBoxSettings::Geometry _geometry)
    :dialog(_dialog),
     data(_data),
     allow_name_change(_allow_name_change),
     geometry(_geometry),
     arrange(*this, dialog_look, _look) {}

private:
  void RefreshPasteButton() noexcept {
    arrange.SetButtonEnabled(paste_button, clipboard_size > 0);
  }

  void OnRename() noexcept;
  void OnCopy() noexcept;
  void OnPaste() noexcept;

  /** Recalculate the layout for @p rc and hand it to #arrange. */
  void UpdateLayout(const PixelRect &rc) noexcept {
    layout = Layout(rc, geometry);
    arrange.SetLayout(layout.info_boxes, layout.info_boxes.remaining);
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  bool Save(bool &changed) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    UpdateLayout(rc);
    arrange.MoveAndShow(rc);
  }

  void Hide() noexcept override {
    arrange.Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    UpdateLayout(rc);
    arrange.Move(rc);
  }

  bool SetFocus() noexcept override {
    arrange.SetFocus();
    return true;
  }
};

InfoBoxesConfigWidget::Layout::Layout(PixelRect rc,
                                      InfoBoxSettings::Geometry geometry)
{
  const unsigned title_scale =
    CommonInterface::GetUISettings().info_boxes.scale_title_font;
  info_boxes = InfoBoxLayout::Calculate(rc, geometry, title_scale);
}

void
InfoBoxesConfigWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  UpdateLayout(rc);

  arrange.SetExtraHelp(allow_name_change
                       ? _("The button with the name of the set renames it, "
                           "Copy remembers all its InfoBoxes, and Paste "
                           "replaces the InfoBoxes of another set with "
                           "them.")
                       : _("Copy remembers all InfoBoxes of this set, Paste "
                           "replaces the InfoBoxes of another set with "
                           "them."));

  /* the upper row changes the set, the lower one acts on the dialogue */
  name_caption = gettext(data.name);
  const unsigned name_button =
    arrange.AddButton(1, name_caption, [this]{ OnRename(); });
  arrange.AddButton(1, _("Copy"), [this]{ OnCopy(); });
  paste_button = arrange.AddButton(1, _("Paste"), [this]{ OnPaste(); });

  arrange.AddButton(_("Help"), [this]{ arrange.ShowHelp(); });
  arrange.AddButton(_("Close"), dialog.MakeModalResultCallback(mrOK));

  arrange.SetPanel(data);
  arrange.Create(parent, rc);
  arrange.FocusSlot(0);

  /* only the sets the user has made himself can be renamed */
  arrange.SetButtonEnabled(name_button, allow_name_change);
  RefreshPasteButton();
}

bool
InfoBoxesConfigWidget::Save(bool &changed_r) noexcept
{
  changed_r = changed;
  return true;
}

void
InfoBoxesConfigWidget::OnRename() noexcept
{
  if (!TextEntryDialog(data.name, _("Name")))
    return;

  name_caption = data.name;
  changed = true;
  arrange.Invalidate();
}

void
InfoBoxesConfigWidget::OnCopy() noexcept
{
  clipboard = data;
  clipboard_size = InfoBoxSettings::Panel::MAX_CONTENTS;

  RefreshPasteButton();
}

void
InfoBoxesConfigWidget::OnPaste() noexcept
{
  if (clipboard_size == 0)
    return;

  if (ShowMessageBox(_("Overwrite all InfoBoxes in this set?"),
                     _("InfoBox paste set"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  for (unsigned item = 0; item < clipboard_size; item++) {
    InfoBoxFactory::Type content = clipboard.contents[item];
    if (content >= InfoBoxFactory::NUM_TYPES)
      continue;

    data.contents[item] = content;
  }

  changed = true;
  arrange.Invalidate();
}

bool
dlgConfigInfoboxesShowModal(SingleWindow &parent,
                            const DialogLook &dialog_look,
                            const InfoBoxLook &_look,
                            InfoBoxSettings::Geometry geometry,
                            InfoBoxSettings::Panel &data_r,
                            bool allow_name_change)
{
  /* the dialogue edits the set in place, so Escape puts it back the
     way it was */
  const InfoBoxSettings::Panel saved = data_r;

  TWidgetDialog<InfoBoxesConfigWidget> dialog(WidgetDialog::Full{}, parent,
                                              dialog_look, nullptr);
  dialog.SetWidget(dialog, dialog_look, _look,
                   data_r, allow_name_change, geometry);

  if (dialog.ShowModal() != mrOK) {
    data_r = saved;
    return false;
  }

  return dialog.GetChanged();
}
