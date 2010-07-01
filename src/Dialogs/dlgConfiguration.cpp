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
#include "InfoBoxManager.hpp"
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
#include "InfoBoxLayout.hpp"

#include <assert.h>

enum {
  ID_USER_LEVEL = 100,
};

static const TCHAR *const captions[] = {
  _T("1 Site"),
  _T("2 Airspace"),
  _T("3 Map Display"),
  _T("4 Terrain Display"),
  _T("5 Glide Computer"),
  _T("6 Safety factors"),
  _T("7 Polar"),
  _T("8 Devices"),
  _T("9 Units"),
  _T("10 Interface"),
  _T("11 Appearance"),
  _T("12 Fonts"),
  _T("13 Vario Gauge and FLARM"),
  _T("14 Default task rules"),
  _T("15 InfoBox Cruise"),
  _T("16 InfoBox Circling"),
  _T("17 InfoBox Final Glide"),
  _T("18 InfoBox Auxiliary"),
  _T("19 Logger"),
  _T("20 Waypoint Edit"),
  _T("21 Experimental features"),
};

static const struct {
  enum DeviceConfig::port_type type;
  const TCHAR *label;
} port_types[] = {
#ifdef _WIN32_WCE
  { DeviceConfig::AUTO, _T("GPS Intermediate Driver") },
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
static Font TempStatisticsFont;
static Font TempUseCustomFontsFont;

extern LOGFONT InfoWindowLogFont;
extern LOGFONT TitleWindowLogFont;
extern LOGFONT MapWindowLogFont;
extern LOGFONT TitleSmallWindowLogFont;
extern LOGFONT MapWindowBoldLogFont;
extern LOGFONT CDIWindowLogFont; // New
extern LOGFONT MapLabelLogFont;
extern LOGFONT StatisticsLogFont;

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
static WndForm *wf=NULL;
static CheckBox *user_level;
TabbedControl *configuration_tabbed;
static WndButton *buttonPilotName=NULL;
static WndButton *buttonAircraftType=NULL;
static WndButton *buttonAircraftRego=NULL;
static WndButton *buttonLoggerID=NULL;
static WndButton *buttonCopy=NULL;
static WndButton *buttonPaste=NULL;

static void UpdateButtons(void) {
  TCHAR text[120];
  TCHAR val[100];
  if (buttonPilotName) {
    Profile::Get(szProfilePilotName, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, gettext(_T("(blank)")));

    _stprintf(text,_T("%s: %s"), gettext(_T("Pilot name")), val);
    buttonPilotName->SetCaption(text);
  }
  if (buttonAircraftType) {
    Profile::Get(szProfileAircraftType, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, gettext(_T("(blank)")));

    _stprintf(text,_T("%s: %s"), gettext(_T("Aircraft type")), val);
    buttonAircraftType->SetCaption(text);
  }
  if (buttonAircraftRego) {
    Profile::Get(szProfileAircraftRego, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, gettext(_T("(blank)")));

    _stprintf(text,_T("%s: %s"), gettext(_T("Competition ID")), val);
    buttonAircraftRego->SetCaption(text);
  }
  if (buttonLoggerID) {
    Profile::Get(szProfileLoggerID, val, 100);
    if (string_is_empty(val))
      _tcscpy(val, gettext(_T("(blank)")));

    _stprintf(text,_T("%s: %s"), gettext(_T("Logger ID")), val);
    buttonLoggerID->SetCaption(text);
  }
}

static void
PageSwitched()
{
  config_page = configuration_tabbed->GetCurrentPage();

  wf->SetCaption(gettext(captions[config_page]));

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
  user_level->set(parent, gettext(_T("Expert")), ID_USER_LEVEL,
                  left, top, width, height, style);
  user_level->set_font(MapWindowFont);
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


static void OnDeviceAData(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
  case DataField::daGet:
    break;
  case DataField::daPut:
  case DataField::daChange:
    UpdateDeviceSetupButton(0, Sender->GetAsString());
    break;
  }

}

static void OnDeviceBData(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
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
    LoadCustomFont(&TempInfoWindowFont, szProfileFontInfoWindowFont);
    LoadCustomFont(&TempTitleWindowFont, szProfileFontTitleWindowFont);
    LoadCustomFont(&TempMapWindowFont, szProfileFontMapWindowFont);
    LoadCustomFont(&TempTitleSmallWindowFont, szProfileFontTitleSmallWindowFont);
    LoadCustomFont(&TempMapWindowBoldFont, szProfileFontMapWindowBoldFont);
    LoadCustomFont(&TempCDIWindowFont, szProfileFontCDIWindowFont);
    LoadCustomFont(&TempMapLabelFont, szProfileFontMapLabelFont);
    LoadCustomFont(&TempStatisticsFont, szProfileFontStatisticsFont);
  }

  InitializeFont (&TempUseCustomFontsFont, MapWindowLogFont);
  InitializeFont(&TempInfoWindowFont, InfoWindowLogFont);
  InitializeFont(&TempTitleWindowFont, TitleWindowLogFont);
  InitializeFont(&TempMapWindowFont, MapWindowLogFont);
  InitializeFont(&TempTitleSmallWindowFont, TitleSmallWindowLogFont);
  InitializeFont(&TempMapWindowBoldFont, MapWindowBoldLogFont);
  InitializeFont(&TempCDIWindowFont, CDIWindowLogFont);
  InitializeFont(&TempMapLabelFont, MapLabelLogFont);
  InitializeFont(&TempStatisticsFont, StatisticsLogFont);
}

static void ShowFontEditButtons(bool bVisible) {
  WndProperty * wp;
  wp = (WndProperty*)wf->FindByName(_T("cmdInfoWindowFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdTitleWindowFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdMapWindowFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdTitleSmallWindowFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdMapWindowBoldFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdCDIWindowFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdMapLabelFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdStatisticsFont"));
  if (wp) {
    wp->set_visible(bVisible);
  }
}


