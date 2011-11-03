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
#include "ConfigPanels/PagesConfigPanel.hpp"
#include "ConfigPanels/PolarConfigPanel.hpp"
#include "ConfigPanels/UnitsConfigPanel.hpp"
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
  {N_("Site Files"), 0, SiteConfigPanel::PreShow, SiteConfigPanel::PreHide, N_("IDR_XML_SITECONFIGPANEL")},
  {N_("Orientation"), 1, NULL, NULL, N_("IDR_XML_MAPDISPLAYCONFIGPANEL")},
  {N_("Elements"), 1, NULL, NULL, N_("IDR_XML_SYMBOLSCONFIGPANEL")},
  {N_("Waypoint"), 1, NULL, NULL, N_("IDR_XML_WAYPOINTDISPLAYCONFIGPANEL")},
  {N_("Terrain"), 1, NULL, NULL, N_("IDR_XML_TERRAINDISPLAYCONFIGPANEL")},
  {N_("Airspace"), 1, NULL, NULL, N_("IDR_XML_AIRSPACECONFIGPANEL")},
  {N_("Safety Factors"), 2, NULL, NULL, N_("IDR_XML_SAFETYFACTORSCONFIGPANEL")},
  {N_("Glide Computer"), 2, NULL, NULL, N_("IDR_XML_GLIDECOMPUTERCONFIGPANEL")},
  {N_("Route"), 2, NULL, NULL, N_("IDR_XML_ROUTECONFIGPANEL")},
  {N_("FLARM, Other"), 3, NULL, NULL, N_("IDR_XML_GAUGESCONFIGPANEL")},
  {N_("Vario"), 3, NULL, NULL, N_("IDR_XML_VARIOCONFIGPANEL")},
  {N_("Task Rules"), 4, NULL, NULL, N_("IDR_XML_TASKRULESCONFIGPANEL")},
  {N_("Turnpoint Types"), 4, NULL, NULL, N_("IDR_XML_TASKDEFAULTSCONFIGPANEL")},
  {N_("Language, Input"), 5, InterfaceConfigPanel::PreShow, InterfaceConfigPanel::PreHide, N_("IDR_XML_INTERFACECONFIGPANEL")},
  {N_("Screen Layout"), 5, NULL, NULL, N_("IDR_XML_LAYOUTCONFIGPANEL")},
  {N_("InfoBox Pages"), 5, NULL, NULL, N_("IDR_XML_PAGESCONFIGPANEL")},
  {N_("InfoBox Modes"), 5, NULL, NULL, N_("IDR_XML_INFOBOXESCONFIGPANEL")},
  {N_("Devices"), 6, NULL, NULL, N_("IDR_XML_DEVICESCONFIGPANEL")},
  {N_("Polar"), 6, PolarConfigPanel::PreShow, PolarConfigPanel::PreHide, N_("IDR_XML_POLARCONFIGPANEL")},
  {N_("Logger"), 6, NULL, NULL, N_("IDR_XML_LOGGERCONFIGPANEL")},
  {N_("Units"), 6, NULL, NULL, N_("IDR_XML_UNITSCONFIGPANEL")},
#ifdef HAVE_TRACKING
  {N_("Tracking"), 6, NULL, NULL, N_("IDR_XML_TRACKINGCONFIGPANEL")},
#endif
#ifdef HAVE_MODEL_TYPE
  {N_("Experimental Features"), 6, NULL, NULL, N_("IDR_XML_EXPERIMENTALCONFIGPANEL")},
#endif
};

static void
OnUserLevel(CheckBoxControl &control)
{
  Profile::Set(szProfileUserLevel, control.get_checked());
  wf->FilterAdvanced(control.get_checked());
  MapDisplayConfigPanel::UpdateVisibilities();
  WaypointDisplayConfigPanel::UpdateVisibilities();
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
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    wTabMenu->PreviousPage();
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    wTabMenu->NextPage();
    return true;

  default:
    return false;
  }
}

