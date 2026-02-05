// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InternalLink.hpp"
#include "util/StringCompare.hxx"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Dialogs/Plane/PlaneDialogs.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Settings/Panels/SiteConfigPanel.hpp"
#include "Dialogs/Settings/Panels/LoggerConfigPanel.hpp"
#include "Dialogs/Settings/Panels/TimeConfigPanel.hpp"
#include "Dialogs/Settings/Panels/InfoBoxesConfigPanel.hpp"
#include "Dialogs/Settings/Panels/PagesConfigPanel.hpp"
#include "Dialogs/Settings/Panels/WeGlideConfigPanel.hpp"
#include "Widget/Widget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Profile/Profile.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Language/Language.hpp"

/**
 * Helper to show a config panel in a dialog.
 */
static void
ShowConfigPanel(const TCHAR *title,
                std::unique_ptr<Widget> (*create_panel)()) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, title);
  auto panel = create_panel();
  dialog.FinishPreliminary(std::move(panel));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  if (dialog.GetChanged())
    Profile::Save();
}

bool
HandleInternalLink(const char *url) noexcept
{
  // Must start with xcsoar://
  if (!StringStartsWith(url, "xcsoar://"))
    return false;

  const char *path = url + 9;  // Skip "xcsoar://"

  // Config dialogs
  if (StringIsEqual(path, "config/devices")) {
    if (backend_components != nullptr &&
        backend_components->device_blackboard != nullptr) {
      ShowDeviceList(*backend_components->device_blackboard,
                     backend_components->devices.get());
    }
    return true;
  }

  if (StringIsEqual(path, "config/planes")) {
    dlgPlanesShowModal();
    return true;
  }

  if (StringIsEqual(path, "config/site-files")) {
    ShowConfigPanel(_("Site Files"), CreateSiteConfigPanel);
    return true;
  }

  if (StringIsEqual(path, "config/logger")) {
    ShowConfigPanel(_("Logger"), CreateLoggerConfigPanel);
    return true;
  }

  if (StringIsEqual(path, "config/time")) {
    ShowConfigPanel(_("Time"), CreateTimeConfigPanel);
    return true;
  }

  if (StringIsEqual(path, "config/infoboxes")) {
    ShowConfigPanel(_("InfoBox Sets"), CreateInfoBoxesConfigPanel);
    return true;
  }

  if (StringIsEqual(path, "config/pages")) {
    ShowConfigPanel(_("Pages"), CreatePagesConfigPanel);
    return true;
  }

  if (StringIsEqual(path, "config/weglide")) {
    ShowConfigPanel(_("WeGlide"), CreateWeGlideConfigPanel);
    return true;
  }

  // Action dialogs
  if (StringIsEqual(path, "dialog/checklist")) {
    dlgChecklistShowModal();
    return true;
  }

  if (StringIsEqual(path, "dialog/flight")) {
    dlgBasicSettingsShowModal();
    return true;
  }

  if (StringIsEqual(path, "dialog/wind")) {
    ShowWindSettingsDialog();
    return true;
  }

  if (StringIsEqual(path, "dialog/task")) {
    dlgTaskManagerShowModal();
    return true;
  }

  if (StringIsEqual(path, "dialog/analysis")) {
    dlgAnalysisShowModal(*CommonInterface::main_window,
                         CommonInterface::main_window->GetLook(),
                         CommonInterface::Full(),
                         *backend_components->glide_computer,
                         data_components->airspaces.get(),
                         data_components->terrain.get());
    return true;
  }

  if (StringIsEqual(path, "dialog/status")) {
    dlgStatusShowModal(-1);
    return true;
  }

  return false;
}
