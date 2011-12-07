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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Form/TabMenu.hpp"
#include "Screen/Busy.hpp"
#include "Screen/Key.h"
#include "Form/CheckBox.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Profile/Profile.hpp"
#include "DataField/FileReader.hpp"
#include "LogFile.hpp"
#include "Util/Macros.hpp"
#include "ConfigPanels/ConfigPanel.hpp"
#include "ConfigPanels/PagesConfigPanel.hpp"
#include "ConfigPanels/PolarConfigPanel.hpp"
#include "ConfigPanels/UnitsConfigPanel.hpp"
#include "ConfigPanels/TimeConfigPanel.hpp"
#include "ConfigPanels/LoggerConfigPanel.hpp"
#include "ConfigPanels/DevicesConfigPanel.hpp"
#include "ConfigPanels/AirspaceConfigPanel.hpp"
#include "ConfigPanels/SiteConfigPanel.hpp"
#include "ConfigPanels/MapDisplayConfigPanel.hpp"
#include "ConfigPanels/WaypointDisplayConfigPanel.hpp"
#include "ConfigPanels/SymbolsConfigPanel.hpp"
#include "ConfigPanels/TerrainDisplayConfigPanel.hpp"
#include "ConfigPanels/GlideComputerConfigPanel.hpp"
#include "ConfigPanels/SafetyFactorsConfigPanel.hpp"
#include "ConfigPanels/RouteConfigPanel.hpp"
#include "ConfigPanels/InterfaceConfigPanel.hpp"
#include "ConfigPanels/LayoutConfigPanel.hpp"
#include "ConfigPanels/GaugesConfigPanel.hpp"
#include "ConfigPanels/VarioConfigPanel.hpp"
#include "ConfigPanels/TaskRulesConfigPanel.hpp"
#include "ConfigPanels/TaskDefaultsConfigPanel.hpp"
#include "ConfigPanels/InfoBoxesConfigPanel.hpp"
#include "ConfigPanels/ExperimentalConfigPanel.hpp"

#ifdef HAVE_TRACKING
#include "ConfigPanels/TrackingConfigPanel.hpp"
#endif

#include <assert.h>

static unsigned current_page;
static WndForm *wf = NULL;

static TabMenuControl* wTabMenu = NULL;

const TCHAR *main_menu_captions[] = {
  N_("Site Files"),
  N_("Map Display"),
  N_("Glide Computer"),
  N_("Gauges"),
  N_("Task Defaults"),
  N_("Look"),
  N_("Setup"),
};

static const TabMenuControl::PageItem pages[] = {
  {N_("Site Files"), 0, NULL, NULL, NULL, CreateSiteConfigPanel },
  {N_("Orientation"), 1, NULL, NULL, NULL, CreateMapDisplayConfigPanel },
  {N_("Elements"), 1, NULL, NULL, NULL, CreateSymbolsConfigPanel },
  {N_("Waypoints"), 1, NULL, NULL, NULL, CreateWaypointDisplayConfigPanel },
  {N_("Terrain"), 1, NULL, NULL, NULL, CreateTerrainDisplayConfigPanel },
  {N_("Airspace"), 1, NULL, NULL, NULL, CreateAirspaceConfigPanel },
  {N_("Safety Factors"), 2, NULL, NULL, NULL, CreateSafetyFactorsConfigPanel },
  {N_("Glide Computer"), 2, NULL, NULL, NULL, CreateGlideComputerConfigPanel },
  {N_("Route"), 2, NULL, NULL, NULL, CreateRouteConfigPanel },
  {N_("FLARM, Other"), 3, NULL, NULL, NULL, CreateGaugesConfigPanel },
  {N_("Vario"), 3, NULL, NULL, NULL, CreateVarioConfigPanel },
  {N_("Task Rules"), 4, NULL, NULL, NULL, CreateTaskRulesConfigPanel },
  {N_("Turnpoint Types"), 4, NULL, NULL, NULL, CreateTaskDefaultsConfigPanel },
  {N_("Language, Input"), 5, NULL, NULL, NULL, CreateInterfaceConfigPanel },
  {N_("Screen Layout"), 5, NULL, NULL, NULL, CreateLayoutConfigPanel },
  {N_("InfoBox Pages"), 5, NULL, NULL, NULL, CreatePagesConfigPanel },
  {N_("InfoBox Modes"), 5, NULL, NULL, NULL, CreateInfoBoxesConfigPanel },
  {N_("Devices"), 6, NULL, NULL, NULL, CreateDevicesConfigPanel },
  {N_("Polar"), 6, PolarConfigPanel::PreShow, PolarConfigPanel::PreHide, N_("IDR_XML_POLARCONFIGPANEL")},
  {N_("Logger"), 6, NULL, NULL, NULL, CreateLoggerConfigPanel },
  {N_("Units"), 6, NULL, NULL, NULL, CreateUnitsConfigPanel },
  // Important: all pages after Units in this list must not have data fields that are
  // unit-dependent because they will be saved after their units may have changed.
  // ToDo: implement API that controls order in which pages are saved
  {N_("Time"), 6, NULL, NULL, NULL, CreateTimeConfigPanel },
#ifdef HAVE_TRACKING
  {N_("Tracking"), 6, NULL, NULL, NULL, CreateTrackingConfigPanel },
#endif
#ifdef HAVE_MODEL_TYPE
  {N_("Experimental Features"), 6, NULL, NULL, NULL, CreateExperimentalConfigPanel, },
#endif
};

