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

#include "Dialogs/Internal.hpp"
#include "Protection.hpp"
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "SettingsMap.hpp"
#include "Appearance.hpp"
#include "LocalPath.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/DisplayConfig.hpp"
#include "Logger/Logger.hpp"
#include "Screen/Busy.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Key.h"
#include "Form/CheckBox.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Profile/Profile.hpp"
#include "Math/FastMath.h"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "DataField/FileReader.hpp"
#include "DataField/ComboList.hpp"
#include "Asset.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "WayPointFile.hpp"
#include "StringUtil.hpp"
#include "Simulator.hpp"
#include "Compiler.h"
#include "Screen/Graphics.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Hardware/Display.hpp"
#include "OS/PathName.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "LogFile.hpp"
#include "LanguageGlue.hpp"
#include "ConfigPanel.hpp"
#include "PagesConfigPanel.hpp"
#include "PolarConfigPanel.hpp"
#include "UnitsConfigPanel.hpp"
#include "LoggerConfigPanel.hpp"
#include "DevicesConfigPanel.hpp"
#include "AirspaceConfigPanel.hpp"
#include "SiteConfigPanel.hpp"
#include "MapDisplayConfigPanel.hpp"
#include "WayPointDisplayConfigPanel.hpp"
#include "SymbolsConfigPanel.hpp"
#include "TerrainDisplayConfigPanel.hpp"
#include "GlideComputerConfigPanel.hpp"


#include <assert.h>

enum config_page {
  PAGE_SITE,
  PAGE_AIRSPACE,
  PAGE_MAP,
  PAGE_WAYPOINTS,
  PAGE_SYMBOLS,
  PAGE_TERRAIN,
  PAGE_GLIDE_COMPUTER,
  PAGE_SAFETY_FACTORS,
  PAGE_ROUTE,
  PAGE_POLAR,
  PAGE_DEVICES,
  PAGE_UNITS,
  PAGE_INTERFACE,
  PAGE_LAYOUT,
  PAGE_VARIO,
  PAGE_TASK_RULES,
  PAGE_TASK_DEFAULTS,
  PAGE_INFOBOXES,
  PAGE_LOGGER,
  PAGE_PAGES,
  PAGE_EXPERIMENTAL,
};

static const TCHAR *const captions[] = {
  N_("Site"),
  N_("Airspace"),
  N_("Map Display"),
  N_("Waypoint Display"),
  N_("Symbols"),
  N_("Terrain Display"),
  N_("Glide Computer"),
  N_("Safety factors"),
  N_("Route"),
  N_("Polar"),
  N_("Devices"),
  N_("Units"),
  N_("Interface"),
  N_("Layout"),
  N_("FLARM and other gauges"),
  N_("Default task rules"),
  N_("Default task turnpoints"),
  N_("InfoBoxes"),
  N_("Logger"),
  N_("Pages"),
  N_("Experimental features"),
};


static bool loading = false;
static bool changed = false;
static bool taskchanged = false;
static bool requirerestart = false;
static bool waypointneedsave = false;
static config_page current_page;
static WndForm *wf = NULL;
TabbedControl *configuration_tabbed;
static WndButton *buttonFonts = NULL;
static WndButton *buttonWaypoints = NULL;

static void
PageSwitched()
{
  current_page = (config_page)configuration_tabbed->GetCurrentPage();

  TCHAR caption[64];
  _sntprintf(caption, 64, _T("%u %s"),
             (unsigned)current_page + 1,
             gettext(captions[(unsigned)current_page]));
  wf->SetCaption(caption);

  if (buttonFonts != NULL)
    buttonFonts->set_visible(current_page == PAGE_INTERFACE);

  if (buttonWaypoints != NULL)
    buttonWaypoints->set_visible(current_page == PAGE_SITE);

  PolarConfigPanel::Activate(current_page == PAGE_POLAR);
}


static void
OnUserLevel(CheckBoxControl &control)
{
  changed = true;
  Profile::Set(szProfileUserLevel, control.get_checked());
  wf->FilterAdvanced(control.get_checked());
}

static void
OnNextClicked(gcc_unused WndButton &button)
{
  configuration_tabbed->NextPage();
  PageSwitched();
}

