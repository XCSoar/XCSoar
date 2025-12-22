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

  bool found_setup = false;

  if (panels != nullptr) {
    for (; panels->load != nullptr; ++panels) {
      assert(panels->name != nullptr);

      auto widget = panels->load(id);

      if (widget == NULL)
        continue;

      if (!found_setup && StringIsEqual(panels->name, _T("Setup"))) {
        /* add a "Switch InfoBox" button to the "Setup" tab -
           kludge! */
        found_setup = true;

        PixelRect button_rc;
        button_rc.left = 0;
        button_rc.top = 0;
        button_rc.right = Layout::Scale(60);
        button_rc.bottom = std::max(2u * Layout::GetMinimumControlHeight(),
                                    Layout::GetMaximumControlHeight());

        auto button = std::make_unique<ButtonWidget>(look.button,
                                                     _("Switch InfoBox"),
                                                     dialog.MakeModalResultCallback(SWITCH_INFO_BOX));

        widget = std::make_unique<TwoWidgets>(std::move(widget),
                                              std::move(button),
                                              false);
      }

      tab_widget.AddTab(std::move(widget), gettext(panels->name));
    }
  }

  if (!found_setup) {
    /* the InfoBox did not provide a "Setup" tab - create a default
       one that allows switching the contents */
    tab_widget.AddTab(std::make_unique<ActionWidget>(dialog.MakeModalResultCallback(SWITCH_INFO_BOX)),
                      _("Switch InfoBox"));
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
  int result = dialog.ShowModal();
  
  /* Clear the dialog pointer and ID after it closes */
  current_dialog = nullptr;
  current_dialog_id = -1;
  
  /* If dialog was closed by clicking outside (result == 0),
     check which InfoBox now has focus and open its dialog */
  if (result == 0) {
    /* The InfoBox that was clicked should now have focus
       - trigger its dialog to open by calling ShowInfoBoxPicker with -1
       which opens the picker for the focused InfoBox */
    InfoBoxManager::ShowInfoBoxPicker(-1);
  }

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