static void
setVariables()
{
#ifdef HAVE_TRACKING
  TrackingConfigPanel::Init(wf, CommonInterface::SettingsComputer().tracking);
#endif
  PolarConfigPanel::Init(wf);
  UnitsConfigPanel::Init(wf);
  LoggerConfigPanel::Init(wf);
  DevicesConfigPanel::Init(wf);
  AirspaceConfigPanel::Init(wf, CommonInterface::SettingsComputer().airspace,
                            CommonInterface::SettingsMap().airspace);
  SiteConfigPanel::Init(wf);
  MapDisplayConfigPanel::Init(wf);
  WaypointDisplayConfigPanel::Init(wf);
  SymbolsConfigPanel::Init(wf);
  TerrainDisplayConfigPanel::Init(wf, CommonInterface::SettingsMap());
  GlideComputerConfigPanel::Init(wf);
  SafetyFactorsConfigPanel::Init(wf);
  RouteConfigPanel::Init(wf);
  InterfaceConfigPanel::Init(wf);
  LayoutConfigPanel::Init(wf);
  GaugesConfigPanel::Init(wf);
  VarioConfigPanel::Init(wf);
  TaskRulesConfigPanel::Init(wf);
  TaskDefaultsConfigPanel::Init(wf);
  InfoBoxesConfigPanel::Init(wf);
  ExperimentalConfigPanel::Init(wf);
  PagesConfigPanel::Init(wf);
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
  DeclareCallBackEntry(AirspaceConfigPanel::OnAirspaceColoursClicked),
  DeclareCallBackEntry(AirspaceConfigPanel::OnAirspaceModeClicked),
  DeclareCallBackEntry(AirspaceConfigPanel::OnAirspaceDisplay),
  DeclareCallBackEntry(AirspaceConfigPanel::OnAirspaceWarning),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(SymbolsConfigPanel::OnTrailLength),
  DeclareCallBackEntry(TerrainDisplayConfigPanel::OnEnableTerrain),
  DeclareCallBackEntry(RouteConfigPanel::OnRouteMode),
  DeclareCallBackEntry(RouteConfigPanel::OnReachMode),
  DeclareCallBackEntry(TaskDefaultsConfigPanel::OnStartType),
  DeclareCallBackEntry(TaskDefaultsConfigPanel::OnFinishType),
  DeclareCallBackEntry(DevicesConfigPanel::OnDeviceAPort),
  DeclareCallBackEntry(DevicesConfigPanel::OnDeviceBPort),
  DeclareCallBackEntry(DevicesConfigPanel::OnDeviceAData),
  DeclareCallBackEntry(DevicesConfigPanel::OnDeviceBData),
  DeclareCallBackEntry(MapDisplayConfigPanel::OnShiftTypeData),
  DeclareCallBackEntry(PolarConfigPanel::OnLoadInternal),
  DeclareCallBackEntry(PolarConfigPanel::OnLoadFromFile),
  DeclareCallBackEntry(PolarConfigPanel::OnExport),
  DeclareCallBackEntry(PolarConfigPanel::OnFieldData),
  DeclareCallBackEntry(UnitsConfigPanel::OnLoadPreset),
  DeclareCallBackEntry(UnitsConfigPanel::OnFieldData),
  DeclareCallBackEntry(UnitsConfigPanel::OnUTCData),
  DeclareCallBackEntry(WaypointDisplayConfigPanel::OnRenderingTypeData),
#ifdef HAVE_TRACKING
  DeclareCallBackEntry(TrackingConfigPanel::OnLT24Enabled),
#endif
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

  /* restore previous page */
  static bool Initialized = false;
  if (!Initialized) {
    current_page = wTabMenu->GotoMenuPage();
    Initialized = true;
  }
  wTabMenu->SetCurrentPage(current_page);
}

void dlgConfigurationShowModal(void)
{
  PrepareConfigurationDialog();

  if (wf->ShowModal() != mrOK) {
    delete wf;
    return;
  }

  /* save page number for next time this dialog is opened */
  current_page = wTabMenu->GetLastContentPage();

  // TODO enhancement: implement a cancel button that skips all this
  // below after exit.
  bool changed = false;
  bool requirerestart = false;
  changed |= PagesConfigPanel::Save();
#ifdef HAVE_TRACKING
  changed |= TrackingConfigPanel::Save(CommonInterface::SetSettingsComputer().tracking);
#endif
  changed |= PolarConfigPanel::Save();
  changed |= LoggerConfigPanel::Save();
  changed |= DevicesConfigPanel::Save();
  changed |= AirspaceConfigPanel::Save(requirerestart,
                                       CommonInterface::SetSettingsComputer().airspace,
                                       CommonInterface::SetSettingsMap().airspace);
  changed |= SiteConfigPanel::Save();
  changed |= MapDisplayConfigPanel::Save();
  changed |= WaypointDisplayConfigPanel::Save();
  changed |= SymbolsConfigPanel::Save();
  changed |= TerrainDisplayConfigPanel::Save(CommonInterface::SetSettingsMap());
  changed |= GlideComputerConfigPanel::Save(requirerestart);
  changed |= SafetyFactorsConfigPanel::Save();
  changed |= RouteConfigPanel::Save();
  changed |= InterfaceConfigPanel::Save(requirerestart);
  changed |= LayoutConfigPanel::Save(requirerestart);
  changed |= GaugesConfigPanel::Save();
  changed |= VarioConfigPanel::Save();
  changed |= TaskRulesConfigPanel::Save();
  changed |= TaskDefaultsConfigPanel::Save();
  changed |= InfoBoxesConfigPanel::Save(requirerestart);
  changed |= ExperimentalConfigPanel::Save(requirerestart);
  // Units need to be saved last to prevent
  // conversion problems with other values
  changed |= UnitsConfigPanel::Save();

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