static void
OnPrevClicked(gcc_unused WndButton &button)
{
  configuration_tabbed->PreviousPage();
  PageSwitched();
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnInfoBoxesCircling(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(InfoBoxManager::PANEL_CIRCLING);
}

static void
OnInfoBoxesCruise(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(InfoBoxManager::PANEL_CRUISE);
}

static void
OnInfoBoxesFinalGlide(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(InfoBoxManager::PANEL_FINAL_GLIDE);
}

static void
OnInfoBoxesAuxiliary(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(InfoBoxManager::PANEL_AUXILIARY);
}

static void
OnFonts(gcc_unused WndButton &button)
{
  dlgConfigFontsShowModal();
}

static void
OnWaypoints(gcc_unused WndButton &button)
{
  dlgConfigWaypointsShowModal();
}

static bool
FormKeyDown(WndForm &Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    configuration_tabbed->PreviousPage();
    PageSwitched();
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    configuration_tabbed->NextPage();
    PageSwitched();
    return true;

  default:
    return false;
  }
}

extern void OnInfoBoxHelp(WindowControl * Sender);

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(AirspaceConfigPanel::OnAirspaceColoursClicked),
  DeclareCallBackEntry(AirspaceConfigPanel::OnAirspaceModeClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(DevicesConfigPanel::OnSetupDeviceAClicked),
  DeclareCallBackEntry(DevicesConfigPanel::OnSetupDeviceBClicked),
  DeclareCallBackEntry(OnInfoBoxHelp),
  DeclareCallBackEntry(DevicesConfigPanel::OnDeviceAData),
  DeclareCallBackEntry(DevicesConfigPanel::OnDeviceBData),
  DeclareCallBackEntry(PolarConfigPanel::OnLoadInteral),
  DeclareCallBackEntry(PolarConfigPanel::OnLoadFromFile),
  DeclareCallBackEntry(PolarConfigPanel::OnExport),
  DeclareCallBackEntry(PolarConfigPanel::OnFieldData),
  DeclareCallBackEntry(UnitsConfigPanel::OnLoadPreset),
  DeclareCallBackEntry(UnitsConfigPanel::OnFieldData),
  DeclareCallBackEntry(UnitsConfigPanel::OnUTCData),
  DeclareCallBackEntry(OnUserLevel),
  DeclareCallBackEntry(NULL)
};

static void
setVariables()
{
  WndProperty *wp;

  buttonFonts = ((WndButton *)wf->FindByName(_T("cmdFonts")));
  if (buttonFonts)
    buttonFonts->SetOnClickNotify(OnFonts);

  buttonWaypoints = ((WndButton *)wf->FindByName(_T("cmdWaypoints")));
  if (buttonWaypoints)
    buttonWaypoints->SetOnClickNotify(OnWaypoints);

  PolarConfigPanel::Init(wf);
  UnitsConfigPanel::Init(wf);
  LoggerConfigPanel::Init(wf);
  DevicesConfigPanel::Init(wf);
  AirspaceConfigPanel::Init(wf);
  SiteConfigPanel::Init(wf);
  MapDisplayConfigPanel::Init(wf);
  WayPointDisplayConfigPanel::Init(wf);
  SymbolsConfigPanel::Init(wf);
  TerrainDisplayConfigPanel::Init(wf);
  GlideComputerConfigPanel::Init(wf);

  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();

  LoadFormProperty(*wf, _T("prpDebounceTimeout"),
                   XCSoarInterface::debounceTimeout);

  LoadFormProperty(*wf, _T("prpEnableFLARMGauge"),
                   XCSoarInterface::SettingsMap().EnableFLARMGauge);
  LoadFormProperty(*wf, _T("prpAutoCloseFlarmDialog"),
                   XCSoarInterface::SettingsMap().AutoCloseFlarmDialog);
  LoadFormProperty(*wf, _T("prpEnableTAGauge"),
                   XCSoarInterface::SettingsMap().EnableTAGauge);


  LoadFormProperty(*wf, _T("prpMenuTimeout"),
                   XCSoarInterface::MenuTimeoutMax / 2);

  LoadFormProperty(*wf, _T("prpSafetyAltitudeArrival"), ugAltitude,
                   settings_computer.safety_height_arrival);
  LoadFormProperty(*wf, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                   settings_computer.route_planner.safety_height_terrain);

  wp = (WndProperty*)wf->FindByName(_T("prpAbortTaskMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Simple"));
    dfe->addEnumText(_("Task"));
    dfe->addEnumText(_("Home"));
    dfe->Set(settings_computer.abort_task_mode);
    wp->RefreshDisplay();
  }


  wp = (WndProperty*)wf->FindByName(_T("prpContests"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("OLC FAI"), OLC_FAI);
    dfe->addEnumText(_("OLC Classic"), OLC_Classic);
    dfe->addEnumText(_("OLC League"), OLC_League);
    dfe->addEnumText(_("OLC Plus"), OLC_Plus);
    dfe->addEnumText(_("XContest"), OLC_XContest);
    dfe->addEnumText(_("DHV-XC"), OLC_DHVXC);
    dfe->addEnumText(_("SIS-AT"), OLC_SISAT);
    dfe->Set(settings_computer.contest);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpHandicap"),
                   settings_computer.contest_handicap);

  LoadFormProperty(*wf, _T("prpAbortSafetyUseCurrent"),
                   settings_computer.safety_mc_use_current);

  LoadFormProperty(*wf, _T("prpSafetyMacCready"), ugVerticalSpeed,
                   settings_computer.safety_mc);

  LoadFormProperty(*wf, _T("prpRiskGamma"), settings_computer.risk_gamma);


  PagesConfigPanel::Init(wf);

  LoadFormProperty(*wf, _T("prpMaxManoeuveringSpeed"), ugHorizontalSpeed,
                   settings_computer.SafetySpeed);

  LoadFormProperty(*wf, _T("prpBallastSecsToEmpty"),
                   settings_computer.BallastSecsToEmpty);

  wp = (WndProperty *)wf->FindByName(_T("prpLanguageFile"));
  if (wp != NULL) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    df.addEnumText(_("Automatic"));
    df.addEnumText(_("None"));

