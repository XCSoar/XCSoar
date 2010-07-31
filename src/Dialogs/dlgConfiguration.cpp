/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Blackboard.hpp"
#include "SettingsAirspace.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Appearance.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "LocalPath.hpp"
#include "ProfileKeys.hpp"
#include "Logger/Logger.hpp"
#include "Device/Register.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Screen/Busy.hpp"
#include "Screen/Blank.hpp"
#include "Screen/CheckBox.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Profile.hpp"
#include "LocalTime.hpp"
#include "Math/FastMath.h"
#include "Polar/BuiltIn.hpp"
#include "Polar/Historical.hpp"
#include "Polar/Loader.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/FileReader.hpp"
#include "Asset.hpp"
#include "Screen/Fonts.hpp"
#include "GlideRatio.hpp"
#include "Components.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WayPointFile.hpp"
#include "WayPoint/WayPointGlue.hpp"
#include "StringUtil.hpp"
#include "Simulator.hpp"
#include "Compiler.h"
#include "InfoBoxes/InfoBoxLayout.hpp"

#include <assert.h>

enum {
  ID_USER_LEVEL = 100,
};

static const TCHAR *const captions[] = {
  N_("Site"),
  N_("Airspace"),
  N_("Map Display"),
  N_("Terrain Display"),
  N_("Glide Computer"),
  N_("Safety factors"),
  N_("Polar"),
  N_("Devices"),
  N_("Units"),
  N_("Interface"),
  N_("Appearance"),
  N_("Fonts"),
  N_("Vario Gauge and FLARM"),
  N_("Default task rules"),
  N_("InfoBox Cruise"),
  N_("InfoBox Circling"),
  N_("InfoBox Final Glide"),
  N_("InfoBox Auxiliary"),
  N_("Logger"),
  N_("Waypoint Edit"),
  N_("Experimental features"),
};

static const struct {
  enum DeviceConfig::port_type type;
  const TCHAR *label;
} port_types[] = {
#ifdef _WIN32_WCE
  { DeviceConfig::AUTO, N_("GPS Intermediate Driver") },
#endif
  { DeviceConfig::SERIAL, NULL } /* sentinel */
};

static const unsigned num_port_types =
  sizeof(port_types) / sizeof(port_types[0]) - 1;

static Font TempInfoWindowFont;
static Font TempTitleWindowFont;
static Font TempMapWindowFont;
static Font TempTitleSmallWindowFont;
static Font TempMapWindowBoldFont;
static Font TempCDIWindowFont; // New
static Font TempMapLabelFont;
static Font TempUseCustomFontsFont;

extern LOGFONT LogInfoBox;
extern LOGFONT LogTitle;
extern LOGFONT LogMap;
extern LOGFONT LogInfoBoxSmall;
extern LOGFONT LogMapBold;
extern LOGFONT LogCDI; // New
extern LOGFONT LogMapLabel;

extern bool dlgFontEditShowModal(const TCHAR * FontDescription,
                          const TCHAR * FontRegKey,
                          LOGFONT autoLogFont);

static bool changed = false;
static bool taskchanged = false;
static bool requirerestart = false;
static bool utcchanged = false;
static bool waypointneedsave = false;
static bool FontRegistryChanged=false;
static unsigned config_page;
static WndForm *wf = NULL;
static CheckBox *user_level;
TabbedControl *configuration_tabbed;
static WndButton *buttonPilotName = NULL;
static WndButton *buttonAircraftType = NULL;
static WndButton *buttonAircraftRego = NULL;
static WndButton *buttonLoggerID = NULL;
static WndButton *buttonCopy = NULL;
static WndButton *buttonPaste = NULL;

static void
UpdateButtons(void)
{
  TCHAR text[120];
  TCHAR val[100];
  if (buttonPilotName) {
    Profile::Get(szProfilePilotName, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, _("(blank)"));

    _stprintf(text, _T("%s: %s"), _("Pilot name"), val);
    buttonPilotName->SetCaption(text);
  }
  if (buttonAircraftType) {
    Profile::Get(szProfileAircraftType, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, _("(blank)"));

    _stprintf(text, _T("%s: %s"), _("Aircraft type"), val);
    buttonAircraftType->SetCaption(text);
  }
  if (buttonAircraftRego) {
    Profile::Get(szProfileAircraftRego, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, _("(blank)"));

    _stprintf(text, _T("%s: %s"), _("Competition ID"), val);
    buttonAircraftRego->SetCaption(text);
  }
  if (buttonLoggerID) {
    Profile::Get(szProfileLoggerID, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, _("(blank)"));

    _stprintf(text, _T("%s: %s"), _("Logger ID"), val);
    buttonLoggerID->SetCaption(text);
  }
}

static void
PageSwitched()
{
  config_page = configuration_tabbed->GetCurrentPage();

  TCHAR caption[64];
  _sntprintf(caption, 64, _T("%u %s"),
             config_page + 1, gettext(captions[config_page]));
  wf->SetCaption(caption);

  if (buttonCopy != NULL)
    buttonCopy->set_visible(config_page >= 14 && config_page <= 17);

  if (buttonPaste != NULL)
    buttonPaste->set_visible(config_page >= 14 && config_page <= 17);
}

static void
SetupDevice(DeviceDescriptor &device)
{
#ifdef ToDo
  device.DoSetup();
  wf->FocusNext(NULL);
#endif

  // this is a hack, devices dont jet support device dependant setup dialogs

  if (is_simulator() || !device.IsVega())
    return;

  changed = dlgConfigurationVarioShowModal();
}

static void
OnSetupDeviceAClicked(WndButton &button)
{
  SetupDevice(DeviceList[0]);

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  button.set_focus();
}


static void
OnSetupDeviceBClicked(WndButton &button)
{
  SetupDevice(DeviceList[1]);

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  button.set_focus();
}

static void
UpdateDeviceSetupButton(unsigned DeviceIdx, const TCHAR *Name)
{
  assert(DeviceIdx < 26);

  TCHAR button_name[] = _T("cmdSetupDeviceA");
  button_name[(sizeof(button_name) / sizeof(button_name[0])) - 2] += DeviceIdx;

  WndButton *wb = (WndButton *)wf->FindByName(button_name);
  if (wb != NULL)
    wb->set_visible(Name != NULL && _tcscmp(Name, _T("Vega")) == 0);
}

static Window *
OnCreateUserLevel(ContainerWindow &parent, int left, int top,
                  unsigned width, unsigned height,
                  const WindowStyle _style)
{
  CheckBoxStyle style(_style);
  style.tab_stop();
  user_level = new CheckBox();
  user_level->set(parent, _("Expert"), ID_USER_LEVEL,
                  left, top, width, height, style);
  user_level->set_font(Fonts::Map);
  user_level->set_checked(XCSoarInterface::UserLevel > 0);
  return user_level;
}

static bool
OnCommand(unsigned id)
{
  switch (id) {
  case ID_USER_LEVEL:
    if ((int)user_level->get_checked() != (int)XCSoarInterface::UserLevel) {
      XCSoarInterface::UserLevel = (int)user_level->get_checked();
      changed = true;
      Profile::Set(szProfileUserLevel,(int)XCSoarInterface::UserLevel);
      wf->FilterAdvanced(XCSoarInterface::UserLevel>0);
    }
    return true;

  default:
    return false;
  }
}

static void
OnDeviceAData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    break;
  case DataField::daPut:
  case DataField::daChange:
    UpdateDeviceSetupButton(0, Sender->GetAsString());
    break;
  }
}

static void
OnDeviceBData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    break;
  case DataField::daPut:
  case DataField::daChange:
    UpdateDeviceSetupButton(1, Sender->GetAsString());
    break;
  }
}

static void
ResetFonts(bool bUseCustom)
{
  if (bUseCustom) {
    Fonts::LoadCustomFont(&TempInfoWindowFont, szProfileFontInfoWindowFont);
    Fonts::LoadCustomFont(&TempTitleWindowFont, szProfileFontTitleWindowFont);
    Fonts::LoadCustomFont(&TempMapWindowFont, szProfileFontMapWindowFont);
    Fonts::LoadCustomFont(&TempTitleSmallWindowFont,
        szProfileFontTitleSmallWindowFont);
    Fonts::LoadCustomFont(&TempMapWindowBoldFont,
        szProfileFontMapWindowBoldFont);
    Fonts::LoadCustomFont(&TempCDIWindowFont, szProfileFontCDIWindowFont);
    Fonts::LoadCustomFont(&TempMapLabelFont, szProfileFontMapLabelFont);
  }

  Fonts::SetFont(&TempUseCustomFontsFont, LogMap);
  Fonts::SetFont(&TempInfoWindowFont, LogInfoBox);
  Fonts::SetFont(&TempTitleWindowFont, LogTitle);
  Fonts::SetFont(&TempMapWindowFont, LogMap);
  Fonts::SetFont(&TempTitleSmallWindowFont, LogInfoBoxSmall);
  Fonts::SetFont(&TempMapWindowBoldFont, LogMapBold);
  Fonts::SetFont(&TempCDIWindowFont, LogCDI);
  Fonts::SetFont(&TempMapLabelFont, LogMapLabel);
}

