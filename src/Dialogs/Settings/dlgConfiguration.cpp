/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Widget/DockWindow.hpp"
#include "Widget/PagerWidget.hpp"
#include "Widget/WindowWidget.hpp"
#include "UIGlobals.hpp"
#include "Form/TabMenuDisplay.hpp"
#include "Form/TabMenuData.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Util/Macros.hpp"
#include "Panels/ConfigPanel.hpp"
#include "Panels/PagesConfigPanel.hpp"
#include "Panels/UnitsConfigPanel.hpp"
#include "Panels/TimeConfigPanel.hpp"
#include "Panels/LoggerConfigPanel.hpp"
#include "Panels/AirspaceConfigPanel.hpp"
#include "Panels/SiteConfigPanel.hpp"
#include "Panels/MapDisplayConfigPanel.hpp"
#include "Panels/WaypointDisplayConfigPanel.hpp"
#include "Panels/SymbolsConfigPanel.hpp"
#include "Panels/TerrainDisplayConfigPanel.hpp"
#include "Panels/GlideComputerConfigPanel.hpp"
#include "Panels/WindConfigPanel.hpp"
#include "Panels/SafetyFactorsConfigPanel.hpp"
#include "Panels/RouteConfigPanel.hpp"
#include "Panels/InterfaceConfigPanel.hpp"
#include "Panels/LayoutConfigPanel.hpp"
#include "Panels/GaugesConfigPanel.hpp"
#include "Panels/VarioConfigPanel.hpp"
#include "Panels/TaskRulesConfigPanel.hpp"
#include "Panels/TaskDefaultsConfigPanel.hpp"
#include "Panels/ScoringConfigPanel.hpp"
#include "Panels/InfoBoxesConfigPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Audio/Features.hpp"
#include "UtilsSettings.hpp"

#ifdef HAVE_PCM_PLAYER
#include "Panels/AudioVarioConfigPanel.hpp"
#endif

#ifdef HAVE_TRACKING
#include "Panels/TrackingConfigPanel.hpp"
#endif

#ifdef HAVE_MODEL_TYPE
#include "Panels/ExperimentalConfigPanel.hpp"
#endif

#include <assert.h>

static unsigned current_page;
static WndForm *dialog = NULL;

static TabMenuDisplay *menu;
static PagerWidget *pager;

static constexpr TabMenuGroup main_menu_captions[] = {
  { N_("Site Files"), },
  { N_("Map Display"), },
  { N_("Glide Computer"), },
  { N_("Gauges"), },
  { N_("Task Defaults"), },
  { N_("Look"), },
  { N_("Setup"), },
};

static constexpr TabMenuPage pages[] = {
  {N_("Site Files"), 0, CreateSiteConfigPanel },
  {N_("Orientation"), 1, CreateMapDisplayConfigPanel },
  {N_("Elements"), 1, CreateSymbolsConfigPanel },
  {N_("Waypoints"), 1, CreateWaypointDisplayConfigPanel },
  {N_("Terrain"), 1, CreateTerrainDisplayConfigPanel },
  {N_("Airspace"), 1, CreateAirspaceConfigPanel },
  {N_("Safety Factors"), 2, CreateSafetyFactorsConfigPanel },
  {N_("Glide Computer"), 2, CreateGlideComputerConfigPanel },
  {N_("Wind"), 2, CreateWindConfigPanel },
  {N_("Route"), 2, CreateRouteConfigPanel },
  {N_("Scoring"), 2, CreateScoringConfigPanel },
  {N_("FLARM, Other"), 3, CreateGaugesConfigPanel },
  {N_("Vario"), 3, CreateVarioConfigPanel },
#ifdef HAVE_PCM_PLAYER
  {N_("Audio Vario"), 3, CreateAudioVarioConfigPanel },
#endif
  {N_("Task Rules"), 4, CreateTaskRulesConfigPanel },
  {N_("Turnpoint Types"), 4, CreateTaskDefaultsConfigPanel },
  {N_("Language, Input"), 5, CreateInterfaceConfigPanel },
  {N_("Screen Layout"), 5, CreateLayoutConfigPanel },
  {N_("Pages"), 5, CreatePagesConfigPanel },
  {N_("InfoBox Sets"), 5, CreateInfoBoxesConfigPanel },
  {N_("Logger"), 6, CreateLoggerConfigPanel },
  {N_("Units"), 6, CreateUnitsConfigPanel },
  // Important: all pages after Units in this list must not have data fields that are
  // unit-dependent because they will be saved after their units may have changed.
  // ToDo: implement API that controls order in which pages are saved
  {N_("Time"), 6, CreateTimeConfigPanel },
#ifdef HAVE_TRACKING
  {N_("Tracking"), 6, CreateTrackingConfigPanel },
#endif
#ifdef HAVE_MODEL_TYPE
  {N_("Experimental Features"), 6, CreateExperimentalConfigPanel, },
#endif
};