static void RefreshFonts(void) {

  WndProperty * wp;

  wp = (WndProperty*)wf->FindByName(_T("prpUseCustomFonts"));
  if (wp) {
    bool bUseCustomFonts= ((DataFieldBoolean*)(wp->GetDataField()))->GetAsBoolean();
    ResetFonts(bUseCustomFonts);
    wp->SetFont(TempUseCustomFontsFont); // this font is never customized
    ShowFontEditButtons(bUseCustomFonts);

  }

// now set SampleTexts on the Fonts frame
  wp = (WndProperty*)wf->FindByName(_T("prpInfoWindowFont"));
  if (wp) {
    wp->SetFont(TempInfoWindowFont);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTitleWindowFont"));
  if (wp) {
    wp->SetFont(TempTitleWindowFont);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpMapWindowFont"));
  if (wp) {
    wp->SetFont(TempMapWindowFont);
  }


  wp = (WndProperty*)wf->FindByName(_T("prpTitleSmallWindowFont"));
  if (wp) {
    wp->SetFont(TempTitleSmallWindowFont);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpMapWindowBoldFont"));
  if (wp) {
    wp->SetFont(TempMapWindowBoldFont);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpCDIWindowFont"));
  if (wp) {
    wp->SetFont(TempCDIWindowFont);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpMapLabelFont"));
  if (wp) {
    wp->SetFont(TempMapLabelFont);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpStatisticsFont"));
  if (wp) {
    wp->SetFont(TempStatisticsFont);
  }

  // now fix the rest of the dlgConfiguration fonts:
  wf->SetFont(TempMapWindowBoldFont);
  wf->SetTitleFont(TempMapWindowBoldFont);
}



static void OnUseCustomFontData(DataField *Sender, DataField::DataAccessKind_t Mode) {

  switch(Mode){
  case DataField::daGet:
    break;

  case DataField::daPut:
    break;

  case DataField::daChange:
    RefreshFonts();

    break;
  }
}

static void GetFontDescription(TCHAR Description[], const TCHAR * prpName, int iMaxLen)
{
  WndProperty * wp;
  wp = (WndProperty*)wf->FindByName(prpName);
  if (wp) {
    _tcsncpy(Description, wp->GetCaption(), iMaxLen-1);
  }
}

static void
OnEditInfoWindowFontClicked(gcc_unused WndButton &button)
{
  // updates registry for font info and updates LogFont values
#define MAX_EDITFONT_DESC_LEN 100
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpInfoWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szProfileFontInfoWindowFont,
                            InfoWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditTitleWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpTitleWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontTitleWindowFont,
                           TitleWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditMapWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontMapWindowFont,
                           MapWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditTitleSmallWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpTitleSmallWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontTitleSmallWindowFont,
                           TitleSmallWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditMapWindowBoldFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapWindowBoldFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontMapWindowBoldFont,
                           MapWindowBoldLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditCDIWindowFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpCDIWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontCDIWindowFont,
                           CDIWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditMapLabelFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapLabelFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontMapLabelFont,
                           MapLabelLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnEditStatisticsFontClicked(gcc_unused WndButton &button)
{
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpStatisticsFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                           szProfileFontStatisticsFont,
                           StatisticsLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void
OnAircraftRegoClicked(gcc_unused WndButton &button)
{
  TCHAR Temp[100];
  if (buttonAircraftRego) {
    Profile::Get(szProfileAircraftRego,Temp,100);
    if (dlgTextEntryShowModal(Temp,100)) {
      Profile::Set(szProfileAircraftRego,Temp);
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
    Profile::Get(szProfileAircraftType,Temp,100);
    if (dlgTextEntryShowModal(Temp,100)){
      Profile::Set(szProfileAircraftType,Temp);
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
    Profile::Get(szProfilePilotName,Temp,100);
    if (dlgTextEntryShowModal(Temp,100)){
      Profile::Set(szProfilePilotName,Temp);
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
    Profile::Get(szProfileLoggerID,Temp,100);
    if(dlgTextEntryShowModal(Temp,100)){
      Profile::Set(szProfileLoggerID,Temp);
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

static int page2mode(void) {
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
  if ((mode<0)||(mode>3)) {
    return;
  }

  for (unsigned item = 0; item < numInfoWindows; item++) {
    WndProperty *wp = FindInfoBoxField(mode, item);
    if (wp) {
      cpyInfoBox[item] = wp->GetDataField()->GetAsInteger();
    }
  }
}

static void
OnPaste(gcc_unused WndButton &button)
{
  int mode = page2mode();
  if ((mode<0)||(mode>3)||(cpyInfoBox[0]<0)) {
    return;
  }

  if(MessageBoxX(gettext(_T("Overwrite?")),
                 gettext(_T("InfoBox paste")),
                 MB_YESNO|MB_ICONQUESTION) != IDYES)
    return;

  for (unsigned item = 0; item < numInfoWindows; item++) {
    WndProperty *wp = FindInfoBoxField(mode, item);
    if (wp && (cpyInfoBox[item] >=0 ) &&
        ((unsigned)cpyInfoBox[item] < NUMSELECTSTRINGS)) {
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

static void SetLocalTime(void) {
  WndProperty* wp;
  TCHAR temp[20];
  Units::TimeToText(temp,
                    (int)TimeLocal((int)(XCSoarInterface::Basic().Time)));

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  if (wp) {
    wp->SetText(temp);
    wp->RefreshDisplay();
  }
}

static void OnUTCData(DataField *Sender, DataField::DataAccessKind_t Mode){
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

static void OnPolarFileData(DataField *Sender, DataField::DataAccessKind_t Mode){
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


static void OnPolarTypeData(DataField *Sender, DataField::DataAccessKind_t Mode){
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

static void OnWaypointNewClicked(WindowControl * Sender){
  (void)Sender;

  Waypoint edit_waypoint = way_points.create(XCSoarInterface::Basic().Location);
  if (dlgWaypointEditShowModal(edit_waypoint)) {
    if (edit_waypoint.Name.size()) {
      way_points.append(edit_waypoint);
      waypointneedsave = true;
    }
  }
}


static void OnWaypointEditClicked(WindowControl * Sender){
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
      MessageBoxX (
                   gettext(TEXT("Waypoint not editable")),
                   TEXT("Error"), MB_OK);
    }
  }
}

static void AskWaypointSave(void) {
  /// @todo terrain check???
  if (WayPointFile::WaypointsOutOfRangeSetting != 2 ||
      MessageBoxX(gettext(_T("Waypoints excluded, save anyway?")),
                   gettext(_T("Waypoints outside terrain")),
                   MB_YESNO | MB_ICONQUESTION) == IDYES) {
    WayPointGlue::SaveWaypoints(way_points);
    WaypointFileChanged= true;
    changed = true;
  }

  waypointneedsave = false;
}


static void OnWaypointSaveClicked(WindowControl * Sender){
  (void)Sender;

  AskWaypointSave();
}


static void OnWaypointDeleteClicked(WindowControl * Sender){
  (void)Sender;
#ifdef OLD_TASK
  int res;
  res = dlgWayPointSelect(XCSoarInterface::Basic().Location);
  if (res != -1){
    if(MessageBoxX(way_points.get(res).Name,
                   gettext(_T("Delete Waypoint?")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      way_points.set(res).FileNum = -1;
      waypointneedsave = true;
    }
  }
#endif
}


static CallBackTableEntry_t CallBackTable[]={
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
  DeclareCallBackEntry(OnEditStatisticsFontClicked),

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


static void SetInfoBoxSelector(unsigned item, int mode)
{
  WndProperty *wp = FindInfoBoxField(mode, item);
  if (wp == NULL)
    return;

  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  for (unsigned i = 0; i < NUMSELECTSTRINGS; i++) {
    dfe->addEnumText(gettext(InfoBoxManager::GetTypeDescription(i)));
  }
  dfe->Sort(0);

  dfe->Set(InfoBoxManager::getType(item, page_mode_to_ibm_mode(mode)));
  wp->RefreshDisplay();
}

static void GetInfoBoxSelector(unsigned item, int mode)
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
static  int dwDeviceIndex1=0;
static  int dwDeviceIndex2=0;
static int Speed = 1; // default is knots
static int TaskSpeed = 2; // default is kph
static int Distance = 2; // default is km
static int Lift = 0;
static int Altitude = 0; //default ft
static  TCHAR temptext[MAX_PATH];

static void
SetupDeviceFields(const DeviceDescriptor &device, const DeviceConfig &config,
                  int &driver_index,
                  WndProperty *port_field, WndProperty *speed_field,
                  WndProperty *driver_field, WndButton *setup_button)
{
  static const TCHAR *const COMMPort[] = {
    _T("COM1"), _T("COM2"), _T("COM3"), _T("COM4"),
    _T("COM5"), _T("COM6"), _T("COM7"), _T("COM8"),
    _T("COM9"), _T("COM10"), _T("COM0")
  };

  static const TCHAR *const tSpeed[] = {
    _T("1200"), _T("2400"), _T("4800"), _T("9600"),
    _T("19200"), _T("38400"), _T("57600"), _T("115200")
  };

  if (port_field != NULL) {
    DataFieldEnum *dfe = (DataFieldEnum *)port_field->GetDataField();

    for (unsigned i = 0; port_types[i].label != NULL; i++) {
      dfe->addEnumText(gettext(port_types[i].label));

      if (port_types[i].type == config.port_type)
        dfe->Set(i);
    }

    for (unsigned i = 0; i < 11; i++)
      dfe->addEnumText(COMMPort[i]);

    switch (config.port_type) {
    case DeviceConfig::SERIAL:
      dfe->Set(config.port_index + num_port_types);
      break;
    }

    port_field->RefreshDisplay();
  }

  if (speed_field != NULL) {
    DataFieldEnum *dfe = (DataFieldEnum *)speed_field->GetDataField();
    for (unsigned i = 0; i < 8; i++)
      dfe->addEnumText(tSpeed[i]);

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

static void setVariables(void) {
  WndProperty *wp;

  buttonPilotName = ((WndButton *)wf->FindByName(_T("cmdPilotName")));
  if (buttonPilotName) {
    buttonPilotName->SetOnClickNotify(OnPilotNameClicked);
  }
  buttonAircraftType = ((WndButton *)wf->FindByName(_T("cmdAircraftType")));
  if (buttonAircraftType) {
    buttonAircraftType->SetOnClickNotify(OnAircraftTypeClicked);
  }
  buttonAircraftRego = ((WndButton *)wf->FindByName(_T("cmdAircraftRego")));
  if (buttonAircraftRego) {
    buttonAircraftRego->SetOnClickNotify(OnAircraftRegoClicked);
  }
  buttonLoggerID = ((WndButton *)wf->FindByName(_T("cmdLoggerID")));
  if (buttonLoggerID) {
    buttonLoggerID->SetOnClickNotify(OnLoggerIDClicked);
  }

  buttonCopy = ((WndButton *)wf->FindByName(_T("cmdCopy")));
  if (buttonCopy) {
    buttonCopy->SetOnClickNotify(OnCopy);
  }
  buttonPaste = ((WndButton *)wf->FindByName(_T("cmdPaste")));
  if (buttonPaste) {
    buttonPaste->SetOnClickNotify(OnPaste);
  }

  UpdateButtons();

  //  DWORD dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};

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

  wp = (WndProperty*)wf->FindByName(_T("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("All on")));
    dfe->addEnumText(gettext(_T("Clip")));
    dfe->addEnumText(gettext(_T("Auto")));
    dfe->addEnumText(gettext(_T("All below")));
    dfe->Set(XCSoarInterface::SetSettingsComputer().AltitudeMode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpUTCOffset"),
                   iround(XCSoarInterface::SettingsComputer().UTCOffset/1800.0)/2.0);

  SetLocalTime();

  wp = (WndProperty*)wf->FindByName(_T("prpClipAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(
        XCSoarInterface::SetSettingsComputer().ClipAltitude)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltWarningMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(
        XCSoarInterface::SetSettingsComputer().AltWarningMargin)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAutoZoom"),
                   XCSoarInterface::SettingsMap().AutoZoom);
  LoadFormProperty(*wf, _T("prpAirspaceOutline"),
                   XCSoarInterface::SettingsMap().bAirspaceBlackOutline);
  LoadFormProperty(*wf, _T("prpLockSettingsInFlight"),
                   XCSoarInterface::LockSettingsInFlight);
  LoadFormProperty(*wf, _T("prpLoggerShortName"),
                   XCSoarInterface::SettingsComputer().LoggerShortName);
  LoadFormProperty(*wf, _T("prpDebounceTimeout"),
                   XCSoarInterface::debounceTimeout);

  wp = (WndProperty*)wf->FindByName(_T("prpEnableFLARMMap"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("OFF")));
    dfe->addEnumText(gettext(_T("ON/Fixed")));
    dfe->addEnumText(gettext(_T("ON/Scaled")));
    dfe->Set(XCSoarInterface::SettingsMap().EnableFLARMMap);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpEnableFLARMGauge"),
                   XCSoarInterface::SettingsMap().EnableFLARMGauge);
  LoadFormProperty(*wf, _T("prpAirspaceWarnings"),
                   XCSoarInterface::SettingsComputer().EnableAirspaceWarnings);
  LoadFormProperty(*wf, _T("prpWarningTime"),
                   XCSoarInterface::SetSettingsComputer().WarningTime);
  LoadFormProperty(*wf, _T("prpAcknowledgementTime"),
                   XCSoarInterface::SetSettingsComputer().AcknowledgementTime);

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointLabels"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Names")));
    dfe->addEnumText(gettext(_T("Numbers")));
    dfe->addEnumText(gettext(_T("First 5")));
    dfe->addEnumText(gettext(_T("None")));
    dfe->addEnumText(gettext(_T("First 3")));
    dfe->addEnumText(gettext(_T("Names in task")));
    dfe->addEnumText(gettext(_T("First word")));
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
    dfe->addEnumText(gettext(_T("Track up")));
    dfe->addEnumText(gettext(_T("North up")));
    dfe->addEnumText(gettext(_T("North circling")));
    dfe->addEnumText(gettext(_T("Target circling")));
    dfe->addEnumText(gettext(_T("North/track")));
    dfe->Set(XCSoarInterface::SettingsMap().DisplayOrientation);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpMenuTimeout"),
                   XCSoarInterface::MenuTimeoutMax / 2);

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeArrival"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(
        XCSoarInterface::SettingsComputer().safety_height_arrival)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeTerrain"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(
        XCSoarInterface::SettingsComputer().safety_height_terrain)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFinalGlideTerrain"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("OFF")));
    dfe->addEnumText(gettext(_T("Line")));
    dfe->addEnumText(gettext(_T("Shade")));
    dfe->Set(XCSoarInterface::SettingsComputer().FinalGlideTerrain);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpEnableNavBaroAltitude"),
                   XCSoarInterface::SettingsComputer().EnableNavBaroAltitude);

  wp = (WndProperty*)wf->FindByName(_T("prpWindArrowStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Arrow head")));
    dfe->addEnumText(gettext(_T("Full arrow")));
    wp->GetDataField()->Set(XCSoarInterface::SettingsMap().WindArrowStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Manual")));
    dfe->addEnumText(gettext(_T("Circling")));
    dfe->addEnumText(gettext(_T("ZigZag")));
    dfe->addEnumText(gettext(_T("Both")));
    wp->GetDataField()->Set(XCSoarInterface::SettingsComputer().AutoWindMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoMcMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Final glide")));
    dfe->addEnumText(gettext(_T("Average climb")));
    dfe->addEnumText(gettext(_T("Both")));
    wp->GetDataField()->Set((int)XCSoarInterface::SettingsComputer().auto_mc_mode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointsOutOfRange"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Ask")));
    dfe->addEnumText(gettext(_T("Include")));
    dfe->addEnumText(gettext(_T("Exclude")));
    wp->GetDataField()->Set(WayPointFile::WaypointsOutOfRangeSetting);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpBlockSTF"),
                   XCSoarInterface::SettingsComputer().EnableBlockSTF);
  LoadFormProperty(*wf, _T("prpFAIFinishHeight"),
                   XCSoarInterface::SettingsComputer().ordered_defaults.fai_finish);

  wp = (WndProperty*)wf->FindByName(_T("prpOLCRules"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(rule_as_text((OLCRules)0).c_str()));
    dfe->addEnumText(gettext(rule_as_text((OLCRules)1).c_str()));
    dfe->addEnumText(gettext(rule_as_text((OLCRules)2).c_str()));
    dfe->Set(XCSoarInterface::SettingsComputer().olc_rules);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpHandicap"),
                   XCSoarInterface::SettingsComputer().olc_handicap);

  if(!Profile::Get(szProfileSpeedUnitsValue,Speed)) {
    Profile::Set(szProfileSpeedUnitsValue, Speed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Statue")));
    dfe->addEnumText(gettext(_T("Nautical")));
    dfe->addEnumText(gettext(_T("Metric")));
    dfe->Set(Speed);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("DDMMSS"));
    dfe->addEnumText(_T("DDMMSS.ss"));
    dfe->addEnumText(_T("DDMM.mmm"));
    dfe->addEnumText(_T("DD.dddd"));
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
    dfe->addEnumText(gettext(_T("Statute")));
    dfe->addEnumText(gettext(_T("Nautical")));
    dfe->addEnumText(gettext(_T("Metric")));
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
    dfe->addEnumText(gettext(_T("Statute")));
    dfe->addEnumText(gettext(_T("Nautical")));
    dfe->addEnumText(gettext(_T("Metric")));
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
    dfe->addEnumText(gettext(_T("Feet")));
    dfe->addEnumText(gettext(_T("Meters")));
    dfe->Set(Altitude);
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
    dfe->addEnumText(gettext(_T("Knots")));
    dfe->addEnumText(gettext(_T("M/s")));
    dfe->Set(Lift);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpTrailDrift"),
                   XCSoarInterface::SettingsMap().EnableTrailDrift);

  LoadFormProperty(*wf, _T("prpSetSystemTimeFromGPS"),
                   XCSoarInterface::SettingsMap().SetSystemTimeFromGPS);
  LoadFormProperty(*wf, _T("prpAbortSafetyUseCurrent"),
                   XCSoarInterface::SettingsComputer().safety_mc_use_current);
  LoadFormProperty(*wf, _T("prpDisableAutoLogger"),
                   !XCSoarInterface::SettingsComputer().DisableAutoLogger);

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    wp->GetDataField()->Set(Units::ToUserVSpeed(XCSoarInterface::SettingsComputer().safety_mc));
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpRiskGamma"), XCSoarInterface::SettingsComputer().risk_gamma);

  wp = (WndProperty*)wf->FindByName(_T("prpTrail"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Off")));
    dfe->addEnumText(gettext(_T("Long")));
    dfe->addEnumText(gettext(_T("Short")));
    dfe->addEnumText(gettext(_T("Full")));
    dfe->Set(XCSoarInterface::SettingsMap().TrailActive);
    wp->RefreshDisplay();
  }

  // VENTA3 VisualGlide
  wp = (WndProperty*)wf->FindByName(_T("prpVGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Off")));
    dfe->addEnumText(gettext(_T("Steady")));
    dfe->addEnumText(gettext(_T("Moving")));
    dfe->Set(XCSoarInterface::SettingsMap().VisualGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMaxManoeuveringSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserSpeed(XCSoarInterface::SettingsComputer().SafetySpeed)));
    wp->GetDataField()->SetUnits(Units::GetSpeedName());
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpBallastSecsToEmpty"),
                   XCSoarInterface::SettingsComputer().BallastSecsToEmpty);

  wp = (WndProperty*)wf->FindByName(_T("prpPolarType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (unsigned i = 0; i < NUMPOLARS; i++) {
      dfe->addEnumText(PolarLabels[i]);
    }
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
    dfe->Set(XCSoarInterface::SettingsComputer().POLARID);
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
    dfe->ScanDirectoryTop(_T("*.dat"));
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
    dfe->addEnumText(gettext(_T("Center")));
    dfe->addEnumText(gettext(_T("Topleft")));
    dfe->Set(Appearance.StateMessageAlign);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTextInput"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Default")));
    dfe->addEnumText(gettext(_T("Keyboard")));
    dfe->addEnumText(gettext(_T("HighScore Style")));
    dfe->Set(Appearance.TextInputStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDialogStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Full width")));
    dfe->addEnumText(gettext(_T("Scaled")));
    dfe->addEnumText(gettext(_T("Scaled centered")));
    dfe->addEnumText(gettext(_T("Fixed")));
    dfe->Set(g_eDialogStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Box")));
    dfe->addEnumText(gettext(_T("Tab")));
    dfe->Set(Appearance.InfoBoxBorder);
    wp->RefreshDisplay();
  }

  if (is_pna() || is_fivv()) {
    wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
    if (wp) {
      DataFieldEnum* dfe;
      dfe = (DataFieldEnum*)wp->GetDataField();

      dfe->addEnumText(gettext(_T("8 Top + Bottom (Portrait)"))); // 0
      dfe->addEnumText(gettext(_T("8 Top (Portrait)")));  // 1
      dfe->addEnumText(gettext(_T("8 Bottom (Portrait)")));  // 2
      dfe->addEnumText(gettext(_T("8 Left + Right (Landscape)")));  // 3
      dfe->addEnumText(gettext(_T("8 Left (Landscape)")));  // 4
      dfe->addEnumText(gettext(_T("8 Right (Landscape)"))); // 5
      dfe->addEnumText(gettext(_T("9 Right + Vario (Landscape)")));  // 6
      dfe->addEnumText(gettext(_T("5 Right (Square)"))); // 7
      dfe->Set(InfoBoxLayout::InfoBoxGeometry);
      wp->RefreshDisplay();
    }
  }

#ifdef PNA
// VENTA-ADDON Model change config menu 11
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Generic")));
    dfe->addEnumText(gettext(_T("HP31x")));
    dfe->addEnumText(gettext(_T("MedionP5")));
    dfe->addEnumText(gettext(_T("MIO")));
    dfe->addEnumText(gettext(_T("Nokia500"))); // VENTA3
    dfe->addEnumText(gettext(_T("PN6000")));

    int iTmp;
    switch (GlobalModelType) {
    case MODELTYPE_PNA_PNA:
      iTmp=(InfoBoxModelAppearance_t)apImPnaGeneric;
      break;
    case MODELTYPE_PNA_HP31X:
      iTmp=(InfoBoxModelAppearance_t)apImPnaHp31x;
      break;
    case MODELTYPE_PNA_PN6000:
      iTmp=(InfoBoxModelAppearance_t)apImPnaPn6000;
      break;
    case MODELTYPE_PNA_MIO:
      iTmp=(InfoBoxModelAppearance_t)apImPnaMio;
      break;
    case MODELTYPE_PNA_NOKIA_500:
      iTmp=(InfoBoxModelAppearance_t)apImPnaNokia500;
      break;
    case MODELTYPE_PNA_MEDION_P5:
      iTmp=(InfoBoxModelAppearance_t)apImPnaMedionP5;
      break;
    default:
      iTmp=(InfoBoxModelAppearance_t)apImPnaGeneric;
      break;
    }

    dfe->Set(iTmp);
    wp->RefreshDisplay();
  }
#elif defined FIVV
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
// VENTA2- PC model
    dfe->addEnumText(gettext(is_embedded()
                             ? _T("PDA/normal") : _T("PC/normal")));
    wp->set_visible(is_fivv());
        dfe->Set(0);
    wp->RefreshDisplay();
  }
#endif

  wp = (WndProperty*)wf->FindByName(_T("prpExtendedVisualGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Normal")));
    dfe->addEnumText(gettext(_T("Extended")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(XCSoarInterface::SettingsMap().ExtendedVisualGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpGestures"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Disabled")));
    dfe->addEnumText(gettext(_T("Enabled")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(XCSoarInterface::SettingsComputer().EnableGestures);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("15 seconds")));
    dfe->addEnumText(gettext(_T("30 seconds")));
    dfe->addEnumText(gettext(_T("60 seconds")));
    dfe->addEnumText(gettext(_T("90 seconds")));
    dfe->addEnumText(gettext(_T("2 minutes")));
    dfe->addEnumText(gettext(_T("3 minutes")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(XCSoarInterface::SettingsComputer().AverEffTime);
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
    dfe->addEnumText(gettext(_T("Normal")));
    dfe->addEnumText(gettext(_T("White outline")));
    dfe->Set(Appearance.CompassAppearance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndFinalGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Default")));
    dfe->addEnumText(gettext(_T("Alternate")));
    dfe->Set(Appearance.IndFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Winpilot")));
    dfe->addEnumText(gettext(_T("Alternate")));
    dfe->addEnumText(gettext(_T("Green/Orange")));
    dfe->Set(Appearance.IndLandable);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEnableExternalTriggerCruise"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("OFF")));
    dfe->addEnumText(gettext(_T("Flap")));
    dfe->addEnumText(gettext(_T("SC")));
    dfe->Set(XCSoarInterface::SettingsComputer().EnableExternalTriggerCruise);
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
    dfe->addEnumText(gettext(_T("Low lands")));
    dfe->addEnumText(gettext(_T("Mountainous")));
    dfe->addEnumText(gettext(_T("Imhof 7")));
    dfe->addEnumText(gettext(_T("Imhof 4")));
    dfe->addEnumText(gettext(_T("Imhof 12")));
    dfe->addEnumText(gettext(_T("Imhof Atlas")));
    dfe->addEnumText(gettext(_T("ICAO")));
    dfe->addEnumText(gettext(_T("Grey")));
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

  wp = (WndProperty*)wf->FindByName(_T("prpFinishMinHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(XCSoarInterface::SettingsComputer().ordered_defaults.finish_min_height)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(XCSoarInterface::SettingsComputer().ordered_defaults.start_max_height)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeightMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserAltitude(XCSoarInterface::SettingsComputer().start_max_height_margin)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("AGL")));
    dfe->addEnumText(gettext(_T("MSL")));
    dfe->Set(XCSoarInterface::SettingsComputer().ordered_defaults.start_max_height_ref);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserSpeed(XCSoarInterface::SettingsComputer().ordered_defaults.start_max_speed)));
    wp->GetDataField()->SetUnits(Units::GetSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeedMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Units::ToUserSpeed(XCSoarInterface::SettingsComputer().start_max_speed_margin)));
    wp->GetDataField()->SetUnits(Units::GetSpeedName());
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpLoggerTimeStepCruise"),
                   XCSoarInterface::SettingsComputer().LoggerTimeStepCruise);
  LoadFormProperty(*wf, _T("prpLoggerTimeStepCircling"),
                   XCSoarInterface::SettingsComputer().LoggerTimeStepCircling);
  LoadFormProperty(*wf, _T("prpSnailWidthScale"),
                   XCSoarInterface::SettingsMap().SnailWidthScale);

#ifdef FIVV
  wp = (WndProperty*)wf->FindByName(_T("prpGPSAltitudeOffset")); // VENTA3 GPSAltitudeOffset
  if (wp) {
    wp->GetDataField()->SetAsFloat(GPSAltitudeOffset);
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }
#endif

  for (unsigned i = 0; i < 4; i++) {
    for (unsigned j=0; j<numInfoWindows; j++) {
      SetInfoBoxSelector(j, i);
    }
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
                        _T("dlgConfiguration_L.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_CONFIGURATION_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgConfiguration.xml"),
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

  if (!is_pna() && !is_fivv()) {
    // JMW we don't want these for non-PDA platforms yet
    wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
    if (wp) {
      wp->hide();
    }
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

  changed |= SetValueRegistryOnChange(wf, _T("prpAbortSafetyUseCurrent"),
                                      szProfileAbortSafetyUseCurrent,
                                      XCSoarInterface::SetSettingsComputer().safety_mc_use_current);

  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpDisableAutoLogger"));
  if (wp) { // GUI label is "Enable Auto Logger"
    if (!XCSoarInterface::SetSettingsComputer().DisableAutoLogger
        != wp->GetDataField()->GetAsBoolean()) {
      XCSoarInterface::SetSettingsComputer().DisableAutoLogger =
        !(wp->GetDataField()->GetAsBoolean());
      Profile::Set(szProfileDisableAutoLogger,
                    XCSoarInterface::SetSettingsComputer().DisableAutoLogger);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    fixed val = Units::ToSysVSpeed(wp->GetDataField()->GetAsFixed());
    if (XCSoarInterface::SettingsComputer().safety_mc != val) {
      XCSoarInterface::SetSettingsComputer().safety_mc = val;
      Profile::Set(szProfileSafetyMacCready,
                    iround(XCSoarInterface::SettingsComputer().safety_mc*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRiskGamma"));
  if (wp) {
    fixed val = wp->GetDataField()->GetAsFixed();
    if (XCSoarInterface::SettingsComputer().risk_gamma != val) {
      XCSoarInterface::SetSettingsComputer().risk_gamma = val;
      Profile::Set(szProfileRiskGamma,
                    iround(XCSoarInterface::SettingsComputer().risk_gamma*10));
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpSetSystemTimeFromGPS"),
                                      szProfileSetSystemTimeFromGPS,
                                      XCSoarInterface::SetSettingsMap().SetSystemTimeFromGPS);
  changed |= SetValueRegistryOnChange(wf, _T("prpTrailDrift"),
                                      szProfileTrailDrift,
                                      XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  changed |= SetValueRegistryOnChange(wf, _T("prpTrail"),
                                      szProfileSnailTrail,
                                      XCSoarInterface::SetSettingsMap().TrailActive);

// VENTA3: do not save VisualGlide to registry or profile

  wp = (WndProperty*)wf->FindByName(_T("prpPolarType"));
  if (wp) {
    if (XCSoarInterface::SettingsComputer().POLARID != (unsigned)wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsComputer().POLARID = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfilePolarID, (int &)XCSoarInterface::SettingsComputer().POLARID);
      PolarFileChanged = true;
      changed = true;
    }
  }

  short tmp = XCSoarInterface::SetSettingsComputer().AltitudeMode;
  changed |= SetValueRegistryOnChange(wf, _T("prpAirspaceDisplay"),
                                      szProfileAltMode, tmp);
  XCSoarInterface::SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)tmp;

  changed |= SetValueRegistryOnChange(wf, _T("prpLockSettingsInFlight"),
                                      szProfileLockSettingsInFlight,
                                      XCSoarInterface::LockSettingsInFlight);

  changed |= SetValueRegistryOnChange(wf, _T("prpLoggerShortName"),
                                      szProfileLoggerShort,
                                      XCSoarInterface::SetSettingsComputer().LoggerShortName);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableFLARMMap"),
                                      szProfileEnableFLARMMap,
                                      XCSoarInterface::SetSettingsMap().EnableFLARMMap);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableFLARMGauge"),
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

  changed |= SetValueRegistryOnChange(wf, _T("prpAirspaceOutline"),
                                      szProfileAirspaceBlackOutline,
                                      XCSoarInterface::SetSettingsMap().bAirspaceBlackOutline);

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoZoom"),
                                      szProfileAutoZoom,
                                      XCSoarInterface::SetSettingsMap().AutoZoom);

  int ival;

  wp = (WndProperty*)wf->FindByName(_T("prpUTCOffset"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()*3600.0);
    if ((XCSoarInterface::SettingsComputer().UTCOffset != ival)||(utcchanged)) {
      XCSoarInterface::SetSettingsComputer().UTCOffset = ival;

      // have to do this because registry variables can't be negative!
      int lival = XCSoarInterface::SettingsComputer().UTCOffset;
      if (lival<0) { lival+= 24*3600; }
      Profile::Set(szProfileUTCOffset, lival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpClipAltitude"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SetSettingsComputer().ClipAltitude != ival) {
      XCSoarInterface::SetSettingsComputer().ClipAltitude = ival;
      Profile::Set(szProfileClipAlt,XCSoarInterface::SetSettingsComputer().ClipAltitude);  // fixed 20060430/sgi was szProfileAltMode
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltWarningMargin"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SetSettingsComputer().AltWarningMargin != ival) {
      XCSoarInterface::SetSettingsComputer().AltWarningMargin = ival;
      Profile::Set(szProfileAltMargin,XCSoarInterface::SetSettingsComputer().AltWarningMargin);
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAirspaceWarnings"),
                                      szProfileAirspaceWarning,
                                      XCSoarInterface::SetSettingsComputer().EnableAirspaceWarnings);

  changed |= SetValueRegistryOnChange(wf, _T("prpWarningTime"),
                                      szProfileWarningTime,
                                      XCSoarInterface::SetSettingsComputer().WarningTime);

  changed |= SetValueRegistryOnChange(wf, _T("prpAcknowledgementTime"),
                                      szProfileAcknowledgementTime,
                                      XCSoarInterface::SetSettingsComputer().AcknowledgementTime);

  changed |= SetValueRegistryOnChange(wf, _T("prpWaypointLabels"),
                                      szProfileDisplayText,
                                      XCSoarInterface::SetSettingsMap().DisplayTextType);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableTerrain"),
                                      szProfileDrawTerrain,
                                      XCSoarInterface::SetSettingsMap().EnableTerrain);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableTopology"),
                                      szProfileDrawTopology,
                                      XCSoarInterface::SetSettingsMap().EnableTopology);

  changed |= SetValueRegistryOnChange(wf, _T("prpCirclingZoom"),
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

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeArrival"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SettingsComputer().safety_height_arrival != ival) {
      XCSoarInterface::SetSettingsComputer().safety_height_arrival = ival;
      Profile::Set(szProfileSafetyAltitudeArrival,
                   (int)XCSoarInterface::SettingsComputer().safety_height_arrival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeTerrain"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SettingsComputer().safety_height_terrain != ival) {
      XCSoarInterface::SetSettingsComputer().safety_height_terrain = ival;
      Profile::Set(szProfileSafetyAltitudeTerrain,
                   (int)XCSoarInterface::SettingsComputer().safety_height_terrain);
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoWind"),
                                      szProfileAutoWind,
                                      XCSoarInterface::SetSettingsComputer().AutoWindMode);

  changed |= SetValueRegistryOnChange(wf, _T("prpWindArrowStyle"),
                                      szProfileWindArrowStyle,
                                      XCSoarInterface::SetSettingsMap().WindArrowStyle);

  int auto_mc_mode = (int)XCSoarInterface::SettingsComputer().auto_mc_mode;
  changed |= SetValueRegistryOnChange(wf, _T("prpAutoMcMode"),
                                      szProfileAutoMcMode,
                                      auto_mc_mode);
  XCSoarInterface::SetSettingsComputer().auto_mc_mode = 
    (TaskBehaviour::AutoMCMode_t)auto_mc_mode;

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

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableNavBaroAltitude"),
                                      szProfileEnableNavBaroAltitude,
                                      XCSoarInterface::SetSettingsComputer().EnableNavBaroAltitude);

  changed |= SetValueRegistryOnChange(wf, _T("prpFinalGlideTerrain"),
                                      szProfileFinalGlideTerrain,
                                      XCSoarInterface::SetSettingsComputer().FinalGlideTerrain);

  changed |= SetValueRegistryOnChange(wf, _T("prpBlockSTF"),
                                      szProfileBlockSTF,
                                      XCSoarInterface::SetSettingsComputer().EnableBlockSTF);

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

  {
    unsigned t= XCSoarInterface::SettingsComputer().olc_rules;
    changed |= SetValueRegistryOnChange(wf, _T("prpOLCRules"),
                                        szProfileOLCRules,
                                        t);
    XCSoarInterface::SetSettingsComputer().olc_rules = (OLCRules)t;
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpHandicap"),
                                      szProfileHandicap,
                                      XCSoarInterface::SetSettingsComputer().olc_handicap);

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

  changed |= SetValueRegistryOnChange(wf, _T("prpBallastSecsToEmpty"),
                                      szProfileBallastSecsToEmpty,
                                      XCSoarInterface::SetSettingsComputer().BallastSecsToEmpty);

  wp = (WndProperty*)wf->FindByName(_T("prpMaxManoeuveringSpeed"));
  if (wp) {
    ival = iround(Units::ToSysSpeed(wp->GetDataField()->GetAsInteger()));
    if (XCSoarInterface::SettingsComputer().SafetySpeed != ival) {
      XCSoarInterface::SetSettingsComputer().SafetySpeed = ival;
      Profile::Set(szProfileSafteySpeed,
                    XCSoarInterface::SettingsComputer().SafetySpeed);
      changed = true;
    }
  }

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
    if (XCSoarInterface::SettingsMap().ExtendedVisualGlide != (ExtendedVisualGlide_t)
        (wp->GetDataField()->GetAsInteger())) {
      XCSoarInterface::SetSettingsMap().ExtendedVisualGlide = (ExtendedVisualGlide_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileExtendedVisualGlide,
                    XCSoarInterface::SettingsMap().ExtendedVisualGlide);
      changed = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpGestures")); // VENTA6
  if (wp) {
    if (XCSoarInterface::SettingsComputer().EnableGestures
        != wp->GetDataField()->GetAsBoolean()) {
      XCSoarInterface::SetSettingsComputer().EnableGestures =
          !XCSoarInterface::SettingsComputer().EnableGestures;
      Profile::Set(szProfileGestures,
                   XCSoarInterface::SettingsComputer().EnableGestures);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime")); // VENTA6
  if (wp) {
    if (XCSoarInterface::SettingsComputer().AverEffTime !=
        (wp->GetDataField()->GetAsInteger())) {
      XCSoarInterface::SetSettingsComputer().AverEffTime =
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAverEffTime,
                    XCSoarInterface::SettingsComputer().AverEffTime);
      changed = true;
      requirerestart = true;
    }
  }


#if defined(PNA) || defined(FIVV)
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    if (InfoBoxLayout::InfoBoxGeometry != (unsigned)wp->GetDataField()->GetAsInteger()) {
      InfoBoxLayout::InfoBoxGeometry = (unsigned)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileInfoBoxGeometry, InfoBoxLayout::InfoBoxGeometry);
      changed = true;
      requirerestart = true;
    }
  }
#endif

#if defined(PNA)
  // VENTA-ADDON MODEL CHANGE
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxModel"));
  if (wp) {
    if (Appearance.InfoBoxModel != (InfoBoxModelAppearance_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.InfoBoxModel = (InfoBoxModelAppearance_t)
        (wp->GetDataField()->GetAsInteger());

      switch (Appearance.InfoBoxModel) {
      case apImPnaGeneric:
        GlobalModelType = MODELTYPE_PNA_PNA;
        break;
      case apImPnaHp31x:
        GlobalModelType = MODELTYPE_PNA_HP31X;
        break;
      case apImPnaPn6000:
        GlobalModelType = MODELTYPE_PNA_PN6000;
        break;
      case apImPnaMio:
        GlobalModelType = MODELTYPE_PNA_MIO;
        break;
      case apImPnaNokia500:
        GlobalModelType = MODELTYPE_PNA_NOKIA_500;
        break;
      case apImPnaMedionP5:
        GlobalModelType = MODELTYPE_PNA_MEDION_P5;
        break;
      default:
        GlobalModelType = MODELTYPE_UNKNOWN; // Can't happen, troubles ..
        break;

      }
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
  TempStatisticsFont.reset();


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

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableExternalTriggerCruise"),
                                      szProfileEnableExternalTriggerCruise,
                                      XCSoarInterface::SetSettingsComputer().EnableExternalTriggerCruise);

  wp = (WndProperty*)wf->FindByName(_T("prpAppInverseInfoBox"));
  if (wp) {
    if ((int)(Appearance.InverseInfoBox) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InverseInfoBox = (wp->GetDataField()->GetAsInteger() != 0);
      Profile::Set(szProfileAppInverseInfoBox,Appearance.InverseInfoBox);
      requirerestart = true;
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpGliderScreenPosition"),
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

  changed |= SetValueRegistryOnChange(wf, _T("prpAppGaugeVarioMc"),
                                      szProfileAppGaugeVarioMc,
                                      Appearance.GaugeVarioMc);

  changed |= SetValueRegistryOnChange(wf, _T("prpAppGaugeVarioBugs"),
                                      szProfileAppGaugeVarioBugs,
                                      Appearance.GaugeVarioBugs);

  changed |= SetValueRegistryOnChange(wf, _T("prpAppGaugeVarioBallast"),
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

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoBacklight"), // VENTA4
                                      szProfileAutoBacklight,
                                      CommonInterface::EnableAutoBacklight);

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoSoundVolume"), // VENTA4
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

  changed |= SetValueRegistryOnChange(wf,
                                      _T("prpTerrainRamp"),
                                      szProfileTerrainRamp,
                                      XCSoarInterface::SetSettingsMap().TerrainRamp);

  wp = (WndProperty*)wf->FindByName(_T("prpFinishMinHeight"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SettingsComputer().ordered_defaults.finish_min_height != ival) {
      XCSoarInterface::SetSettingsComputer().ordered_defaults.finish_min_height = ival;
      Profile::Set(szProfileFinishMinHeight,ival);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeight"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SettingsComputer().ordered_defaults.start_max_height != ival) {
      XCSoarInterface::SetSettingsComputer().ordered_defaults.start_max_height = ival;
      Profile::Set(szProfileStartMaxHeight, 
                    XCSoarInterface::SettingsComputer().ordered_defaults.start_max_height);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeightMargin"));
  if (wp) {
    ival = iround(Units::ToSysAltitude(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SettingsComputer().start_max_height_margin != ival) {
      XCSoarInterface::SetSettingsComputer().start_max_height_margin = ival;
      Profile::Set(szProfileStartMaxHeightMargin,
                    XCSoarInterface::SettingsComputer().start_max_height_margin);
      changed = true;
      taskchanged = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    if (XCSoarInterface::SettingsComputer().ordered_defaults.start_max_height_ref 
        != (unsigned)wp->GetDataField()->GetAsInteger()) {

      XCSoarInterface::SetSettingsComputer().ordered_defaults.start_max_height_ref = 
        wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileStartHeightRef, 
                    XCSoarInterface::SettingsComputer().ordered_defaults.start_max_height_ref);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeed"));
  if (wp) {
    ival = iround(Units::ToSysSpeed(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SetSettingsComputer().ordered_defaults.start_max_speed != ival) {
      XCSoarInterface::SetSettingsComputer().ordered_defaults.start_max_speed = ival;
      Profile::Set(szProfileStartMaxSpeed,
                    (int)XCSoarInterface::SettingsComputer().ordered_defaults.start_max_speed);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeedMargin"));
  if (wp) {
    ival = iround(Units::ToSysSpeed(wp->GetDataField()->GetAsInteger()));
    if ((int)XCSoarInterface::SettingsComputer().start_max_speed_margin != ival) {
      XCSoarInterface::SetSettingsComputer().start_max_speed_margin = ival;
      Profile::Set(szProfileStartMaxSpeedMargin,
                    (int)XCSoarInterface::SettingsComputer().start_max_speed_margin);
      changed = true;
      taskchanged = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpLoggerTimeStepCruise"),
                                      szProfileLoggerTimeStepCruise,
                                      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCruise);

  changed |= SetValueRegistryOnChange(wf, _T("prpLoggerTimeStepCircling"),
                                      szProfileLoggerTimeStepCircling,
                                      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCircling);

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

  if (DevicePortChanged) {
    for (unsigned i = 0; i < NUMDEV; ++i) {
      Profile::SetDeviceConfig(i, device_config[i]);
    }
  }

  for (unsigned i = 0; i < 4; ++i) {
    for (unsigned j = 0; j < numInfoWindows; ++j) {
      GetInfoBoxSelector(j, i);
    }
  }

  if (waypointneedsave) {
    way_points.optimise();
    if(MessageBoxX(gettext(_T("Save changes to waypoint file?")),
                   gettext(_T("Waypoints edited")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      AskWaypointSave();

    }
  }

  if (taskchanged) {
    changed = true;
  }

  if (!is_embedded() && DevicePortChanged) {
    requirerestart = true;
  }

  if (changed) {
    Profile::Save();

    if (!requirerestart) {
      MessageBoxX(gettext(_T("Changes to configuration saved.")),
                  _T(""), MB_OK);
    } else {

      MessageBoxX(gettext(_T("Changes to configuration saved.  Restart XCSoar to apply changes.")),
                  _T(""), MB_OK);
    }
  }

  delete wf;

  wf = NULL;

}