#ifdef HAVE_BUILTIN_LANGUAGES
    for (const struct builtin_language *l = language_table;
         l->resource != NULL; ++l)
      df.addEnumText(l->resource);
#endif

    DataFieldFileReader files(NULL);
    files.ScanDirectoryTop(_T("*.mo"));
    for (unsigned i = 0; i < files.size(); ++i) {
      const TCHAR *path = files.getItem(i);
      if (path == NULL)
        continue;

      path = BaseName(path);
      if (path != NULL && !df.Exists(path))
        df.addEnumText(path);
    }

    df.Sort(2);

    TCHAR value[MAX_PATH];
    if (!Profile::GetPath(szProfileLanguageFile, value))
      value[0] = _T('\0');

    if (_tcscmp(value, _T("none")) == 0)
      df.Set(1);
    else if (!string_is_empty(value) && _tcscmp(value, _T("auto")) != 0) {
      const TCHAR *base = BaseName(value);
      if (base != NULL)
        df.SetAsString(base);
    }

    wp->RefreshDisplay();
  }

  ConfigPanel::InitFileField(*wf, _T("prpStatusFile"),
                szProfileStatusFile, _T("*.xcs\0"));
  ConfigPanel::InitFileField(*wf, _T("prpInputFile"),
                szProfileInputFile, _T("*.xci\0"));

  wp = (WndProperty*)wf->FindByName(_T("prpAppStatusMessageAlignment"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Center"));
    dfe->addEnumText(_("Topleft"));
    dfe->Set(Appearance.StateMessageAlign);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTextInput"));
  assert(wp != NULL);
  if (has_pointer()) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Default"));
    dfe->addEnumText(_("Keyboard"));
    dfe->addEnumText(_("HighScore Style"));
    dfe->Set(Appearance.TextInputStyle);
    wp->RefreshDisplay();
  } else
    /* on-screen keyboard doesn't work without a pointing device
       (mouse or touch screen), hide the option on Altair */
    wp->hide();

  wp = (WndProperty*)wf->FindByName(_T("prpTabDialogStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Text"));
    dfe->addEnumText(_("Icons"));
    dfe->Set(Appearance.DialogTabStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDialogStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Full width"));
    dfe->addEnumText(_("Scaled"));
    dfe->addEnumText(_("Scaled centered"));
    dfe->addEnumText(_("Fixed"));
    dfe->Set(DialogStyleSetting);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Box"));
    dfe->addEnumText(_("Tab"));
    dfe->Set(Appearance.InfoBoxBorder);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();

    dfe->addEnumText(_("8 Top + Bottom (Portrait)")); // 0
    dfe->addEnumText(_("8 Bottom (Portrait)"));  // 1
    dfe->addEnumText(_("8 Top (Portrait)"));  // 2
    dfe->addEnumText(_("8 Left + Right (Landscape)"));  // 3
    dfe->addEnumText(_("8 Left (Landscape)"));  // 4
    dfe->addEnumText(_("8 Right (Landscape)")); // 5
    dfe->addEnumText(_("9 Right + Vario (Landscape)"));  // 6
    dfe->addEnumText(_("5 Right (Square)")); // 7
    dfe->addEnumText(_("12 Right (Landscape)")); // 8
    dfe->Set(InfoBoxLayout::InfoBoxGeometry);
    wp->RefreshDisplay();
  }

  if (Display::RotateSupported()) {
    wp = (WndProperty*)wf->FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);

    DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
    dfe->addEnumText(_("Default"));
    dfe->addEnumText(_("Portrait"));
    dfe->addEnumText(_("Landscape"));
    dfe->Set(Profile::GetDisplayOrientation());
    wp->RefreshDisplay();
  } else {
    wp = (WndProperty*)wf->FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);
    wp->hide();
  }