namespace ConfigPanel {
  static WndButton *GetExtraButton(unsigned);
}

WndButton *
ConfigPanel::GetExtraButton(unsigned number)
{
  assert(number >= 1 && number <= 2);

  WndButton *extra_button = NULL;

  switch (number) {
  case 1:
    extra_button = (WndButton *)dialog->FindByName(_T("cmdExtra1"));
    break;
  case 2:
    extra_button = (WndButton *)dialog->FindByName(_T("cmdExtra2"));
    break;
  }

  return extra_button;
}

void
ConfigPanel::BorrowExtraButton(unsigned i, const TCHAR *caption,
                               void (*callback)())
{
  WndButton &button = *GetExtraButton(i);
  button.SetCaption(caption);
  button.SetOnClickNotify(callback);
  button.Show();
}

void
ConfigPanel::ReturnExtraButton(unsigned i)
{
  WndButton &button = *GetExtraButton(i);
  button.Hide();
}

static void
OnUserLevel(CheckBoxControl &control)
{
  const bool expert = control.GetState();
  CommonInterface::SetUISettings().dialog.expert = expert;
  Profile::Set(ProfileKeys::UserLevel, expert);

  /* force layout update */
  pager->Move(pager->GetPosition());
}

static void
OnNextClicked()
{
  pager->Next(true);
}

static void
OnPrevClicked()
{
  pager->Previous(true);
}

/**
 * close dialog from menu page.  from content, goes to menu page
 */
static void
OnCloseClicked()
{
  if (pager->GetCurrentIndex() == 0)
    dialog->SetModalResult(mrOK);
  else
    pager->ClickPage(0);
}

static bool
FormKeyDown(unsigned key_code)
{
  if (pager->KeyPress(key_code))
    return true;

  switch (key_code) {
  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    pager->Previous(true);
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    pager->Next(true);
    return true;

  default:
    return false;
  }
}

static void
OnPageFlipped()
{
  menu->OnPageFlipped();

  TCHAR buffer[128];
  const TCHAR *caption = menu->GetCaption(buffer, ARRAY_SIZE(buffer));
  if (caption == nullptr)
    caption = _("Configuration");
  dialog->SetCaption(caption);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnUserLevel),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
PrepareConfigurationDialog()
{
  dialog = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_CONFIGURATION_L") :
                                      _T("IDR_XML_CONFIGURATION"));
  if (dialog == NULL)
    return;

  dialog->SetKeyDownFunction(FormKeyDown);

  bool expert_mode = CommonInterface::GetUISettings().dialog.expert;
  CheckBox *cb = (CheckBox *)dialog->FindByName(_T("Expert"));
  cb->SetState(expert_mode);

  pager = new PagerWidget();

  const DialogLook &look = UIGlobals::GetDialogLook();
  menu = new TabMenuDisplay(*pager, look);
  pager->Add(new WindowWidget(menu));
  menu->InitMenu(pages, ARRAY_SIZE(pages),
                 main_menu_captions, ARRAY_SIZE(main_menu_captions));

  DockWindow &dock = *(DockWindow *)dialog->FindByName(_T("menu"));

  WindowStyle menu_style;
  menu_style.Hide();
  menu_style.TabStop();
  menu->Create(dock, dock.GetClientRect(), menu_style);

  dock.SetWidget(pager);

  pager->SetPageFlippedCallback(OnPageFlipped);

  /* restore last selected menu item */
  menu->SetCursor(current_page);
}

static void
Save()
{
  /* save page number for next time this dialog is opened */
  current_page = menu->GetCursor();

  bool changed = false;
  pager->Save(changed);

  if (changed) {
    Profile::Save();
    LogDebug(_T("Configuration: Changes saved"));
    if (require_restart)
      ShowMessageBox(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                  _T(""), MB_OK);
  }
}

void dlgConfigurationShowModal()
{
  PrepareConfigurationDialog();

  dialog->ShowModal();

  if (dialog->IsDefined()) {
    /* hide the dialog, to avoid redraws inside Save() on a dialog
       that has already been deregistered from the SingleWindow */
    dialog->Hide();

    Save();
  }

  /* destroy the TabMenuControl first, to have a well-defined
     destruction order; this is necessary because some config panels
     refer to buttons belonging to the dialog */
  DockWindow &dock = *(DockWindow *)dialog->FindByName(_T("menu"));
  dock.SetWidget(nullptr);

  delete menu;

  delete dialog;
  dialog = NULL;
}