static void
ShowFontEditButtons(bool bVisible)
{
  WndProperty * wp;
  wp = (WndProperty*)wf->FindByName(_T("cmdInfoWindowFont"));
  if (wp)
    wp->set_visible(bVisible);

  wp = (WndProperty*)wf->FindByName(_T("cmdTitleWindowFont"));
  if (wp)
    wp->set_visible(bVisible);

  wp = (WndProperty*)wf->FindByName(_T("cmdMapWindowFont"));
  if (wp)
    wp->set_visible(bVisible);

  wp = (WndProperty*)wf->FindByName(_T("cmdTitleSmallWindowFont"));
  if (wp)
    wp->set_visible(bVisible);

  wp = (WndProperty*)wf->FindByName(_T("cmdMapWindowBoldFont"));
  if (wp)
    wp->set_visible(bVisible);

  wp = (WndProperty*)wf->FindByName(_T("cmdCDIWindowFont"));
  if (wp)
    wp->set_visible(bVisible);

  wp = (WndProperty*)wf->FindByName(_T("cmdMapLabelFont"));
  if (wp)
    wp->set_visible(bVisible);
}

static void
RefreshFonts(void)
{
  WndProperty * wp;

  wp = (WndProperty*)wf->FindByName(_T("prpUseCustomFonts"));
  if (wp) {
    bool bUseCustomFonts =
        ((DataFieldBoolean*)(wp->GetDataField()))->GetAsBoolean();
    ResetFonts(bUseCustomFonts);
    wp->SetFont(TempUseCustomFontsFont); // this font is never customized
    ShowFontEditButtons(bUseCustomFonts);
  }

  // now set SampleTexts on the Fonts frame
  wp = (WndProperty*)wf->FindByName(_T("prpInfoWindowFont"));
  if (wp)
    wp->SetFont(TempInfoWindowFont);

  wp = (WndProperty*)wf->FindByName(_T("prpTitleWindowFont"));
  if (wp)
    wp->SetFont(TempTitleWindowFont);

  wp = (WndProperty*)wf->FindByName(_T("prpMapWindowFont"));
  if (wp)
    wp->SetFont(TempMapWindowFont);

  wp = (WndProperty*)wf->FindByName(_T("prpTitleSmallWindowFont"));
  if (wp)
    wp->SetFont(TempTitleSmallWindowFont);

  wp = (WndProperty*)wf->FindByName(_T("prpMapWindowBoldFont"));
  if (wp)
    wp->SetFont(TempMapWindowBoldFont);

  wp = (WndProperty*)wf->FindByName(_T("prpCDIWindowFont"));
  if (wp)
    wp->SetFont(TempCDIWindowFont);

  wp = (WndProperty*)wf->FindByName(_T("prpMapLabelFont"));
  if (wp)
    wp->SetFont(TempMapLabelFont);

  // now fix the rest of the dlgConfiguration fonts:
  wf->SetFont(TempMapWindowBoldFont);
  wf->SetTitleFont(TempMapWindowBoldFont);
}

static void
OnUseCustomFontData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    break;

  case DataField::daPut:
    break;

  case DataField::daChange:
    RefreshFonts();

    break;
  }
}

static void
GetFontDescription(TCHAR Description[], const TCHAR * prpName, int iMaxLen)
{
  WndProperty * wp;
  wp = (WndProperty*)wf->FindByName(prpName);
  if (wp)
    _tcsncpy(Description, wp->GetCaption(), iMaxLen - 1);
}

static void
OnEditInfoWindowFontClicked(gcc_unused WndButton &button)
{
  // updates registry for font info and updates LogFont values
#define MAX_EDITFONT_DESC_LEN 100
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpInfoWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontInfoWindowFont,
                           LogInfoBox)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditTitleWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpTitleWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontTitleWindowFont,
                           LogTitle)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditMapWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontMapWindowFont,
                           LogMap)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditTitleSmallWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpTitleSmallWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontTitleSmallWindowFont,
                           LogInfoBoxSmall)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditMapWindowBoldFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapWindowBoldFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontMapWindowBoldFont,
                           LogMapBold)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditCDIWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpCDIWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontCDIWindowFont,
                           LogCDI)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditMapLabelFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapLabelFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, szProfileFontMapLabelFont,
                           LogMapLabel)) {
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnAircraftRegoClicked(gcc_unused WndButton &button)
{
  TCHAR Temp[100];
  if (buttonAircraftRego) {
    Profile::Get(szProfileAircraftRego, Temp, 100);
    if (dlgTextEntryShowModal(Temp, 100)) {
      Profile::Set(szProfileAircraftRego, Temp);
      changed = true;
    }
  }
  UpdateButtons();
}

static void
OnAircraftTypeClicked(gcc_unused WndButton &button)
{
  TCHAR Temp[100];
  if (buttonAircraftType) {
    Profile::Get(szProfileAircraftType, Temp, 100);
    if (dlgTextEntryShowModal(Temp, 100)) {
      Profile::Set(szProfileAircraftType, Temp);
      changed = true;
    }
  }
  UpdateButtons();
}

static void
OnPilotNameClicked(gcc_unused WndButton &button)
{
  TCHAR Temp[100];
  if (buttonPilotName) {
    Profile::Get(szProfilePilotName, Temp, 100);
    if (dlgTextEntryShowModal(Temp, 100)) {
      Profile::Set(szProfilePilotName, Temp);
      changed = true;
    }
  }
  UpdateButtons();
}

static void
OnLoggerIDClicked(gcc_unused WndButton &button)
{
  TCHAR Temp[100];
  if (buttonLoggerID) {
    Profile::Get(szProfileLoggerID, Temp, 100);
    if (dlgTextEntryShowModal(Temp, 100)) {
      Profile::Set(szProfileLoggerID, Temp);
      changed = true;
    }
    ReadAssetNumber();
  }
  UpdateButtons();
}

static void
OnAirspaceColoursClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(true);
}

static void
OnAirspaceModeClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(false);
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

static int cpyInfoBox[10];

static int
page2mode(void)
{
  return configuration_tabbed->GetCurrentPage() - 14;
}

static const TCHAR *const info_box_mode_names[] = {
  _T("Circling"),
  _T("Cruise"),
  _T("FinalGlide"),
  _T("Aux"),
};

static void InfoBoxPropName(TCHAR *name, int item, int mode) {
  _stprintf(name, _T("prpInfoBox%s%1d"), info_box_mode_names[mode], item);
}

static WndProperty *
FindInfoBoxField(int mode, int item)
{
  TCHAR name[80];
  InfoBoxPropName(name, item, mode);
  return (WndProperty*)wf->FindByName(name);
}

static void
OnCopy(gcc_unused WndButton &button)
{
  int mode = page2mode();
  if ((mode < 0) || (mode > 3))
    return;

  for (unsigned item = 0; item < InfoBoxLayout::numInfoWindows; item++) {
    WndProperty *wp = FindInfoBoxField(mode, item);
    if (wp)
      cpyInfoBox[item] = wp->GetDataField()->GetAsInteger();
  }
}