#if defined(_WIN32_WCE) && !defined(GNAV)
// VENTA-ADDON Model change config menu 11
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Generic"));
    dfe->addEnumText(_T("HP31x"));
    dfe->addEnumText(_T("MedionP5"));
    dfe->addEnumText(_T("MIO"));
    dfe->addEnumText(_T("Nokia500")); // VENTA3
    dfe->addEnumText(_T("PN6000"));
    dfe->Set((int)GlobalModelType);
    wp->RefreshDisplay();
  }
#endif

  wp = (WndProperty*)wf->FindByName(_T("prpGestures"));
  if (wp) {
    if (has_pointer()) {
      DataFieldBoolean &df = *(DataFieldBoolean *)wp->GetDataField();
      df.Set(settings_computer.EnableGestures);
      wp->RefreshDisplay();
    } else {
      wp->hide();
    }
  }

  LoadFormProperty(*wf, _T("prpAppInverseInfoBox"),
                   Appearance.InverseInfoBox);

  LoadFormProperty(*wf, _T("prpAppInfoBoxColors"), Appearance.InfoBoxColors);

  LoadFormProperty(*wf, _T("prpAppAveNeedle"),
                   XCSoarInterface::main_window.vario->ShowAveNeedle);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                   XCSoarInterface::main_window.vario->ShowSpeedToFly);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                   XCSoarInterface::main_window.vario->ShowAvgText);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                   XCSoarInterface::main_window.vario->ShowGross);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioMc"),
                   XCSoarInterface::main_window.vario->ShowMc);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioBugs"),
                   XCSoarInterface::main_window.vario->ShowBugs);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                   XCSoarInterface::main_window.vario->ShowBallast);

  wp = (WndProperty*)wf->FindByName(_T("prpAutoBlank"));
  if (wp) {
    if (is_altair() || !is_embedded())
      wp->hide();
    DataFieldBoolean *df = (DataFieldBoolean *)wp->GetDataField();
    df->Set(XCSoarInterface::SettingsMap().EnableAutoBlank);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpFinishMinHeight"), ugAltitude,
                   settings_computer.ordered_defaults.finish_min_height);
  LoadFormProperty(*wf, _T("prpStartMaxHeight"), ugAltitude,
                   settings_computer.ordered_defaults.start_max_height);
  LoadFormProperty(*wf, _T("prpStartMaxHeightMargin"), ugAltitude,
                   settings_computer.start_max_height_margin);

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("AGL"));
    dfe->addEnumText(_("MSL"));
    dfe->Set(settings_computer.ordered_defaults.start_max_height_ref);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpStartMaxSpeed"), ugHorizontalSpeed,
                   settings_computer.ordered_defaults.start_max_speed);
  LoadFormProperty(*wf, _T("prpStartMaxSpeedMargin"), ugHorizontalSpeed,
                   settings_computer.start_max_speed_margin);


  OrderedTask* temptask = protected_task_manager->task_blank();
  temptask->set_factory(TaskBehaviour::FACTORY_RT);

  wp = (WndProperty*)wf->FindByName(_T("prpStartType"));
  if (wp) {
    const AbstractTaskFactory::LegalPointVector point_types =
        temptask->get_factory().getValidStartTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < point_types.size(); i++) {
      dfe->addEnumText(OrderedTaskPointName(point_types[i]), (unsigned)point_types[i],
          OrderedTaskPointDescription(point_types[i]));
      if (point_types[i] == settings_computer.sector_defaults.start_type)
        dfe->Set((unsigned)point_types[i]);
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpTurnpointType"));
  if (wp) {
    const AbstractTaskFactory::LegalPointVector point_types =
        temptask->get_factory().getValidIntermediateTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < point_types.size(); i++) {
      dfe->addEnumText(OrderedTaskPointName(point_types[i]),
          (unsigned)point_types[i],
          OrderedTaskPointDescription(point_types[i]));
      if (point_types[i] == settings_computer.sector_defaults.turnpoint_type) {
        dfe->Set((unsigned)point_types[i]);
      }
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFinishType"));
  if (wp) {
    const AbstractTaskFactory::LegalPointVector point_types =
        temptask->get_factory().getValidFinishTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < point_types.size(); i++) {
      dfe->addEnumText(OrderedTaskPointName(point_types[i]), (unsigned)point_types[i],
          OrderedTaskPointDescription(point_types[i]));
      if (point_types[i] == settings_computer.sector_defaults.finish_type)
        dfe->Set((unsigned)point_types[i]);
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpTaskType"));
  if (wp) {
    const std::vector<TaskBehaviour::Factory_t> factory_types =
        temptask->get_factory_types();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < factory_types.size(); i++) {
      dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
          (unsigned)factory_types[i], OrderedTaskFactoryDescription(
              factory_types[i]));
      if (factory_types[i] == settings_computer.task_type_default)
        dfe->Set((unsigned)factory_types[i]);
    }
    wp->RefreshDisplay();
  }

  delete temptask;

  LoadFormProperty(*wf, _T("prpStartRadius"),
                                  ugDistance,
                                  settings_computer.sector_defaults.start_radius);
  LoadFormProperty(*wf, _T("prpTurnpointRadius"),
                                  ugDistance,
                                  settings_computer.sector_defaults.turnpoint_radius);
  LoadFormProperty(*wf, _T("prpFinishRadius"),
                                  ugDistance,
                                  settings_computer.sector_defaults.finish_radius);
  LoadFormProperty(*wf, _T("prpAATMinTime"),
                   (unsigned)(settings_computer.ordered_defaults.aat_min_time / 60));


  wp = (WndProperty*)wf->FindByName(_T("prpRoutePlannerMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("None"));
    dfe->addEnumText(_("Terrain"));
    dfe->addEnumText(_("Airspace"));
    dfe->addEnumText(_("Both"));
    dfe->Set(settings_computer.route_planner.mode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpRoutePlannerAllowClimb"),
                   settings_computer.route_planner.allow_climb);

  LoadFormProperty(*wf, _T("prpRoutePlannerUseCeiling"),
                   settings_computer.route_planner.use_ceiling);

}

static void
PrepareConfigurationDialog()
{
  gcc_unused ScopeBusyIndicator busy;

  loading = true;

  WndProperty *wp;

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

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesCircling")))->
      SetOnClickNotify(OnInfoBoxesCircling);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesCruise")))->
      SetOnClickNotify(OnInfoBoxesCruise);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesFinalGlide")))->
      SetOnClickNotify(OnInfoBoxesFinalGlide);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAuxiliary")))->
      SetOnClickNotify(OnInfoBoxesAuxiliary);

  configuration_tabbed = ((TabbedControl *)wf->FindByName(_T("tabbed")));
  assert(configuration_tabbed != NULL);

  if (!is_windows_ce() || is_altair()) {
    wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
    if (wp) {
      wp->hide();
    }
  }

  setVariables();

  /* restore previous page */
  configuration_tabbed->SetCurrentPage((unsigned)current_page);
  PageSwitched();

  changed = false;
  taskchanged = false;
  requirerestart = false;
  waypointneedsave = false;
  loading = false;
}

void dlgConfigurationShowModal(void)
{
  PrepareConfigurationDialog();

  if (wf->ShowModal() != mrOK) {
    delete wf;
    return;
  }

  /* save page number for next time this dialog is opened */
  current_page = (config_page)configuration_tabbed->GetCurrentPage();

  // TODO enhancement: implement a cancel button that skips all this
  // below after exit.

  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();

  changed |= SaveFormProperty(*wf, _T("prpAbortSafetyUseCurrent"),
                              szProfileAbortSafetyUseCurrent,
                              settings_computer.safety_mc_use_current);

  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpAbortTaskMode"));
  if (wp) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    if ((int)settings_computer.abort_task_mode != df.GetAsInteger()) {
      settings_computer.abort_task_mode = (AbortTaskMode)df.GetAsInteger();
      Profile::Set(szProfileAbortTaskMode,
                   (unsigned)settings_computer.abort_task_mode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    fixed val = Units::ToSysVSpeed(df.GetAsFixed());
    if (settings_computer.safety_mc != val) {
      settings_computer.safety_mc = val;
      Profile::Set(szProfileSafetyMacCready,
                    iround(settings_computer.safety_mc*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRiskGamma"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    fixed val = df.GetAsFixed();
    if (settings_computer.risk_gamma != val) {
      settings_computer.risk_gamma = val;
      Profile::Set(szProfileRiskGamma,
                    iround(settings_computer.risk_gamma*10));
      changed = true;
    }
  }


  changed |= SaveFormProperty(*wf, _T("prpEnableFLARMGauge"),
                              szProfileEnableFLARMGauge,
                              XCSoarInterface::SetSettingsMap().EnableFLARMGauge);

  changed |= SaveFormProperty(*wf, _T("prpAutoCloseFlarmDialog"),
                              szProfileAutoCloseFlarmDialog,
                              XCSoarInterface::SetSettingsMap().AutoCloseFlarmDialog);

  changed |= SaveFormProperty(*wf, _T("prpEnableTAGauge"),
                              szProfileEnableTAGauge,
                              XCSoarInterface::SetSettingsMap().EnableTAGauge);

  changed |= SaveFormProperty(*wf, _T("prpDebounceTimeout"),
                              szProfileDebounceTimeout,
                              XCSoarInterface::debounceTimeout);

  wp = (WndProperty*)wf->FindByName(_T("prpMenuTimeout"));
  if (wp) {
    if ((int)XCSoarInterface::MenuTimeoutMax != wp->GetDataField()->GetAsInteger()*2) {
      XCSoarInterface::MenuTimeoutMax = wp->GetDataField()->GetAsInteger()*2;
      Profile::Set(szProfileMenuTimeout,XCSoarInterface::MenuTimeoutMax);
      changed = true;
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpSafetyAltitudeArrival"), ugAltitude,
                              settings_computer.safety_height_arrival,
                              szProfileSafetyAltitudeArrival);

  changed |= SaveFormProperty(*wf, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                              settings_computer.route_planner.safety_height_terrain,
                              szProfileSafetyAltitudeTerrain);


  {
    unsigned t= settings_computer.contest;
    changed |= SaveFormProperty(*wf, _T("prpContests"), szProfileOLCRules,
                                t);
    settings_computer.contest = (Contests)t;
  }

  changed |= SaveFormProperty(*wf, _T("prpHandicap"), szProfileHandicap,
                              settings_computer.contest_handicap);

  wp = (WndProperty *)wf->FindByName(_T("prpLanguageFile"));
  if (wp != NULL) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();

    TCHAR old_value[MAX_PATH];
    if (!Profile::GetPath(szProfileLanguageFile, old_value))
      old_value[0] = _T('\0');

    const TCHAR *old_base = BaseName(old_value);
    if (old_base == NULL)
      old_base = old_value;

    TCHAR buffer[MAX_PATH];
    const TCHAR *new_value, *new_base;

    switch (df.GetAsInteger()) {
    case 0:
      new_value = new_base = _T("auto");
      break;

    case 1:
      new_value = new_base = _T("none");
      break;

    default:
      _tcscpy(buffer, df.GetAsString());
      ContractLocalPath(buffer);
      new_value = buffer;
      new_base = BaseName(new_value);
      if (new_base == NULL)
        new_base = new_value;
      break;
    }

    if (_tcscmp(old_value, new_value) != 0 &&
        _tcscmp(old_base, new_base) != 0) {
      Profile::Set(szProfileLanguageFile, new_value);
      LanguageChanged = changed = true;
    }
  }

  if (ConfigPanel::FinishFileField(*wf, _T("prpStatusFile"), szProfileStatusFile)) {
    changed = true;
    requirerestart = true;
  }

  if (ConfigPanel::FinishFileField(*wf, _T("prpInputFile"), szProfileInputFile)) {
    changed = true;
    requirerestart = true;
  }

  changed |= SaveFormProperty(*wf, _T("prpBallastSecsToEmpty"),
                              szProfileBallastSecsToEmpty,
                              settings_computer.BallastSecsToEmpty);

  changed |= SaveFormProperty(*wf, _T("prpMaxManoeuveringSpeed"),
                              ugHorizontalSpeed, settings_computer.SafetySpeed,
                              szProfileSafteySpeed);

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    if (Appearance.InfoBoxBorder != (InfoBoxBorderAppearance_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppInfoBoxBorder,
                    Appearance.InfoBoxBorder);
      changed = true;
      requirerestart = true;
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpGestures"), szProfileGestures,
                              settings_computer.EnableGestures);


  bool info_box_geometry_changed = false;
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    if (InfoBoxLayout::InfoBoxGeometry !=
        (InfoBoxLayout::Geometry)wp->GetDataField()->GetAsInteger()) {
      InfoBoxLayout::InfoBoxGeometry =
          (InfoBoxLayout::Geometry)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileInfoBoxGeometry,
                   (unsigned)InfoBoxLayout::InfoBoxGeometry);
      changed = true;
      info_box_geometry_changed = true;
    }
  }

  bool orientation_changed = false;
  if (Display::RotateSupported()) {
    wp = (WndProperty*)wf->FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);

    const DataFieldEnum *dfe = (const DataFieldEnum *)wp->GetDataField();
    Display::orientation orientation =
      (Display::orientation)dfe->GetAsInteger();
    if (orientation != Profile::GetDisplayOrientation()) {
      Profile::SetDisplayOrientation(orientation);
      changed = true;
      orientation_changed = true;
    }
  }

#if defined(_WIN32_WCE) && !defined(GNAV)
  // VENTA-ADDON MODEL CHANGE
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
  if (wp) {
    if (GlobalModelType != (ModelType)wp->GetDataField()->GetAsInteger()) {
      GlobalModelType = (ModelType)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileAppInfoBoxModel,
                    GlobalModelType);
      changed = true;
      requirerestart = true;
    }
  }
//
#endif

  wp = (WndProperty*)wf->FindByName(_T("prpAppStatusMessageAlignment"));
  if (wp) {
    if (Appearance.StateMessageAlign != (StateMessageAlign_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.StateMessageAlign = (StateMessageAlign_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppStatusMessageAlignment,
                    Appearance.StateMessageAlign);
      changed = true;
    }
  }

  if (has_pointer()) {
    wp = (WndProperty*)wf->FindByName(_T("prpTextInput"));
    assert(wp != NULL);
    if (Appearance.TextInputStyle != (TextInputStyle_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.TextInputStyle = (TextInputStyle_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppTextInputStyle, Appearance.TextInputStyle);
      changed = true;
    }
  }

    wp = (WndProperty*)wf->FindByName(_T("prpTabDialogStyle"));
    assert(wp != NULL);
    if (Appearance.DialogTabStyle != (DialogTabStyle_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.DialogTabStyle = (DialogTabStyle_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppDialogTabStyle, Appearance.DialogTabStyle);
      changed = true;
    }

  wp = (WndProperty*)wf->FindByName(_T("prpDialogStyle"));
  if (wp)
    {
      if (DialogStyleSetting != (DialogStyle)(wp->GetDataField()->GetAsInteger()))
        {
          DialogStyleSetting = (DialogStyle)(wp->GetDataField()->GetAsInteger());
          Profile::Set(szProfileAppDialogStyle, DialogStyleSetting);
          changed = true;
        }
    }

  changed |= requirerestart |=
    SaveFormProperty(*wf, _T("prpAppInverseInfoBox"),
                     szProfileAppInverseInfoBox, Appearance.InverseInfoBox);

  changed |= SaveFormProperty(*wf, _T("prpAppAveNeedle"), szProfileAppAveNeedle,
                              XCSoarInterface::main_window.vario->ShowAveNeedle);

  changed |= requirerestart |=
    SaveFormProperty(*wf, _T("prpAppInfoBoxColors"),
                     szProfileAppInfoBoxColors, Appearance.InfoBoxColors);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                              szProfileAppGaugeVarioSpeedToFly,
                              XCSoarInterface::main_window.vario->ShowSpeedToFly);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                              szProfileAppGaugeVarioAvgText,
                              XCSoarInterface::main_window.vario->ShowAvgText);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                              szProfileAppGaugeVarioGross,
                              XCSoarInterface::main_window.vario->ShowGross);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioMc"),
                              szProfileAppGaugeVarioMc,
                              XCSoarInterface::main_window.vario->ShowMc);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioBugs"),
                              szProfileAppGaugeVarioBugs,
                              XCSoarInterface::main_window.vario->ShowBugs);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                              szProfileAppGaugeVarioBallast,
                              XCSoarInterface::main_window.vario->ShowBallast);

