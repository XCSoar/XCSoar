// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ActionWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"

#include <cassert>
#include <stdio.h>

/**
 * This modal result will trigger the "Switch InfoBox" dialog.
 */
static constexpr int SWITCH_INFO_BOX = 100;

/**
 * Pointer to the currently open InfoBox dialog, or nullptr if none is open.
 */
static WndForm *current_dialog = nullptr;

/**
 * ID of the InfoBox that owns the current dialog, or -1 if none.
 */
static int current_dialog_id = -1;

void
dlgInfoBoxAccessShowModeless(const int id, const InfoBoxPanel *panels)
{
  assert (id > -1);

  /* Close any existing InfoBox dialog before opening a new one */
  dlgInfoBoxAccessClose();

  const InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = CommonInterface::GetUIState().panel_index;
  const InfoBoxSettings::Panel &panel = settings.panels[panel_index];
  const InfoBoxFactory::Type old_type = panel.contents[id];

  const DialogLook &look = UIGlobals::GetDialogLook();

  PixelRect form_rc = InfoBoxManager::layout.remaining;

  TWidgetDialog<TabWidget>
    dialog(UIGlobals::GetMainWindow(), look, form_rc,
           gettext(InfoBoxFactory::GetName(old_type)),
           nullptr);
  dialog.SetWidget(TabWidget::Orientation::HORIZONTAL);
  dialog.PrepareWidget();
  auto &tab_widget = dialog.GetWidget();
  
  /* Track the current dialog and its owner */
  current_dialog = &dialog;
  current_dialog_id = id;
  if (panels != nullptr) {
    for (; panels->load != nullptr; ++panels) {
      assert(panels->name != nullptr);

      auto widget = panels->load(id);

      if (widget == NULL)
        continue;
      tab_widget.AddTab(std::move(widget), gettext(panels->name));
    }
  }

  tab_widget.AddTab(std::make_unique<ActionWidget>(dialog.MakeModalResultCallback(mrOK)),
                    _("Close"));

  const PixelRect client_rc = dialog.GetClientAreaWindow().GetClientRect();
  const PixelSize max_size = tab_widget.GetMaximumSize();
  if (max_size.height < client_rc.GetHeight()) {
    form_rc.top += client_rc.GetHeight() - max_size.height;
    dialog.Move(form_rc);
  }

  dialog.SetModeless();

  /* When InfoBox panel opens, adjust bottom margin to make room */
  GlueMapWindow *map = UIGlobals::GetMap();
  if (map != nullptr) {
    unsigned dialog_height = form_rc.GetHeight();

    if (dialog_height > 0)
      map->SetBottomMargin(dialog_height);
  }

  int result = dialog.ShowModal();

  if (map != nullptr)
    map->SetBottomMargin(0);

  current_dialog = nullptr;
  current_dialog_id = -1;

  if (result == SWITCH_INFO_BOX)
    InfoBoxManager::ShowInfoBoxPicker(id);
}

void
dlgInfoBoxAccessClose() noexcept
{
  if (current_dialog != nullptr) {
    /* Prevent focus restoration to the InfoBox that owned this dialog
       by signaling the dialog to close via SetModalResult(mrCancel) and
       clearing current_dialog/current_dialog_id so ownership and focus
       won't be restored. The dialog is destroyed later by the caller. */
    current_dialog->SetModalResult(mrCancel);
    current_dialog = nullptr;
    current_dialog_id = -1;

    /* Restore map scale position when panel closes */
    GlueMapWindow *map = UIGlobals::GetMap();
    if (map != nullptr)
      map->SetBottomMargin(0);
  }
}

bool
dlgInfoBoxAccessCloseOthers(int id) noexcept
{
  /* Only close if a different InfoBox owns the dialog */
  if (current_dialog != nullptr && current_dialog_id != id) {
    dlgInfoBoxAccessClose();
    return true;
  }
  return false;
}