static void
OnPaste(gcc_unused WndButton &button)
{
  int mode = page2mode();
  if ((mode < 0) || (mode > 3) || (cpyInfoBox[0] < 0))
    return;

  if(MessageBoxX(_("Overwrite?"),
                 _("InfoBox paste"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  for (unsigned item = 0; item < InfoBoxLayout::numInfoWindows; item++) {
    WndProperty *wp = FindInfoBoxField(mode, item);
    if (wp &&
        cpyInfoBox[item] >= 0 &&
        (unsigned)cpyInfoBox[item] < NUMSELECTSTRINGS) {
      wp->GetDataField()->Set(cpyInfoBox[item]);
      wp->RefreshDisplay();
    }
  }
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
  case '6':
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    configuration_tabbed->PreviousPage();
    PageSwitched();
    //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

  case '7':
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    configuration_tabbed->NextPage();
    PageSwitched();
    //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

static void
SetLocalTime(void)
{
  WndProperty* wp;
  TCHAR temp[20];
  Units::TimeToText(temp, (int)TimeLocal((int)(XCSoarInterface::Basic().Time)));

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  if (wp) {
    wp->SetText(temp);
    wp->RefreshDisplay();
  }
}

static void
OnUTCData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  //  WndProperty* wp;
  int ival;

  switch(Mode){
  case DataField::daGet:
    break;
  case DataField::daPut:
  case DataField::daChange:
    ival = iround(Sender->GetAsFloat()*3600.0);
    if (XCSoarInterface::SettingsComputer().UTCOffset != ival) {
      XCSoarInterface::SetSettingsComputer().UTCOffset = ival;
      utcchanged = true;
    }
    SetLocalTime();
    break;
  }
}

static int lastSelectedPolarFile = -1;

static void
OnPolarFileData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  WndProperty* wp;

  switch(Mode){
  case DataField::daGet:
    break;
  case DataField::daPut:
  case DataField::daChange:
    if (Sender->GetAsString() != NULL && _tcscmp(Sender->GetAsString(), _T("")) != 0){
      // then ... set Polar Tape to Winpilot
      wp = (WndProperty *)wf->FindByName(_T("prpPolarType"));
      if (wp != NULL){
        wp->GetDataField()->SetAsInteger(POLARUSEWINPILOTFILE);
        wp->RefreshDisplay();
      }
    }
    break;
  }
}

static void
OnPolarTypeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  WndProperty* wp;

  switch(Mode){
  case DataField::daGet:
    break;
  case DataField::daPut:
  case DataField::daChange:
    wp = (WndProperty *)wf->FindByName(_T("prpPolarFile"));

    if (Sender->GetAsInteger() != POLARUSEWINPILOTFILE){
      // then ... clear Winpilot File if Polar Type is not WinpilotFile
      if (wp != NULL && wp->GetDataField()->GetAsBoolean()) {
        lastSelectedPolarFile = wp->GetDataField()->GetAsInteger();
        wp->GetDataField()->SetAsInteger(-1);
        wp->RefreshDisplay();
      }
    } else {
      if (wp != NULL && wp->GetDataField()->GetAsInteger() <= 0 && lastSelectedPolarFile > 0){
        wp->GetDataField()->SetAsInteger(lastSelectedPolarFile);
        wp->RefreshDisplay();
      }
    }
    break;
  }
}

extern void OnInfoBoxHelp(WindowControl * Sender);

static void
OnWaypointNewClicked(WindowControl * Sender)
{
  (void)Sender;

  Waypoint edit_waypoint = way_points.create(XCSoarInterface::Basic().Location);
  if (dlgWaypointEditShowModal(edit_waypoint)) {
    if (edit_waypoint.Name.size()) {
      way_points.append(edit_waypoint);
      waypointneedsave = true;
    }
  }
}

static void
OnWaypointEditClicked(WindowControl * Sender)
{
  (void)Sender;

  const Waypoint *way_point = dlgWayPointSelect(XCSoarInterface::main_window,
                                                XCSoarInterface::Basic().Location);
  if (way_point){
    if (way_points.get_writable(*way_point)) {
      Waypoint wp_copy = *way_point;
      if (dlgWaypointEditShowModal(wp_copy)) {
        waypointneedsave = true;
        way_points.replace(*way_point, wp_copy);
      }
    } else {
      MessageBoxX(_("Waypoint not editable"), _T("Error"), MB_OK);
    }
  }
}

static void
AskWaypointSave(void)
{
  /// @todo terrain check???
  if (WayPointFile::WaypointsOutOfRangeSetting != 2 ||
      MessageBoxX(_("Waypoints excluded, save anyway?"),
                  _("Waypoints outside terrain"),
                  MB_YESNO | MB_ICONQUESTION) == IDYES) {
    WayPointGlue::SaveWaypoints(way_points);
    WaypointFileChanged= true;
    changed = true;
  }

  waypointneedsave = false;
}

static void
OnWaypointSaveClicked(WindowControl * Sender)
{
  (void)Sender;

  AskWaypointSave();
}

static void
OnWaypointDeleteClicked(WindowControl * Sender)
{
  (void)Sender;
#ifdef OLD_TASK
  int res;
  res = dlgWayPointSelect(XCSoarInterface::Basic().Location);
  if (res != -1){
    if(MessageBoxX(way_points.get(res).Name,
                   _("Delete Waypoint?"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      way_points.set(res).FileNum = -1;
      waypointneedsave = true;
    }
  }
#endif
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnAirspaceColoursClicked),
  DeclareCallBackEntry(OnAirspaceModeClicked),
  DeclareCallBackEntry(OnUTCData),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnSetupDeviceAClicked),
  DeclareCallBackEntry(OnSetupDeviceBClicked),
  DeclareCallBackEntry(OnInfoBoxHelp),
  DeclareCallBackEntry(OnWaypointNewClicked),
  DeclareCallBackEntry(OnWaypointDeleteClicked),
  DeclareCallBackEntry(OnWaypointEditClicked),
  DeclareCallBackEntry(OnWaypointSaveClicked),
  DeclareCallBackEntry(OnPolarFileData),
  DeclareCallBackEntry(OnPolarTypeData),
  DeclareCallBackEntry(OnDeviceAData),
  DeclareCallBackEntry(OnDeviceBData),
  DeclareCallBackEntry(OnUseCustomFontData),
  DeclareCallBackEntry(OnEditInfoWindowFontClicked),
  DeclareCallBackEntry(OnEditTitleWindowFontClicked),
  DeclareCallBackEntry(OnEditMapWindowFontClicked),
  DeclareCallBackEntry(OnEditTitleSmallWindowFontClicked),
  DeclareCallBackEntry(OnEditMapWindowBoldFontClicked),
  DeclareCallBackEntry(OnEditCDIWindowFontClicked),
  DeclareCallBackEntry(OnEditMapLabelFontClicked),
  DeclareCallBackEntry(OnCreateUserLevel),
  DeclareCallBackEntry(NULL)
};

/**
 * Convert a page number mode returned by page2mode() to a
 * InfoBoxManager mode number.
 */
static unsigned
page_mode_to_ibm_mode(unsigned page_mode)
{
  static const unsigned table[] = { 1, 0, 2, 3 };

  assert(page_mode < sizeof(table) / sizeof(table[0]));
  return table[page_mode];
}

static void
SetInfoBoxSelector(unsigned item, int mode)
{
  WndProperty *wp = FindInfoBoxField(mode, item);
  if (wp == NULL)
    return;

  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  for (unsigned i = 0; i < NUMSELECTSTRINGS; i++)
    dfe->addEnumText(gettext(InfoBoxManager::GetTypeDescription(i)));

  dfe->Sort(0);

  dfe->Set(InfoBoxManager::getType(item, page_mode_to_ibm_mode(mode)));
  wp->RefreshDisplay();
}

static void
GetInfoBoxSelector(unsigned item, int mode)
{
  WndProperty *wp = FindInfoBoxField(mode, item);
  if (wp == NULL)
    return;

  int itnew = wp->GetDataField()->GetAsInteger();
  int it = InfoBoxManager::getType(item, page_mode_to_ibm_mode(mode));

  if (it == itnew)
    return;

  changed = true;
  InfoBoxManager::setType(item, itnew, page_mode_to_ibm_mode(mode));
  Profile::SetInfoBoxes(item, InfoBoxManager::getTypeAll(item));
}

static TCHAR szPolarFile[MAX_PATH];
static TCHAR szAirspaceFile[MAX_PATH];
static TCHAR szAdditionalAirspaceFile[MAX_PATH];
static TCHAR szWaypointFile[MAX_PATH];
static TCHAR szAdditionalWaypointFile[MAX_PATH];
static TCHAR szTerrainFile[MAX_PATH];
static TCHAR szTopologyFile[MAX_PATH];
static TCHAR szAirfieldFile[MAX_PATH];
static TCHAR szLanguageFile[MAX_PATH];
static TCHAR szStatusFile[MAX_PATH];
static TCHAR szInputFile[MAX_PATH];
static TCHAR szMapFile[MAX_PATH];
static DeviceConfig device_config[NUMDEV];
static int dwDeviceIndex1 = 0;
static int dwDeviceIndex2 = 0;
static int Speed = 1; // default is knots
static int TaskSpeed = 2; // default is kph
static int Distance = 2; // default is km
static int Lift = 0;
static int Altitude = 0; //default ft
static int Temperature = 0; //default is celcius
static TCHAR temptext[MAX_PATH];

static void
SetupDeviceFields(const DeviceDescriptor &device, const DeviceConfig &config,
                  int &driver_index,
                  WndProperty *port_field, WndProperty *speed_field,
                  WndProperty *driver_field, WndButton *setup_button)
{
  static const TCHAR *const COMMPort[] = {
    _T("COM1"), _T("COM2"), _T("COM3"), _T("COM4"),
    _T("COM5"), _T("COM6"), _T("COM7"), _T("COM8"),
    _T("COM9"), _T("COM10"), _T("COM0"),
    NULL
  };

  static const TCHAR *const tSpeed[] = {
    _T("1200"), _T("2400"), _T("4800"), _T("9600"),
    _T("19200"), _T("38400"), _T("57600"), _T("115200"),
    NULL
  };

  if (port_field != NULL) {
    DataFieldEnum *dfe = (DataFieldEnum *)port_field->GetDataField();

    for (unsigned i = 0; port_types[i].label != NULL; i++) {
      dfe->addEnumText(gettext(port_types[i].label));

      if (port_types[i].type == config.port_type)
        dfe->Set(i);
    }

    dfe->addEnumTexts(COMMPort);

    switch (config.port_type) {
    case DeviceConfig::SERIAL:
      dfe->Set(config.port_index + num_port_types);
      break;
    }

    port_field->RefreshDisplay();
  }

  if (speed_field != NULL) {
    DataFieldEnum *dfe = (DataFieldEnum *)speed_field->GetDataField();
    dfe->addEnumTexts(tSpeed);

    dfe->Set(config.speed_index);
    speed_field->RefreshDisplay();
  }

  if (driver_field) {
    DataFieldEnum *dfe = (DataFieldEnum *)driver_field->GetDataField();

    const TCHAR *DeviceName;
    for (unsigned i = 0; (DeviceName = devRegisterGetName(i)) != NULL; i++) {
      dfe->addEnumText(DeviceName);

      if (!is_simulator()) {
        if (device.IsDriver(DeviceName))
            driver_index = i;
      } else {
        if (_tcscmp(DeviceName, config.driver_name) == 0)
          driver_index = i;
      }
    }

    dfe->Sort(1);
    dfe->Set(driver_index);

    driver_field->RefreshDisplay();
  }

  if (setup_button != NULL)
    setup_button->set_visible(is_simulator()
                              ? _tcscmp(config.driver_name, _T("Vega")) == 0
                              : device.IsVega());
}

static void
setVariables()
{
  WndProperty *wp;

  buttonPilotName = ((WndButton *)wf->FindByName(_T("cmdPilotName")));
  if (buttonPilotName)
    buttonPilotName->SetOnClickNotify(OnPilotNameClicked);

  buttonAircraftType = ((WndButton *)wf->FindByName(_T("cmdAircraftType")));
  if (buttonAircraftType)
    buttonAircraftType->SetOnClickNotify(OnAircraftTypeClicked);

  buttonAircraftRego = ((WndButton *)wf->FindByName(_T("cmdAircraftRego")));
  if (buttonAircraftRego)
    buttonAircraftRego->SetOnClickNotify(OnAircraftRegoClicked);

  buttonLoggerID = ((WndButton *)wf->FindByName(_T("cmdLoggerID")));
  if (buttonLoggerID)
    buttonLoggerID->SetOnClickNotify(OnLoggerIDClicked);

  buttonCopy = ((WndButton *)wf->FindByName(_T("cmdCopy")));
  if (buttonCopy)
    buttonCopy->SetOnClickNotify(OnCopy);

  buttonPaste = ((WndButton *)wf->FindByName(_T("cmdPaste")));
  if (buttonPaste)
    buttonPaste->SetOnClickNotify(OnPaste);

  UpdateButtons();

  for (unsigned i = 0; i < NUMDEV; ++i)
    Profile::GetDeviceConfig(i, device_config[i]);

  SetupDeviceFields(DeviceList[0], device_config[0], dwDeviceIndex1,
                    (WndProperty*)wf->FindByName(_T("prpComPort1")),
                    (WndProperty*)wf->FindByName(_T("prpComSpeed1")),
                    (WndProperty*)wf->FindByName(_T("prpComDevice1")),
                    (WndButton *)wf->FindByName(_T("cmdSetupDeviceA")));

  SetupDeviceFields(DeviceList[1], device_config[1], dwDeviceIndex2,
                    (WndProperty*)wf->FindByName(_T("prpComPort2")),
                    (WndProperty*)wf->FindByName(_T("prpComSpeed2")),
                    (WndProperty*)wf->FindByName(_T("prpComDevice2")),
                    (WndButton *)wf->FindByName(_T("cmdSetupDeviceB")));

  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();

  const SETTINGS_MAP &settings_map =
    XCSoarInterface::SettingsMap();

  wp = (WndProperty*)wf->FindByName(_T("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("All on"));
    dfe->addEnumText(_("Clip"));
    dfe->addEnumText(_("Auto"));
    dfe->addEnumText(_("All below"));
    dfe->Set(settings_computer.AltitudeMode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpUTCOffset"),
      iround(settings_computer.UTCOffset / 1800.0) / 2.0);

  SetLocalTime();

  LoadFormProperty(*wf, _T("prpClipAltitude"), ugAltitude,
                   settings_computer.ClipAltitude);
  LoadFormProperty(*wf, _T("prpAltWarningMargin"), ugAltitude,
                   settings_computer.AltWarningMargin);

  LoadFormProperty(*wf, _T("prpAutoZoom"), settings_map.AutoZoom);
  LoadFormProperty(*wf, _T("prpAirspaceOutline"),
                   settings_map.bAirspaceBlackOutline);
  LoadFormProperty(*wf, _T("prpLockSettingsInFlight"),
                   XCSoarInterface::LockSettingsInFlight);
  LoadFormProperty(*wf, _T("prpLoggerShortName"),
                   settings_computer.LoggerShortName);
  LoadFormProperty(*wf, _T("prpDebounceTimeout"),
                   XCSoarInterface::debounceTimeout);

  wp = (WndProperty*)wf->FindByName(_T("prpEnableFLARMMap"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("OFF"));
    dfe->addEnumText(_("ON/Fixed"));
    dfe->addEnumText(_("ON/Scaled"));
    dfe->Set(XCSoarInterface::SettingsMap().EnableFLARMMap);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpEnableFLARMGauge"),
                   XCSoarInterface::SettingsMap().EnableFLARMGauge);
  LoadFormProperty(*wf, _T("prpAirspaceWarnings"),
                   settings_computer.EnableAirspaceWarnings);
  LoadFormProperty(*wf, _T("prpWarningTime"),
                   settings_computer.WarningTime);
  LoadFormProperty(*wf, _T("prpAcknowledgementTime"),
                   settings_computer.AcknowledgementTime);

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointLabels"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Names"));
    dfe->addEnumText(_("Numbers"));
    dfe->addEnumText(_("First 5"));
    dfe->addEnumText(_("None"));
    dfe->addEnumText(_("First 3"));
    dfe->addEnumText(_("Names in task"));
    dfe->addEnumText(_("First word"));
    dfe->Set(XCSoarInterface::SettingsMap().DisplayTextType);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpEnableTerrain"),
                   XCSoarInterface::SettingsMap().EnableTerrain);
  LoadFormProperty(*wf, _T("prpEnableTopology"),
                   XCSoarInterface::SettingsMap().EnableTopology);
  LoadFormProperty(*wf, _T("prpCirclingZoom"),
                   XCSoarInterface::SettingsMap().CircleZoom);

  wp = (WndProperty*)wf->FindByName(_T("prpOrientation"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Track up"));
    dfe->addEnumText(_("North up"));
    dfe->addEnumText(_("North circling"));
    dfe->addEnumText(_("Target circling"));
    dfe->addEnumText(_("North/track"));
    dfe->Set(XCSoarInterface::SettingsMap().DisplayOrientation);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpMenuTimeout"),
                   XCSoarInterface::MenuTimeoutMax / 2);

  LoadFormProperty(*wf, _T("prpSafetyAltitudeArrival"), ugAltitude,
                   settings_computer.safety_height_arrival);
  LoadFormProperty(*wf, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                   settings_computer.safety_height_terrain);

  wp = (WndProperty*)wf->FindByName(_T("prpFinalGlideTerrain"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("OFF"));
    dfe->addEnumText(_("Line"));
    dfe->addEnumText(_("Shade"));
    dfe->Set(settings_computer.FinalGlideTerrain);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpEnableNavBaroAltitude"),
                   settings_computer.EnableNavBaroAltitude);

  wp = (WndProperty*)wf->FindByName(_T("prpWindArrowStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Arrow head"));
    dfe->addEnumText(_("Full arrow"));
    wp->GetDataField()->Set(XCSoarInterface::SettingsMap().WindArrowStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Manual"));
    dfe->addEnumText(_("Circling"));
    dfe->addEnumText(_("ZigZag"));
    dfe->addEnumText(_("Both"));
    wp->GetDataField()->Set(settings_computer.AutoWindMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoMcMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Final glide"));
    dfe->addEnumText(_("Average climb"));
    dfe->addEnumText(_("Both"));
    wp->GetDataField()->Set((int)settings_computer.auto_mc_mode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointsOutOfRange"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Ask"));
    dfe->addEnumText(_("Include"));
    dfe->addEnumText(_("Exclude"));
    wp->GetDataField()->Set(WayPointFile::WaypointsOutOfRangeSetting);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpBlockSTF"),
                   settings_computer.EnableBlockSTF);
  LoadFormProperty(*wf, _T("prpFAIFinishHeight"),
                   settings_computer.ordered_defaults.fai_finish);

  wp = (WndProperty*)wf->FindByName(_T("prpOLCRules"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(rule_as_text((OLCRules)0).c_str()));
    dfe->addEnumText(gettext(rule_as_text((OLCRules)1).c_str()));
    dfe->addEnumText(gettext(rule_as_text((OLCRules)2).c_str()));
    dfe->Set(settings_computer.olc_rules);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpHandicap"),
                   settings_computer.olc_handicap);

  if(!Profile::Get(szProfileSpeedUnitsValue,Speed)) {
    Profile::Set(szProfileSpeedUnitsValue, Speed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Statue"));
    dfe->addEnumText(_("Nautical"));
    dfe->addEnumText(_("Metric"));
    dfe->Set(Speed);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    const TCHAR *const units_lat_lon[] = {
      _T("DDMMSS"),
      _T("DDMMSS.ss"),
      _T("DDMM.mmm"),
      _T("DD.dddd"),
      NULL
    };

    dfe->addEnumTexts(units_lat_lon);
    dfe->Set(Units::GetCoordinateFormat());
    wp->RefreshDisplay();
  }

  if(!Profile::Get(szProfileTaskSpeedUnitsValue,TaskSpeed)) {
    Profile::Set(szProfileTaskSpeedUnitsValue, TaskSpeed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTaskSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Statute"));
    dfe->addEnumText(_("Nautical"));
    dfe->addEnumText(_("Metric"));
    dfe->Set(TaskSpeed);
    wp->RefreshDisplay();
  }

  if(!Profile::Get(szProfileDistanceUnitsValue,Distance)) {
    Profile::Set(szProfileDistanceUnitsValue, Distance);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsDistance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Statute"));
    dfe->addEnumText(_("Nautical"));
    dfe->addEnumText(_("Metric"));
    dfe->Set(Distance);
    wp->RefreshDisplay();
  }

  if(!Profile::Get(szProfileAltitudeUnitsValue,Altitude)) {
    Profile::Set(szProfileAltitudeUnitsValue, Altitude);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsAltitude"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Feet"));
    dfe->addEnumText(_("Meters"));
    dfe->Set(Altitude);
    wp->RefreshDisplay();
  }

  if(!Profile::Get(szProfileTemperatureUnitsValue,Temperature)) {
    Profile::Set(szProfileTemperatureUnitsValue, Temperature);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTemperature"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("C"));
    dfe->addEnumText(_("F"));
    dfe->Set(Temperature);
    wp->RefreshDisplay();
  }

  if(!Profile::Get(szProfileLiftUnitsValue,Lift)) {
    Profile::Set(szProfileLiftUnitsValue, Lift);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLift"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Knots"));
    dfe->addEnumText(_("M/s"));
    dfe->addEnumText(_("ft/min"));
    dfe->Set(Lift);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpTrailDrift"),
                   XCSoarInterface::SettingsMap().EnableTrailDrift);

  LoadFormProperty(*wf, _T("prpSetSystemTimeFromGPS"),
                   XCSoarInterface::SettingsMap().SetSystemTimeFromGPS);
  LoadFormProperty(*wf, _T("prpIgnoreNMEAChecksum"),
                   NMEAParser::ignore_checksum);
  LoadFormProperty(*wf, _T("prpAbortSafetyUseCurrent"),
                   settings_computer.safety_mc_use_current);
  LoadFormProperty(*wf, _T("prpDisableAutoLogger"),
                   !settings_computer.DisableAutoLogger);

  LoadFormProperty(*wf, _T("prpSafetyMacCready"), ugVerticalSpeed,
                   settings_computer.safety_mc);

  LoadFormProperty(*wf, _T("prpRiskGamma"), settings_computer.risk_gamma);

  wp = (WndProperty*)wf->FindByName(_T("prpTrail"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"));
    dfe->addEnumText(_("Long"));
    dfe->addEnumText(_("Short"));
    dfe->addEnumText(_("Full"));
    dfe->Set(XCSoarInterface::SettingsMap().TrailActive);
    wp->RefreshDisplay();
  }

  // VENTA3 VisualGlide
  wp = (WndProperty*)wf->FindByName(_T("prpVGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"));
    dfe->addEnumText(_("Steady"));
    dfe->addEnumText(_("Moving"));
    dfe->Set(XCSoarInterface::SettingsMap().VisualGlide);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpMaxManoeuveringSpeed"), ugHorizontalSpeed,
                   settings_computer.SafetySpeed);

  LoadFormProperty(*wf, _T("prpBallastSecsToEmpty"),
                   settings_computer.BallastSecsToEmpty);

  wp = (WndProperty*)wf->FindByName(_T("prpPolarType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(PolarLabels);

    unsigned i = 0;
    bool ok = true;
    while (ok) {
      const TCHAR *name;
      name = GetWinPilotPolarInternalName(i);
      if (!name) {
        ok=false;
      } else {
        dfe->addEnumText(name);
      }
      i++;
    }
    dfe->Sort();
    dfe->Set(settings_computer.POLARID);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfilePolarFile, szPolarFile, MAX_PATH);
  _tcscpy(temptext,szPolarFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.plr"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileAirspaceFile, szAirspaceFile, MAX_PATH);
  _tcscpy(temptext,szAirspaceFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.txt"));
    dfe->ScanDirectoryTop(_T("*.air"));
    dfe->ScanDirectoryTop(_T("*.sua"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileAdditionalAirspaceFile,
                    szAdditionalAirspaceFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalAirspaceFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpAdditionalAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.txt"));
    dfe->ScanDirectoryTop(_T("*.air"));
    dfe->ScanDirectoryTop(_T("*.sua"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileWayPointFile, szWaypointFile, MAX_PATH);
  _tcscpy(temptext,szWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.dat"));
    dfe->ScanDirectoryTop(_T("*.xcw"));
    dfe->ScanDirectoryTop(_T("*.cup"));
    dfe->ScanDirectoryTop(_T("*.wpz"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileAdditionalWayPointFile,
                    szAdditionalWaypointFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.dat"));
    dfe->ScanDirectoryTop(_T("*.xcw"));
    dfe->ScanDirectoryTop(_T("*.cup"));
    dfe->ScanDirectoryTop(_T("*.wpz"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileMapFile, szMapFile, MAX_PATH);
  _tcscpy(temptext,szMapFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpMapFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.xcm"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileTerrainFile, szTerrainFile, MAX_PATH);
  _tcscpy(temptext,szTerrainFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.jp2"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileTopologyFile, szTopologyFile, MAX_PATH);
  _tcscpy(temptext,szTopologyFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.tpl"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileAirfieldFile, szAirfieldFile, MAX_PATH);
  _tcscpy(temptext,szAirfieldFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileLanguageFile, szLanguageFile, MAX_PATH);
  _tcscpy(temptext,szLanguageFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.xcl"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileStatusFile, szStatusFile, MAX_PATH);
  _tcscpy(temptext,szStatusFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.xcs"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  Profile::Get(szProfileInputFile, szInputFile, MAX_PATH);
  _tcscpy(temptext,szInputFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.xci"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

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
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Default"));
    dfe->addEnumText(_("Keyboard"));
    dfe->addEnumText(_("HighScore Style"));
    dfe->Set(Appearance.TextInputStyle);
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
    dfe->Set(g_eDialogStyle);
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
    dfe->addEnumText(_("8 Top (Portrait)"));  // 1
    dfe->addEnumText(_("8 Bottom (Portrait)"));  // 2
    dfe->addEnumText(_("8 Left + Right (Landscape)"));  // 3
    dfe->addEnumText(_("8 Left (Landscape)"));  // 4
    dfe->addEnumText(_("8 Right (Landscape)")); // 5
    dfe->addEnumText(_("9 Right + Vario (Landscape)"));  // 6
    dfe->addEnumText(_("5 Right (Square)")); // 7
    dfe->Set(InfoBoxLayout::InfoBoxGeometry);
    wp->RefreshDisplay();
  }

#if defined(_WIN32_WCE) && !defined(GNAV)
// VENTA-ADDON Model change config menu 11
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Generic"));
    dfe->addEnumText(_("HP31x"));
    dfe->addEnumText(_("MedionP5"));
    dfe->addEnumText(_("MIO"));
    dfe->addEnumText(_("Nokia500")); // VENTA3
    dfe->addEnumText(_("PN6000"));
    dfe->Set((int)GlobalModelType);
    wp->RefreshDisplay();
  }
#endif

  wp = (WndProperty*)wf->FindByName(_T("prpExtendedVisualGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Normal"));
    dfe->addEnumText(_("Extended"));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(XCSoarInterface::SettingsMap().ExtendedVisualGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpGestures"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Disabled"));
    dfe->addEnumText(_("Enabled"));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(settings_computer.EnableGestures);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("15 seconds"));
    dfe->addEnumText(_("30 seconds"));
    dfe->addEnumText(_("60 seconds"));
    dfe->addEnumText(_("90 seconds"));
    dfe->addEnumText(_("2 minutes"));
    dfe->addEnumText(_("3 minutes"));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(settings_computer.AverEffTime);
    wp->RefreshDisplay();
  }

// Fonts
  wp = (WndProperty*)wf->FindByName(_T("prpUseCustomFonts"));
  if (wp) {
    DataFieldBoolean * dfb = (DataFieldBoolean*) wp->GetDataField();
    dfb->Set(Appearance.UseCustomFonts);
    ShowFontEditButtons(dfb->GetAsBoolean());
    wp->RefreshDisplay();
    RefreshFonts();
  }
  FontRegistryChanged=false;


// end fonts

  wp = (WndProperty*)wf->FindByName(_T("prpAppCompassAppearance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Normal"));
    dfe->addEnumText(_("White outline"));
    dfe->Set(Appearance.CompassAppearance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndFinalGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Default"));
    dfe->addEnumText(_("Alternate"));
    dfe->Set(Appearance.IndFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Winpilot"));
    dfe->addEnumText(_("Alternate"));
    dfe->addEnumText(_("Green/Orange"));
    dfe->Set(Appearance.IndLandable);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEnableExternalTriggerCruise"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("OFF"));
    dfe->addEnumText(_("Flap"));
    dfe->addEnumText(_("SC"));
    dfe->Set(settings_computer.EnableExternalTriggerCruise);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAppInverseInfoBox"),
                   Appearance.InverseInfoBox);
  LoadFormProperty(*wf, _T("prpAppDefaultMapWidth"),
                   Appearance.DefaultMapWidth);
  LoadFormProperty(*wf, _T("prpGliderScreenPosition"),
                   iround(XCSoarInterface::SettingsMap().GliderScreenPosition));
  LoadFormProperty(*wf, _T("prpTerrainContrast"),
                   iround(XCSoarInterface::SettingsMap().TerrainContrast*100/255));
  LoadFormProperty(*wf, _T("prpTerrainBrightness"),
                   iround(XCSoarInterface::SettingsMap().TerrainBrightness*100/255));

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainRamp"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Low lands"));
    dfe->addEnumText(_("Mountainous"));
    dfe->addEnumText(_("Imhof 7"));
    dfe->addEnumText(_("Imhof 4"));
    dfe->addEnumText(_("Imhof 12"));
    dfe->addEnumText(_("Imhof Atlas"));
    dfe->addEnumText(_("ICAO"));
    dfe->addEnumText(_("Grey"));
    dfe->Set(XCSoarInterface::SettingsMap().TerrainRamp);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAppInfoBoxColors"), Appearance.InfoBoxColors);
  LoadFormProperty(*wf, _T("prpAppAveNeedle"), Appearance.GaugeVarioAveNeedle);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                   Appearance.GaugeVarioSpeedToFly);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                   Appearance.GaugeVarioAvgText );
  LoadFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                   Appearance.GaugeVarioGross);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioMc"), Appearance.GaugeVarioMc);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioBugs"), Appearance.GaugeVarioBugs);
  LoadFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                   Appearance.GaugeVarioBallast);

  wp = (WndProperty*)wf->FindByName(_T("prpAutoBlank"));
  if (wp) {
    if (is_altair() || !is_embedded())
      wp->hide();
    wp->GetDataField()->Set(XCSoarInterface::SettingsMap().EnableAutoBlank);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoBacklight")); // VENTA4
  if (wp) {
    wp->set_visible(model_is_hp31x());
    wp->GetDataField()->Set(CommonInterface::EnableAutoBacklight);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoSoundVolume")); // VENTA4
  if (wp) {
    wp->set_visible(is_embedded());
    wp->GetDataField()->Set(CommonInterface::EnableAutoSoundVolume);
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

  LoadFormProperty(*wf, _T("prpLoggerTimeStepCruise"),
                   settings_computer.LoggerTimeStepCruise);
  LoadFormProperty(*wf, _T("prpLoggerTimeStepCircling"),
                   settings_computer.LoggerTimeStepCircling);
  LoadFormProperty(*wf, _T("prpSnailWidthScale"),
                   XCSoarInterface::SettingsMap().SnailWidthScale);

  for (unsigned i = 0; i < 4; i++) {
    for (unsigned j = 0; j < InfoBoxLayout::numInfoWindows; j++) {
      SetInfoBoxSelector(j, i);
    }
  }
}

static bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, DisplayTextType_t &value)
{
  WndProperty* wp = (WndProperty*)wfm->FindByName(field);
  if (wp) {
    if ((int)value != wp->GetDataField()->GetAsInteger()) {
      value = (DisplayTextType_t)wp->GetDataField()->GetAsInteger();
      return true;
    }
  }
  return false;
}

static bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, const TCHAR *reg,
                         DisplayTextType_t &value) {
  if (SaveFormProperty(wfm, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

static bool
FinishFileField(const WndProperty &wp, const TCHAR *profile_key,
                const TCHAR *old_value)
{
  const DataFieldFileReader *dfe =
    (const DataFieldFileReader *)wp.GetDataField();
  _tcscpy(temptext, dfe->GetPathFile());
  ContractLocalPath(temptext);

  if (_tcscmp(temptext, old_value) == 0)
    return false;

  Profile::Set(profile_key, temptext);
  changed = true;
  return true;
}

static bool
FinishFileField(WndForm &wf, const TCHAR *control_name,
                const TCHAR *profile_key, const TCHAR *old_value)
{
  const WndProperty *wp = (const WndProperty *)wf.FindByName(control_name);
  return wp != NULL && FinishFileField(*wp, profile_key, old_value);
}

/**
 * @return true if the value has changed
 */
static bool
FinishPortField(DeviceConfig &config, WndProperty &port_field)
{
  int value = port_field.GetDataField()->GetAsInteger();

  if (value < (int)num_port_types) {
    if (port_types[value].type == config.port_type)
      return false;

    config.port_type = port_types[value].type;
    return true;
  } else {
    value -= num_port_types;

    if (config.port_type == DeviceConfig::SERIAL &&
        value == (int)config.port_index)
      return false;

    config.port_type = DeviceConfig::SERIAL;
    config.port_index = value;
    return true;
  }
}

static bool
FinishDeviceFields(DeviceConfig &config, int &driver_index,
                   WndProperty *port_field, WndProperty *speed_field,
                   WndProperty *driver_field)
{
  bool changed = false;

  if (port_field != NULL && FinishPortField(config, *port_field))
    changed = true;

  if (speed_field != NULL &&
      (int)config.speed_index != speed_field->GetDataField()->GetAsInteger()) {
    config.speed_index = speed_field->GetDataField()->GetAsInteger();
    changed = true;
  }

  if (driver_field != NULL &&
      driver_index != driver_field->GetDataField()->GetAsInteger()) {
    driver_index = driver_field->GetDataField()->GetAsInteger();
    _tcscpy(config.driver_name, devRegisterGetName(driver_index));
    changed = true;
  }

  return changed;
}

static void
PrepareConfigurationDialog()
{
  gcc_unused ScopeBusyIndicator busy;

  WndProperty *wp;

  if (Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_CONFIGURATION_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_CONFIGURATION"));
  }

  if (wf == NULL)
    return;

  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetCommandCallback(OnCommand);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  configuration_tabbed = ((TabbedControl *)wf->FindByName(_T("tabbed")));
  assert(configuration_tabbed != NULL);

  wf->FilterAdvanced(XCSoarInterface::UserLevel>0);

  if (!is_embedded() || !is_altair()) {
    wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
    if (wp) {
      wp->hide();
    }
  }

  for (int item=0; item<10; item++) {
    cpyInfoBox[item] = -1;
  }

  setVariables();

  /* restore previous page */
  configuration_tabbed->SetCurrentPage(config_page);
  PageSwitched();

  changed = false;
  taskchanged = false;
  requirerestart = false;
  utcchanged = false;
  waypointneedsave = false;
}

void dlgConfigurationShowModal(void)
{
  PrepareConfigurationDialog();

  wf->ShowModal();

  /* save page number for next time this dialog is opened */
  config_page = configuration_tabbed->GetCurrentPage();

  // TODO enhancement: implement a cancel button that skips all this
  // below after exit.

  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();

  changed |= SaveFormProperty(wf, _T("prpAbortSafetyUseCurrent"),
                              szProfileAbortSafetyUseCurrent,
                              settings_computer.safety_mc_use_current);

  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(_T("prpDisableAutoLogger"));
  if (wp) { // GUI label is "Enable Auto Logger"
    if (!settings_computer.DisableAutoLogger
        != wp->GetDataField()->GetAsBoolean()) {
      settings_computer.DisableAutoLogger =
        !(wp->GetDataField()->GetAsBoolean());
      Profile::Set(szProfileDisableAutoLogger,
                    settings_computer.DisableAutoLogger);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    fixed val = Units::ToSysVSpeed(wp->GetDataField()->GetAsFixed());
    if (settings_computer.safety_mc != val) {
      settings_computer.safety_mc = val;
      Profile::Set(szProfileSafetyMacCready,
                    iround(settings_computer.safety_mc*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRiskGamma"));
  if (wp) {
    fixed val = wp->GetDataField()->GetAsFixed();
    if (settings_computer.risk_gamma != val) {
      settings_computer.risk_gamma = val;
      Profile::Set(szProfileRiskGamma,
                    iround(settings_computer.risk_gamma*10));
      changed = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpSetSystemTimeFromGPS"),
                              szProfileSetSystemTimeFromGPS,
                              XCSoarInterface::SetSettingsMap().SetSystemTimeFromGPS);

  changed |= SaveFormProperty(wf, _T("prpIgnoreNMEAChecksum"),
                              szProfileIgnoreNMEAChecksum,
                              NMEAParser::ignore_checksum);

  changed |= SaveFormProperty(wf, _T("prpTrailDrift"),
                              szProfileTrailDrift,
                              XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  changed |= SaveFormProperty(wf, _T("prpTrail"),
                              szProfileSnailTrail,
                              XCSoarInterface::SetSettingsMap().TrailActive);

// VENTA3: do not save VisualGlide to registry or profile

  wp = (WndProperty*)wf->FindByName(_T("prpPolarType"));
  if (wp) {
    if (settings_computer.POLARID != (unsigned)wp->GetDataField()->GetAsInteger()) {
      settings_computer.POLARID = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfilePolarID, (int &)settings_computer.POLARID);
      PolarFileChanged = true;
      changed = true;
    }
  }

  short tmp = settings_computer.AltitudeMode;
  changed |= SaveFormProperty(wf, _T("prpAirspaceDisplay"),
                              szProfileAltMode, tmp);
  settings_computer.AltitudeMode = (AirspaceDisplayMode_t)tmp;

  changed |= SaveFormProperty(wf, _T("prpLockSettingsInFlight"),
                              szProfileLockSettingsInFlight,
                              XCSoarInterface::LockSettingsInFlight);

  changed |= SaveFormProperty(wf, _T("prpLoggerShortName"),
                              szProfileLoggerShort,
                              settings_computer.LoggerShortName);

  changed |= SaveFormProperty(wf, _T("prpEnableFLARMMap"),
                              szProfileEnableFLARMMap,
                              XCSoarInterface::SetSettingsMap().EnableFLARMMap);

  changed |= SaveFormProperty(wf, _T("prpEnableFLARMGauge"),
                              szProfileEnableFLARMGauge,
                              XCSoarInterface::SetSettingsMap().EnableFLARMGauge);

  wp = (WndProperty*)wf->FindByName(_T("prpDebounceTimeout"));
  if (wp) {
    if ((int)XCSoarInterface::debounceTimeout != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::debounceTimeout = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileDebounceTimeout, (int)XCSoarInterface::debounceTimeout);
      changed = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpAirspaceOutline"),
                              szProfileAirspaceBlackOutline,
                              XCSoarInterface::SetSettingsMap().bAirspaceBlackOutline);

  changed |= SaveFormProperty(wf, _T("prpAutoZoom"),
                              szProfileAutoZoom,
                              XCSoarInterface::SetSettingsMap().AutoZoom);

  int ival;

  wp = (WndProperty*)wf->FindByName(_T("prpUTCOffset"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()*3600.0);
    if ((settings_computer.UTCOffset != ival)||(utcchanged)) {
      settings_computer.UTCOffset = ival;

      // have to do this because registry variables can't be negative!
      int lival = settings_computer.UTCOffset;
      if (lival<0) { lival+= 24*3600; }
      Profile::Set(szProfileUTCOffset, lival);
      changed = true;
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpClipAltitude"), ugAltitude,
                              settings_computer.ClipAltitude,
                              szProfileClipAlt);

  changed |= SaveFormProperty(*wf, _T("prpAltWarningMargin"),
                              ugAltitude, settings_computer.AltWarningMargin,
                              szProfileAltMargin);

  changed |= SaveFormProperty(wf, _T("prpAirspaceWarnings"),
                              szProfileAirspaceWarning,
                              settings_computer.EnableAirspaceWarnings);

  changed |= SaveFormProperty(wf, _T("prpWarningTime"),
                              szProfileWarningTime,
                              settings_computer.WarningTime);

  changed |= SaveFormProperty(wf, _T("prpAcknowledgementTime"),
                              szProfileAcknowledgementTime,
                              settings_computer.AcknowledgementTime);

  changed |= SaveFormProperty(wf, _T("prpWaypointLabels"),
                              szProfileDisplayText,
                              XCSoarInterface::SetSettingsMap().DisplayTextType);

  changed |= SaveFormProperty(wf, _T("prpEnableTerrain"),
                              szProfileDrawTerrain,
                              XCSoarInterface::SetSettingsMap().EnableTerrain);

  changed |= SaveFormProperty(wf, _T("prpEnableTopology"),
                              szProfileDrawTopology,
                              XCSoarInterface::SetSettingsMap().EnableTopology);

  changed |= SaveFormProperty(wf, _T("prpCirclingZoom"),
                              szProfileCircleZoom,
                              XCSoarInterface::SetSettingsMap().CircleZoom);

  wp = (WndProperty*)wf->FindByName(_T("prpOrientation"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().DisplayOrientation != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().DisplayOrientation = (DisplayOrientation_t)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileDisplayUpValue,
                    XCSoarInterface::SettingsMap().DisplayOrientation);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMenuTimeout"));
  if (wp) {
    if (XCSoarInterface::MenuTimeoutMax != wp->GetDataField()->GetAsInteger()*2) {
      XCSoarInterface::MenuTimeoutMax = wp->GetDataField()->GetAsInteger()*2;
      Profile::Set(szProfileMenuTimeout,XCSoarInterface::MenuTimeoutMax);
      changed = true;
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpSafetyAltitudeArrival"), ugAltitude,
                              settings_computer.safety_height_arrival,
                              szProfileSafetyAltitudeArrival);

  changed |= SaveFormProperty(*wf, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                              settings_computer.safety_height_terrain,
                              szProfileSafetyAltitudeTerrain);

  changed |= SaveFormProperty(wf, _T("prpAutoWind"), szProfileAutoWind,
                              settings_computer.AutoWindMode);

  changed |= SaveFormProperty(wf, _T("prpWindArrowStyle"),
                              szProfileWindArrowStyle,
                              XCSoarInterface::SetSettingsMap().WindArrowStyle);

  int auto_mc_mode = (int)settings_computer.auto_mc_mode;
  changed |= SaveFormProperty(wf, _T("prpAutoMcMode"), szProfileAutoMcMode,
                              auto_mc_mode);
  settings_computer.auto_mc_mode = (TaskBehaviour::AutoMCMode_t)auto_mc_mode;

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointsOutOfRange"));
  if (wp) {
    if (WayPointFile::WaypointsOutOfRangeSetting !=
        wp->GetDataField()->GetAsInteger()) {
      WayPointFile::WaypointsOutOfRangeSetting =
        wp->GetDataField()->GetAsInteger();

      Profile::Set(szProfileWaypointsOutOfRange,
                   WayPointFile::WaypointsOutOfRangeSetting);

      WaypointFileChanged = true;
      changed = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpEnableNavBaroAltitude"),
                              szProfileEnableNavBaroAltitude,
                              settings_computer.EnableNavBaroAltitude);

  changed |= SaveFormProperty(wf, _T("prpFinalGlideTerrain"),
                              szProfileFinalGlideTerrain,
                              settings_computer.FinalGlideTerrain);

  changed |= SaveFormProperty(wf, _T("prpBlockSTF"),
                              szProfileBlockSTF,
                              settings_computer.EnableBlockSTF);

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  if (wp) {
    if ((int)Speed != wp->GetDataField()->GetAsInteger()) {
      Speed = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileSpeedUnitsValue, Speed);
      changed = true;

      switch (Speed) {
      case 0:
        Units::SetUserSpeedUnit(unStatuteMilesPerHour);
        Units::SetUserWindSpeedUnit(unStatuteMilesPerHour);
        break;
      case 1:
        Units::SetUserSpeedUnit(unKnots);
        Units::SetUserWindSpeedUnit(unKnots);
        break;
      case 2:
      default:
        Units::SetUserSpeedUnit(unKiloMeterPerHour);
        Units::SetUserWindSpeedUnit(unKiloMeterPerHour);
        break;
      }
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  if (wp) {
    if ((int)Units::GetCoordinateFormat() != wp->GetDataField()->GetAsInteger()) {
      Units::SetCoordinateFormat(
          (CoordinateFormats_t)wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileLatLonUnits, Units::GetCoordinateFormat());
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTaskSpeed"));
  if (wp) {
    if ((int)TaskSpeed != wp->GetDataField()->GetAsInteger()) {
      TaskSpeed = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileTaskSpeedUnitsValue, TaskSpeed);
      changed = true;

      switch (TaskSpeed) {
      case 0:
        Units::SetUserTaskSpeedUnit(unStatuteMilesPerHour);
        break;
      case 1:
        Units::SetUserTaskSpeedUnit(unKnots);
        break;
      case 2:
      default:
        Units::SetUserTaskSpeedUnit(unKiloMeterPerHour);
        break;
      }
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsDistance"));
  if (wp) {
    if ((int)Distance != wp->GetDataField()->GetAsInteger()) {
      Distance = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileDistanceUnitsValue, Distance);
      changed = true;

      switch (Distance) {
      case 0:
        Units::SetUserDistanceUnit(unStatuteMiles);
        break;
      case 1:
        Units::SetUserDistanceUnit(unNauticalMiles);
        break;
      case 2:
      default:
        Units::SetUserDistanceUnit(unKiloMeter);
        break;
      }
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLift"));
  if (wp) {
    if ((int)Lift != wp->GetDataField()->GetAsInteger()) {
      Lift = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileLiftUnitsValue, Lift);
      changed = true;

      switch (Lift) {
      case 0:
        Units::SetUserVerticalSpeedUnit(unKnots);
        break;
      case 1:
      default:
        Units::SetUserVerticalSpeedUnit(unMeterPerSecond);
        break;
      case 2:
        Units::SetUserVerticalSpeedUnit(unFeetPerMinute);
        break;
      }
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsAltitude"));
  if (wp) {
    if ((int)Altitude != wp->GetDataField()->GetAsInteger()) {
      Altitude = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileAltitudeUnitsValue, Altitude);
      changed = true;

      switch (Altitude) {
      case 0:
        Units::SetUserAltitudeUnit(unFeet);
        break;
      case 1:
      default:
        Units::SetUserAltitudeUnit(unMeter);
        break;
      }
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTemperature"));
  if (wp) {
    if ((int)Temperature != wp->GetDataField()->GetAsInteger()) {
      Temperature = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileTemperatureUnitsValue, Temperature);
      changed = true;

      switch (Temperature) {
      case 0:
        Units::SetUserTemperatureUnit(unGradCelcius);
        break;
      case 1:
      default:
        Units::SetUserTemperatureUnit(unGradFahrenheit);
        break;
      }
    }
  }

  {
    unsigned t= settings_computer.olc_rules;
    changed |= SaveFormProperty(wf, _T("prpOLCRules"), szProfileOLCRules,
                                t);
    settings_computer.olc_rules = (OLCRules)t;
  }

  changed |= SaveFormProperty(wf, _T("prpHandicap"), szProfileHandicap,
                              settings_computer.olc_handicap);

  PolarFileChanged = FinishFileField(*wf, _T("prpPolarFile"),
                                     szProfilePolarFile, szPolarFile);

  WaypointFileChanged =
    FinishFileField(*wf, _T("prpWaypointFile"),
                    szProfileWayPointFile, szWaypointFile) ||
    FinishFileField(*wf, _T("prpAdditionalWaypointFile"),
                    szProfileAdditionalWayPointFile, szAdditionalWaypointFile);

  AirspaceFileChanged =
    FinishFileField(*wf, _T("prpAirspaceFile"),
                    szProfileAirspaceFile, szAirspaceFile) ||
    FinishFileField(*wf, _T("prpAdditionalAirspaceFile"),
                    szProfileAdditionalAirspaceFile, szAdditionalAirspaceFile);

  MapFileChanged = FinishFileField(*wf, _T("prpMapFile"),
                                   szProfileMapFile, szMapFile);

  TerrainFileChanged = FinishFileField(*wf, _T("prpTerrainFile"),
                                       szProfileTerrainFile, szTerrainFile);

  TopologyFileChanged = FinishFileField(*wf, _T("prpTopologyFile"),
                                        szProfileTopologyFile, szTopologyFile);

  AirfieldFileChanged = FinishFileField(*wf, _T("prpAirfieldFile"),
                                        szProfileAirfieldFile, szAirfieldFile);

  if (FinishFileField(*wf, _T("prpLanguageFile"),
                      szProfileLanguageFile, szLanguageFile))
    requirerestart = true;

  if (FinishFileField(*wf, _T("prpStartMaxSpeedMargin"),
                      szProfileStatusFile, szStatusFile))
    requirerestart = true;

  if (FinishFileField(*wf, _T("prpInputFile"),
                      szProfileInputFile, szInputFile))
    requirerestart = true;

  changed |= SaveFormProperty(wf, _T("prpBallastSecsToEmpty"),
                              szProfileBallastSecsToEmpty,
                              settings_computer.BallastSecsToEmpty);

  changed |= SaveFormProperty(*wf, _T("prpMaxManoeuveringSpeed"),
                              ugHorizontalSpeed, settings_computer.SafetySpeed,
                              szProfileSafteySpeed);

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndFinalGlide"));
  if (wp) {
    if (Appearance.IndFinalGlide != (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndFinalGlide = (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppIndFinalGlide, Appearance.IndFinalGlide);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppCompassAppearance"));
  if (wp) {
    if (Appearance.CompassAppearance != (CompassAppearance_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.CompassAppearance = (CompassAppearance_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppCompassAppearance,
                    Appearance.CompassAppearance);
      changed = true;
      requirerestart = true;
    }
  }

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

  wp = (WndProperty*)wf->FindByName(_T("prpExtendedVisualGlide")); // VENTA4
  if (wp) {
    if (XCSoarInterface::SettingsMap().ExtendedVisualGlide != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().ExtendedVisualGlide = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileExtendedVisualGlide,
                    XCSoarInterface::SettingsMap().ExtendedVisualGlide);
      changed = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpGestures")); // VENTA6
  if (wp) {
    if (settings_computer.EnableGestures != wp->GetDataField()->GetAsBoolean()) {
      settings_computer.EnableGestures =
          !settings_computer.EnableGestures;
      Profile::Set(szProfileGestures,
                   settings_computer.EnableGestures);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime")); // VENTA6
  if (wp) {
    if (settings_computer.AverEffTime != wp->GetDataField()->GetAsInteger()) {
      settings_computer.AverEffTime = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileAverEffTime,
                   settings_computer.AverEffTime);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    if (InfoBoxLayout::InfoBoxGeometry != (unsigned)wp->GetDataField()->GetAsInteger()) {
      InfoBoxLayout::InfoBoxGeometry = (unsigned)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileInfoBoxGeometry, InfoBoxLayout::InfoBoxGeometry);
      changed = true;
      requirerestart = true;
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

  //Fonts
  wp = (WndProperty*)wf->FindByName(_T("prpUseCustomFonts"));
  if (wp) {
    DataFieldBoolean * dfb = (DataFieldBoolean*) wp->GetDataField();
    if (dfb) {
      if ((Appearance.UseCustomFonts != dfb->GetAsBoolean())
          || (Appearance.UseCustomFonts && FontRegistryChanged)) {
        Appearance.UseCustomFonts = !Appearance.UseCustomFonts;
        Profile::Set(szProfileUseCustomFonts, Appearance.UseCustomFonts);
        changed = true;
        requirerestart = true;
      }
    }
  }
  TempUseCustomFontsFont.reset();

  TempInfoWindowFont.reset();
  TempTitleWindowFont.reset();
  TempMapWindowFont.reset();
  TempTitleSmallWindowFont.reset();
  TempMapWindowBoldFont.reset();
  TempCDIWindowFont.reset();
  TempMapLabelFont.reset();

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

  wp = (WndProperty*)wf->FindByName(_T("prpTextInput"));
  if (wp)
    {
      if (Appearance.TextInputStyle != (TextInputStyle_t)(wp->GetDataField()->GetAsInteger()))
        {
          Appearance.TextInputStyle = (TextInputStyle_t)(wp->GetDataField()->GetAsInteger());
          Profile::Set(szProfileAppTextInputStyle, Appearance.TextInputStyle);
          changed = true;
        }
    }

  wp = (WndProperty*)wf->FindByName(_T("prpDialogStyle"));
  if (wp)
    {
      if (g_eDialogStyle != (DialogStyle_t)(wp->GetDataField()->GetAsInteger()))
        {
          g_eDialogStyle = (DialogStyle_t)(wp->GetDataField()->GetAsInteger());
          Profile::Set(szProfileAppDialogStyle, g_eDialogStyle);
          changed = true;
        }
    }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppIndLandable, Appearance.IndLandable);
      changed = true;
      requirerestart = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpEnableExternalTriggerCruise"),
                              szProfileEnableExternalTriggerCruise,
                              settings_computer.EnableExternalTriggerCruise);

  wp = (WndProperty*)wf->FindByName(_T("prpAppInverseInfoBox"));
  if (wp) {
    if ((int)(Appearance.InverseInfoBox) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InverseInfoBox = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppInverseInfoBox,Appearance.InverseInfoBox);
      requirerestart = true;
      changed = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpGliderScreenPosition"),
                              szProfileGliderScreenPosition,
                              XCSoarInterface::SetSettingsMap().GliderScreenPosition);

  wp = (WndProperty*)wf->FindByName(_T("prpAppDefaultMapWidth"));
  if (wp) {
    if ((int)(Appearance.DefaultMapWidth) != wp->GetDataField()->GetAsInteger()) {
      Appearance.DefaultMapWidth = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileAppDefaultMapWidth,Appearance.DefaultMapWidth);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppAveNeedle"));
  if (wp) {
    if ((int)(Appearance.GaugeVarioAveNeedle) !=
        wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAveNeedle =
        (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppAveNeedle,Appearance.GaugeVarioAveNeedle);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxColors"));
  if (wp) {
    if ((int)(Appearance.InfoBoxColors) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InfoBoxColors = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppInfoBoxColors,Appearance.InfoBoxColors);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    if ((int)(Appearance.GaugeVarioSpeedToFly) != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioSpeedToFly = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppGaugeVarioSpeedToFly,Appearance.GaugeVarioSpeedToFly);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppGaugeVarioAvgText"));
  if (wp) {
    if ((int)Appearance.GaugeVarioAvgText != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAvgText = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppGaugeVarioAvgText,Appearance.GaugeVarioAvgText);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppGaugeVarioGross"));
  if (wp) {
    if ((int)Appearance.GaugeVarioGross != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioGross = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppGaugeVarioGross,Appearance.GaugeVarioGross);
      changed = true;
      requirerestart = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpAppGaugeVarioMc"),
                              szProfileAppGaugeVarioMc,
                              Appearance.GaugeVarioMc);

  changed |= SaveFormProperty(wf, _T("prpAppGaugeVarioBugs"),
                              szProfileAppGaugeVarioBugs,
                              Appearance.GaugeVarioBugs);

  changed |= SaveFormProperty(wf, _T("prpAppGaugeVarioBallast"),
                              szProfileAppGaugeVarioBallast,
                              Appearance.GaugeVarioBallast);

#ifdef HAVE_BLANK
  wp = (WndProperty*)wf->FindByName(_T("prpAutoBlank"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().EnableAutoBlank != (wp->GetDataField()->GetAsInteger()!=0)) {
      XCSoarInterface::SetSettingsMap().EnableAutoBlank = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAutoBlank, XCSoarInterface::SettingsMap().EnableAutoBlank);
      changed = true;
    }
  }
#endif

  changed |= SaveFormProperty(wf, _T("prpAutoBacklight"),
                              szProfileAutoBacklight,
                              CommonInterface::EnableAutoBacklight);

  changed |= SaveFormProperty(wf, _T("prpAutoSoundVolume"),
                              szProfileAutoSoundVolume,
                              CommonInterface::EnableAutoSoundVolume);

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainContrast"));
  if (wp) {
    if (iround(XCSoarInterface::SettingsMap().TerrainContrast*100/255) !=
        wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().TerrainContrast = (short)iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      Profile::Set(szProfileTerrainContrast,XCSoarInterface::SettingsMap().TerrainContrast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainBrightness"));
  if (wp) {
    if (iround(XCSoarInterface::SettingsMap().TerrainBrightness*100/255) !=
        wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().TerrainBrightness = (short)iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      Profile::Set(szProfileTerrainBrightness,
                    XCSoarInterface::SettingsMap().TerrainBrightness);
      changed = true;
    }
  }

  changed |= SaveFormProperty(wf, _T("prpTerrainRamp"), szProfileTerrainRamp,
                              XCSoarInterface::SetSettingsMap().TerrainRamp);

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

  changed |= taskchanged;

  changed |= SaveFormProperty(wf, _T("prpLoggerTimeStepCruise"),
                              szProfileLoggerTimeStepCruise,
                              settings_computer.LoggerTimeStepCruise);

  changed |= SaveFormProperty(wf, _T("prpLoggerTimeStepCircling"),
                              szProfileLoggerTimeStepCircling,
                              settings_computer.LoggerTimeStepCircling);

  DevicePortChanged =
    FinishDeviceFields(device_config[0], dwDeviceIndex1,
                       (WndProperty*)wf->FindByName(_T("prpComPort1")),
                       (WndProperty*)wf->FindByName(_T("prpComSpeed1")),
                       (WndProperty*)wf->FindByName(_T("prpComDevice1")));

  DevicePortChanged =
    FinishDeviceFields(device_config[1], dwDeviceIndex2,
                       (WndProperty*)wf->FindByName(_T("prpComPort2")),
                       (WndProperty*)wf->FindByName(_T("prpComSpeed2")),
                       (WndProperty*)wf->FindByName(_T("prpComDevice2"))) ||
    DevicePortChanged;

  if (DevicePortChanged)
    changed = true;

  wp = (WndProperty*)wf->FindByName(_T("prpSnailWidthScale"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().SnailWidthScale != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().SnailWidthScale = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileSnailWidthScale,
                    XCSoarInterface::SettingsMap().SnailWidthScale);
      changed = true;
      requirerestart = true;
    }
  }

  if (DevicePortChanged)
    for (unsigned i = 0; i < NUMDEV; ++i)
      Profile::SetDeviceConfig(i, device_config[i]);

  for (unsigned i = 0; i < 4; ++i)
    for (unsigned j = 0; j < InfoBoxLayout::numInfoWindows; ++j)
      GetInfoBoxSelector(j, i);

  if (waypointneedsave) {
    way_points.optimise();
    if (MessageBoxX(_("Save changes to waypoint file?"), _("Waypoints edited"),
                    MB_YESNO | MB_ICONQUESTION) == IDYES)
      AskWaypointSave();
  }

  if (!is_embedded() && DevicePortChanged)
    requirerestart = true;

  if (changed) {
    Profile::Save();

    if (!requirerestart)
      MessageBoxX(_("Changes to configuration saved."),
                  _T(""), MB_OK);
    else
      MessageBoxX(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                  _T(""), MB_OK);
  }

  delete wf;
  wf = NULL;
}