#ifdef HAVE_BLANK
  changed |= SaveFormProperty(*wf, _T("prpAutoBlank"),
                              szProfileAutoBlank,
                              XCSoarInterface::SetSettingsMap().EnableAutoBlank);
#endif

  taskchanged |= SaveFormProperty(*wf, _T("prpFinishMinHeight"), ugAltitude,
                                  settings_computer.ordered_defaults.finish_min_height,
                                  szProfileFinishMinHeight);

  taskchanged |= SaveFormProperty(*wf, _T("prpStartMaxHeight"), ugAltitude,
                                  settings_computer.ordered_defaults.start_max_height,
                                  szProfileStartMaxHeight);

  taskchanged |= SaveFormProperty(*wf, _T("prpStartMaxHeightMargin"), ugAltitude,
                                  settings_computer.start_max_height_margin,
                                  szProfileStartMaxHeightMargin);

  taskchanged |= SaveFormProperty(*wf, _T("prpStartHeightRef"), ugAltitude,
                                  settings_computer.ordered_defaults.start_max_height_ref,
                                  szProfileStartHeightRef);

  taskchanged |= SaveFormProperty(*wf, _T("prpStartMaxSpeed"),
                                  ugHorizontalSpeed,
                                  settings_computer.ordered_defaults.start_max_speed,
                                  szProfileStartMaxSpeed);

  taskchanged |= SaveFormProperty(*wf, _T("prpStartMaxSpeedMargin"),
                                  ugHorizontalSpeed,
                                  settings_computer.start_max_speed_margin,
                                  szProfileStartMaxSpeedMargin);

  unsigned sdtemp = 0;
  sdtemp = (unsigned)settings_computer.sector_defaults.start_type;
  taskchanged |= SaveFormProperty(*wf, _T("prpStartType"),
                                  szProfileStartType,
                                  sdtemp);
  settings_computer.sector_defaults.start_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;

  taskchanged |= SaveFormProperty(*wf, _T("prpStartRadius"),
                                  ugDistance,
                                  settings_computer.sector_defaults.start_radius,
                                  szProfileStartRadius);

  sdtemp = (unsigned)settings_computer.sector_defaults.turnpoint_type;
  taskchanged |= SaveFormProperty(*wf, _T("prpTurnpointType"),
                                  szProfileTurnpointType,
                                  sdtemp);
  settings_computer.sector_defaults.turnpoint_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;

  taskchanged |= SaveFormProperty(*wf, _T("prpTurnpointRadius"),
                                  ugDistance,
                                  settings_computer.sector_defaults.turnpoint_radius,
                                  szProfileTurnpointRadius);

  sdtemp = (unsigned)settings_computer.sector_defaults.finish_type;
  taskchanged |= SaveFormProperty(*wf, _T("prpFinishType"),
                                  szProfileFinishType,
                                  sdtemp);
  settings_computer.sector_defaults.finish_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;

  taskchanged |= SaveFormProperty(*wf, _T("prpFinishRadius"),
                                  ugDistance,
                                  settings_computer.sector_defaults.finish_radius,
                                  szProfileFinishRadius);

  sdtemp = (unsigned)settings_computer.task_type_default;
  taskchanged |= SaveFormProperty(*wf, _T("prpTaskType"),
                                  szProfileTaskType,
                                  sdtemp);
  settings_computer.task_type_default =
      (TaskBehaviour::Factory_t)sdtemp;


  unsigned aatminutes = unsigned(settings_computer.ordered_defaults.aat_min_time) / 60;
  wp = (WndProperty*)wf->FindByName(_T("prpAATMinTime"));
  if (aatminutes != (unsigned)wp->GetDataField()->GetAsInteger()) {
    aatminutes = wp->GetDataField()->GetAsInteger();
    settings_computer.ordered_defaults.aat_min_time = fixed(aatminutes * 60);
    Profile::Set(szProfileAATMinTime, aatminutes * 60);
    taskchanged = true;
  }

  changed |= taskchanged;

  wp = (WndProperty*)wf->FindByName(_T("prpRoutePlannerMode"));
  if (wp) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    if ((int)settings_computer.route_planner.mode != df.GetAsInteger()) {
      settings_computer.route_planner.mode = (RoutePlannerConfig::Mode)df.GetAsInteger();
      Profile::Set(szProfileRoutePlannerMode,
                   (unsigned)settings_computer.route_planner.mode);
      changed = true;
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpRoutePlannerAllowClimb"),
                              szProfileRoutePlannerAllowClimb,
                              settings_computer.route_planner.allow_climb);

  changed |= SaveFormProperty(*wf, _T("prpRoutePlannerUseCeiling"),
                              szProfileRoutePlannerUseCeiling,
                              settings_computer.route_planner.use_ceiling);

  changed |= UnitsConfigPanel::Save();
  changed |= PagesConfigPanel::Save(wf);
  changed |= PolarConfigPanel::Save();
  changed |= LoggerConfigPanel::Save();
  changed |= DevicesConfigPanel::Save(requirerestart);
  changed |= AirspaceConfigPanel::Save();
  changed |= SiteConfigPanel::Save();
  changed |= MapDisplayConfigPanel::Save();
  changed |= WayPointDisplayConfigPanel::Save();
  changed |= SymbolsConfigPanel::Save();
  changed |= TerrainDisplayConfigPanel::Save();
  changed |= GlideComputerConfigPanel::Save(requirerestart);

  if (orientation_changed) {
    assert(Display::RotateSupported());

    Display::orientation orientation = Profile::GetDisplayOrientation();
    if (orientation == Display::ORIENTATION_DEFAULT)
      Display::RotateRestore();
    else {
      if (!Display::Rotate(orientation))
        LogStartUp(_T("Display rotation failed"));
    }
  } else if (info_box_geometry_changed)
    XCSoarInterface::main_window.ReinitialiseLayout();

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