WndForm &
ConfigPanel::GetForm()
{
  assert(wf != NULL);

  return *wf;
}

static void
OnUserLevel(CheckBoxControl &control)
{
  Profile::Set(szProfileUserLevel, control.get_checked());
  wf->FilterAdvanced(control.get_checked());
}

static void
OnNextClicked(gcc_unused WndButton &button)
{
  wTabMenu->NextPage();
}

static void
OnPrevClicked(gcc_unused WndButton &button)
{
  wTabMenu->PreviousPage();
}

/**
 * close dialog from menu page.  from content, goes to menu page
 */
static void
OnCloseClicked(gcc_unused WndButton &button)
{
  if (wTabMenu->IsCurrentPageTheMenu())
    wf->SetModalResult(mrOK);
  else
    wTabMenu->GotoMenuPage();
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    if (wTabMenu->IsCurrentPageTheMenu())
      wTabMenu->HighlightPreviousMenuItem();
    else {
      wTabMenu->PreviousPage();
      ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    }
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    if (wTabMenu->IsCurrentPageTheMenu())
      wTabMenu->HighlightNextMenuItem();
    else {
      wTabMenu->NextPage();
      ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    }
    return true;

  default:
    return false;
  }
}

static void
setVariables()
{
  PolarConfigPanel::Init(wf);
}

static void
PrepareConfigurationMenu()
{
  assert (wf != NULL);

  wTabMenu = (TabMenuControl*)wf->FindByName(_T("TabMenu"));
  assert(wTabMenu != NULL);
  wTabMenu->InitMenu(pages,
                     ARRAY_SIZE(pages),
                     main_menu_captions,
                     ARRAY_SIZE(main_menu_captions));
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(PolarConfigPanel::OnLoadInternal),
  DeclareCallBackEntry(PolarConfigPanel::OnLoadFromFile),
  DeclareCallBackEntry(PolarConfigPanel::OnExport),
  DeclareCallBackEntry(PolarConfigPanel::OnFieldData),
  DeclareCallBackEntry(OnUserLevel),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
PrepareConfigurationDialog()
{
  gcc_unused ScopeBusyIndicator busy;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_CONFIGURATION_L") :
                                      _T("IDR_XML_CONFIGURATION"));
  if (wf == NULL)
    return;

  wf->SetKeyDownNotify(FormKeyDown);

  bool expert_mode = false;
  Profile::Get(szProfileUserLevel, expert_mode);

  CheckBox *cb = (CheckBox *)wf->FindByName(_T("Expert"));
  cb->set_checked(expert_mode);
  wf->FilterAdvanced(expert_mode);

  PrepareConfigurationMenu();

  setVariables();

  wTabMenu->GotoMenuPage();
  /* restore last selected menu item */
  static bool Initialized = false;
  if (!Initialized)
    Initialized = true;
  else
    wTabMenu->SetLastContentPage(current_page);
}

void dlgConfigurationShowModal(void)
{
  PrepareConfigurationDialog();

  wf->ShowModal();

  /* save page number for next time this dialog is opened */
  current_page = wTabMenu->GetLastContentPage();

  // TODO enhancement: implement a cancel button that skips all this
  // below after exit.
  bool changed = false;
  bool requirerestart = false;
  changed |= PolarConfigPanel::Save();
  wTabMenu->Save(changed, requirerestart);

  if (changed) {
    Profile::Save();
    LogDebug(_T("Configuration: Changes saved"));
    if (requirerestart)
      MessageBoxX(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                  _T(""), MB_OK);
  }

  delete wf;
  wf = NULL;
}
