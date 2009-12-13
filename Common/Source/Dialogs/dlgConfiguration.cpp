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
#include "InfoBoxManager.h"
#include "Device/Parser.h"
#include "Blackboard.hpp"
#include "SettingsAirspace.hpp"
#include "SettingsComputer.hpp"
#include "Task.h"
#include "SettingsUser.hpp"
#include "Appearance.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "LocalPath.hpp"
#include "UtilsProfile.hpp"
#include "Logger.h"
#include "MacCready.h"
#include "Device/device.h"
#include "Screen/Animation.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Registry.hpp"
#include "Profile.hpp"
#include "LocalTime.hpp"
#include "Math/FastMath.h"
#include "WayPointParser.h"
#include "Polar/BuiltIn.hpp"
#include "Polar/Historical.hpp"
#include "Polar/Loader.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/FileReader.hpp"
#include "Asset.hpp"
#include "Screen/Fonts.hpp"
#include "GlideRatio.hpp"
#include "WayPointList.hpp"
#include "Components.hpp"

extern ldrotary_s rotaryLD;

#include <assert.h>

static SETTINGS_TASK settings_task;

static Font TempInfoWindowFont;
static Font TempTitleWindowFont;
static Font TempMapWindowFont;
static Font TempTitleSmallWindowFont;
static Font TempMapWindowBoldFont;
static Font TempCDIWindowFont; // New
static Font TempMapLabelFont;
static Font TempStatisticsFont;
static Font TempUseCustomFontsFont;

extern LOGFONT autoInfoWindowLogFont;
extern LOGFONT autoTitleWindowLogFont;
extern LOGFONT autoMapWindowLogFont;
extern LOGFONT autoTitleSmallWindowLogFont;
extern LOGFONT autoMapWindowBoldLogFont;
extern LOGFONT autoCDIWindowLogFont; // New
extern LOGFONT autoMapLabelLogFont;
extern LOGFONT autoStatisticsLogFont;

extern bool dlgFontEditShowModal(const TCHAR * FontDescription,
                          const TCHAR * FontRegKey,
                          LOGFONT autoLogFont);

static bool changed = false;
static bool taskchanged = false;
static bool requirerestart = false;
static bool utcchanged = false;
static bool waypointneedsave = false;
static bool FontRegistryChanged=false;
int config_page=0;
static WndForm *wf=NULL;
static WndFrame *wConfig1=NULL;
static WndFrame *wConfig2=NULL;
static WndFrame *wConfig3=NULL;
static WndFrame *wConfig4=NULL;
static WndFrame *wConfig5=NULL;
static WndFrame *wConfig6=NULL;
static WndFrame *wConfig7=NULL;
static WndFrame *wConfig8=NULL;
static WndFrame *wConfig9=NULL;
static WndFrame *wConfig10=NULL;
static WndFrame *wConfig11=NULL;
static WndFrame *wConfig12=NULL;
static WndFrame *wConfig13=NULL;
static WndFrame *wConfig14=NULL;
static WndFrame *wConfig15=NULL;
static WndFrame *wConfig16=NULL;
static WndFrame *wConfig17=NULL;
static WndFrame *wConfig18=NULL;
static WndFrame *wConfig19=NULL;
static WndFrame *wConfig20=NULL;
static WndFrame *wConfig21=NULL;
static WndFrame *wConfig22=NULL;
static WndButton *buttonPilotName=NULL;
static WndButton *buttonAircraftType=NULL;
static WndButton *buttonAircraftRego=NULL;
static WndButton *buttonLoggerID=NULL;
static WndButton *buttonCopy=NULL;
static WndButton *buttonPaste=NULL;

#define NUMPAGES 22



static void UpdateButtons(void) {
  TCHAR text[120];
  TCHAR val[100];
  if (buttonPilotName) {
    GetRegistryString(szRegistryPilotName, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(_T("(blank)")));
    }
    _stprintf(text,_T("%s: %s"), gettext(_T("Pilot name")), val);
    buttonPilotName->SetCaption(text);
  }
  if (buttonAircraftType) {
    GetRegistryString(szRegistryAircraftType, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(_T("(blank)")));
    }
    _stprintf(text,_T("%s: %s"), gettext(_T("Aircraft type")), val);
    buttonAircraftType->SetCaption(text);
  }
  if (buttonAircraftRego) {
    GetRegistryString(szRegistryAircraftRego, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(_T("(blank)")));
    }
    _stprintf(text,_T("%s: %s"), gettext(_T("Competition ID")), val);
    buttonAircraftRego->SetCaption(text);
  }
  if (buttonLoggerID) {
    GetRegistryString(szRegistryLoggerID, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(_T("(blank)")));
    }
    _stprintf(text,_T("%s: %s"), gettext(_T("Logger ID")), val);
    buttonLoggerID->SetCaption(text);
  }
}

static void NextPage(int Step){
  config_page += Step;
  if (config_page>=NUMPAGES) { config_page=0; }
  if (config_page<0) { config_page=NUMPAGES-1; }
  switch(config_page) {
  case 0:
    wf->SetCaption(gettext(_T("1 Site")));
    break;
  case 1:
    wf->SetCaption(gettext(_T("2 Airspace")));
    break;
  case 2:
    wf->SetCaption(gettext(_T("3 Map Display")));
    break;
  case 3:
    wf->SetCaption(gettext(_T("4 Terrain Display")));
    break;
  case 4:
    wf->SetCaption(gettext(_T("5 Glide Computer")));
    break;
  case 5:
    wf->SetCaption(gettext(_T("6 Safety factors")));
    break;
  case 6:
    wf->SetCaption(gettext(_T("7 Polar")));
    break;
  case 7:
    wf->SetCaption(gettext(_T("8 Devices")));
    break;
  case 8:
    wf->SetCaption(gettext(_T("9 Units")));
    break;
  case 9:
    wf->SetCaption(gettext(_T("10 Interface")));
    break;
  case 10:
    wf->SetCaption(gettext(_T("11 Appearance")));
    break;
  case 11:
    wf->SetCaption(gettext(_T("12 Fonts")));
    break;
  case 12:
    wf->SetCaption(gettext(_T("13 Vario Gauge and FLARM")));
    break;
  case 13:
    wf->SetCaption(gettext(_T("14 Task")));
    break;
  case 14:
    wf->SetCaption(gettext(_T("15 Task rules")));
    break;
  case 15:
    wf->SetCaption(gettext(_T("16 InfoBox Cruise")));
    break;
  case 16:
    wf->SetCaption(gettext(_T("17 InfoBox Circling")));
    break;
  case 17:
    wf->SetCaption(gettext(_T("18 InfoBox Final Glide")));
    break;
  case 18:
    wf->SetCaption(gettext(_T("19 InfoBox Auxiliary")));
    break;
  case 19:
    wf->SetCaption(gettext(_T("20 Logger")));
    break;
  case 20:
    wf->SetCaption(gettext(_T("21 Waypoint Edit")));
    break;
  case 21:
    wf->SetCaption(gettext(_T("22 Experimental features")));
    break;
  }
  if ((config_page>=15) && (config_page<=18)) {
    if (buttonCopy) {
      buttonCopy->show();
    }
    if (buttonPaste) {
      buttonPaste->show();
    }
  } else {
    if (buttonCopy) {
      buttonCopy->hide();
    }
    if (buttonPaste) {
      buttonPaste->hide();
    }
  }
  wConfig1->set_visible(config_page == 0);
  wConfig2->set_visible(config_page == 1);
  wConfig3->set_visible(config_page == 2);
  wConfig4->set_visible(config_page == 3);
  wConfig5->set_visible(config_page == 4);
  wConfig6->set_visible(config_page == 5);
  wConfig7->set_visible(config_page == 6);
  wConfig8->set_visible(config_page == 7);
  wConfig9->set_visible(config_page == 8);
  wConfig10->set_visible(config_page == 9);
  wConfig11->set_visible(config_page == 10);
  wConfig12->set_visible(config_page == 11);
  wConfig13->set_visible(config_page == 12);
  wConfig14->set_visible(config_page == 13);
  wConfig15->set_visible(config_page == 14);
  wConfig16->set_visible(config_page == 15);
  wConfig17->set_visible(config_page == 16);
  wConfig18->set_visible(config_page == 17);
  wConfig19->set_visible(config_page == 18);
  wConfig20->set_visible(config_page == 19);
  wConfig21->set_visible(config_page == 20);
  wConfig22->set_visible(config_page == 21);
}



static void OnSetupDeviceAClicked(WindowControl * Sender){
  (void)Sender;

  #ifdef ToDo
    devA()->DoSetup();
    wf->FocusNext(NULL);
  #endif

// this is a hack, devices dont jet support device dependant setup dialogs

    if (!is_simulator() && (devA() == NULL ||
                            _tcscmp(devA()->Name,_T("Vega")) != 0))
      return;

    changed = dlgConfigurationVarioShowModal();

    // this is a hack to get the dialog to retain focus because
    // the progress dialog in the vario configuration somehow causes
    // focus problems
    wf->FocusNext(NULL);
}


static void OnSetupDeviceBClicked(WindowControl * Sender){
  (void)Sender;

  #ifdef ToDo
    devB()->DoSetup();
    wf->FocusNext(NULL);
  #endif

// this is a hack, devices dont jet support device dependant setup dialogs

    if (!is_simulator() && (devB() == NULL ||
                            _tcscmp(devB()->Name,_T("Vega")) != 0))
      return;

    changed = dlgConfigurationVarioShowModal();

    // this is a hack to get the dialog to retain focus because
    // the progress dialog in the vario configuration somehow causes
    // focus problems
    wf->FocusNext(NULL);
}

static void
UpdateDeviceSetupButton(int DeviceIdx, const TCHAR *Name)
{
  WndButton *wb;

  if (DeviceIdx == 0){

    wb = ((WndButton *)wf->FindByName(_T("cmdSetupDeviceA")));
    if (wb != NULL) {
      wb->set_visible(_tcscmp(Name, _T("Vega")) == 0);
    }

  }

  if (DeviceIdx == 1){

    wb = ((WndButton *)wf->FindByName(_T("cmdSetupDeviceB")));
    if (wb != NULL) {
      wb->set_visible(_tcscmp(Name, _T("Vega")) == 0);
    }

  }

}


static void OnUserLevel(DataField *Sender, DataField::DataAccessKind_t Mode){
  WndProperty* wp;

  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    case DataField::daChange:
      wp = (WndProperty*)wf->FindByName(_T("prpUserLevel"));
      if (wp) {
        if (wp->GetDataField()->GetAsInteger() !=
	    (int)XCSoarInterface::UserLevel) {
          XCSoarInterface::UserLevel = wp->GetDataField()->GetAsInteger();
          changed = true;
          SetToRegistry(szRegistryUserLevel,(int)XCSoarInterface::UserLevel);
          wf->FilterAdvanced(XCSoarInterface::UserLevel>0);
        }
      }
    break;
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


static void ResetFonts(bool bUseCustom) {
// resest fonts when UseCustomFonts is turned off

  int UseCustomFontsold = UseCustomFonts;
  UseCustomFonts=bUseCustom;



  InitializeOneFont (&TempUseCustomFontsFont,
                        _T("THIS FONT IS NOT CUSTOMIZABLE"),
                        autoMapWindowLogFont,
                        NULL);



  InitializeOneFont (&TempInfoWindowFont,
                        szRegistryFontInfoWindowFont,
                        autoInfoWindowLogFont,
                        NULL);

  InitializeOneFont (&TempTitleWindowFont,
                        szRegistryFontTitleWindowFont,
                        autoTitleWindowLogFont,
                        NULL);

  InitializeOneFont (&TempMapWindowFont,
                        szRegistryFontMapWindowFont,
                        autoMapWindowLogFont,
                        NULL);

  InitializeOneFont (&TempTitleSmallWindowFont,
                        szRegistryFontTitleSmallWindowFont,
                        autoTitleSmallWindowLogFont,
                        NULL);

  InitializeOneFont (&TempMapWindowBoldFont,
                        szRegistryFontMapWindowBoldFont,
                        autoMapWindowBoldLogFont,
                        NULL);

  InitializeOneFont (&TempCDIWindowFont,
                        szRegistryFontCDIWindowFont,
                        autoCDIWindowLogFont,
                        NULL);

  InitializeOneFont (&TempMapLabelFont,
                        szRegistryFontMapLabelFont,
                        autoMapLabelLogFont,
                        NULL);

  InitializeOneFont (&TempStatisticsFont,
                        szRegistryFontStatisticsFont,
                        autoStatisticsLogFont,
                        NULL);

  UseCustomFonts=UseCustomFontsold;
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

  wp = (WndProperty*)wf->FindByName(_T("cmdNext"));
  if (wp) {
    wp->SetFont(TempCDIWindowFont);
  }
  wp = (WndProperty*)wf->FindByName(_T("cmdPrev"));
  if (wp) {
    wp->SetFont(TempCDIWindowFont);
  }

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

static void OnEditInfoWindowFontClicked(WindowControl *Sender) {
  // updates registry for font info and updates LogFont values
#define MAX_EDITFONT_DESC_LEN 100
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpInfoWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontInfoWindowFont,
                            autoInfoWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void OnEditTitleWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpTitleWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontTitleWindowFont,
                            autoTitleWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditMapWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontMapWindowFont,
                            autoMapWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditTitleSmallWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpTitleSmallWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontTitleSmallWindowFont,
                            autoTitleSmallWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditMapWindowBoldFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapWindowBoldFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontMapWindowBoldFont,
                            autoMapWindowBoldLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditCDIWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpCDIWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontCDIWindowFont,
                            autoCDIWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditMapLabelFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpMapLabelFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontMapLabelFont,
                            autoMapLabelLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditStatisticsFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, _T("prpStatisticsFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontStatisticsFont,
                            autoStatisticsLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}


static void OnAircraftRegoClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonAircraftRego) {
    GetRegistryString(szRegistryAircraftRego,Temp,100);
    if (dlgTextEntryShowModal(Temp,100)) {
      SetRegistryString(szRegistryAircraftRego,Temp);
      changed = true;
    }
  }
  UpdateButtons();
}


static void OnAircraftTypeClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonAircraftType) {
    GetRegistryString(szRegistryAircraftType,Temp,100);
    if (dlgTextEntryShowModal(Temp,100)){
      SetRegistryString(szRegistryAircraftType,Temp);
      changed = true;
    }
  }
  UpdateButtons();
}


static void OnPilotNameClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonPilotName) {
    GetRegistryString(szRegistryPilotName,Temp,100);
    if (dlgTextEntryShowModal(Temp,100)){
      SetRegistryString(szRegistryPilotName,Temp);
      changed = true;
    }
  }
  UpdateButtons();
}


static void OnLoggerIDClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonLoggerID) {
    GetRegistryString(szRegistryLoggerID,Temp,100);
    if(dlgTextEntryShowModal(Temp,100)){
      SetRegistryString(szRegistryLoggerID,Temp);
      changed = true;
    }
    ReadAssetNumber();
  }
  UpdateButtons();
}


static void OnAirspaceColoursClicked(WindowControl * Sender){
	(void)Sender;
  dlgAirspaceShowModal(true);
}

static void OnAirspaceModeClicked(WindowControl * Sender){
	(void)Sender;
  dlgAirspaceShowModal(false);
}

static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static int cpyInfoBox[10];

static int page2mode(void) {
  return config_page-15;  // RLD upped by 1
}


static void InfoBoxPropName(TCHAR *name, int item, int mode) {
  _tcscpy(name,_T("prpInfoBox"));
  switch (mode) {
  case 0:
    _tcscat(name,_T("Circling"));
    break;
  case 1:
    _tcscat(name,_T("Cruise"));
    break;
  case 2:
    _tcscat(name,_T("FinalGlide"));
    break;
  case 3:
    _tcscat(name,_T("Aux"));
    break;
  }
  TCHAR buf[3];
  _stprintf(buf,_T("%1d"), item);
  _tcscat(name,buf);
}

static void OnCopy(WindowControl *Sender) {
  (void)Sender;
  int mode = page2mode();
  TCHAR name[80];
  if ((mode<0)||(mode>3)) {
    return;
  }

  for (unsigned item = 0; item < numInfoWindows; item++) {
    InfoBoxPropName(name, item, mode);
    WndProperty *wp;
    wp = (WndProperty*)wf->FindByName(name);
    if (wp) {
      cpyInfoBox[item] = wp->GetDataField()->GetAsInteger();
    }
  }
}

static void OnPaste(WindowControl *Sender) {
  (void)Sender;
  int mode = page2mode();
  TCHAR name[80];
  if ((mode<0)||(mode>3)||(cpyInfoBox[0]<0)) {
    return;
  }

  if(MessageBoxX(gettext(_T("Overwrite?")),
		 gettext(_T("InfoBox paste")),
		 MB_YESNO|MB_ICONQUESTION) == IDYES) {

    for (unsigned item = 0; item < numInfoWindows; item++) {
      InfoBoxPropName(name, item, mode);
      WndProperty *wp;
      wp = (WndProperty*)wf->FindByName(name);
      if (wp && (cpyInfoBox[item] >=0 ) &&
          ((unsigned)cpyInfoBox[item] < NUMSELECTSTRINGS)) {
	wp->GetDataField()->Set(cpyInfoBox[item]);
	wp->RefreshDisplay();
      }
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
      NextPage(-1);
      //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

    case '7':
      ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
      NextPage(+1);
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

        if (wp != NULL && wp->GetDataField()->GetAsInteger() > 0){
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

  WAYPOINT edit_waypoint;
  edit_waypoint.Location = XCSoarInterface::Basic().Location;
  edit_waypoint.FileNum = 0; // default, put into primary waypoint file
  edit_waypoint.Flags = 0;
  edit_waypoint.Comment[0] = 0;
  edit_waypoint.Name[0] = 0;
  edit_waypoint.Details = 0;
  edit_waypoint.Number = 0;
  dlgWaypointEditShowModal(edit_waypoint);
  if (_tcslen(edit_waypoint.Name)>0) {
    int i = way_points.append(edit_waypoint);
    if (i >= 0) {
      way_points.set(i).Details = 0;
      waypointneedsave = true;
    }
  }
}


static void OnWaypointEditClicked(WindowControl * Sender){
	(void)Sender;
  int res;
  res = dlgWayPointSelect(XCSoarInterface::Basic().Location);
  if (res != -1){
    dlgWaypointEditShowModal(way_points.set(res));
    waypointneedsave = true;
  }
}

static void AskWaypointSave(void) {
  if (WaypointsOutOfRange==2) {

    if(MessageBoxX(gettext(_T("Waypoints excluded, save anyway?")),
                   gettext(_T("Waypoints outside terrain")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      WaypointWriteFiles(way_points, XCSoarInterface::SettingsComputer());

      WAYPOINTFILECHANGED= true;
      changed = true;

    }
  } else {

    WaypointWriteFiles(way_points, XCSoarInterface::SettingsComputer());

    WAYPOINTFILECHANGED= true;
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

  DeclareCallBackEntry(OnUserLevel),

  DeclareCallBackEntry(NULL)
};



static void SetInfoBoxSelector(unsigned item, int mode)
{
  TCHAR name[80];
  InfoBoxPropName(name, item, mode);

  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (unsigned i = 0; i < NUMSELECTSTRINGS; i++) {
      dfe->addEnumText(gettext(InfoBoxManager::GetTypeDescription(i)));
    }
    dfe->Sort(0);

    int it=0;

    switch(mode) {
    case 1: // climb
      it = InfoBoxManager::getType(item, 0);
      break;
    case 0: // cruise
      it = InfoBoxManager::getType(item, 1);
      break;
    case 2: // final glide
      it = InfoBoxManager::getType(item, 2);
      break;
    case 3: // aux
      it = InfoBoxManager::getType(item, 3);
      break;
    };
    dfe->Set(it);
    wp->RefreshDisplay();
  }
}


static void GetInfoBoxSelector(unsigned item, int mode)
{
  TCHAR name[80];
  InfoBoxPropName(name, item, mode);
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    int itnew = wp->GetDataField()->GetAsInteger();
    int it=0;

    switch(mode) {
    case 1: // climb
      it = InfoBoxManager::getType(item, 0);
      break;
    case 0: // cruise
      it = InfoBoxManager::getType(item, 1);
      break;
    case 2: // final glide
      it = InfoBoxManager::getType(item, 2);
      break;
    case 3: // aux
      it = InfoBoxManager::getType(item, 3);
      break;
    };

    if (it != itnew) {

      changed = true;

      switch(mode) {
      case 0: // cruise
	InfoBoxManager::setType(item, 1, itnew);
	break;
      case 1: // climb
	InfoBoxManager::setType(item, 0, itnew);
	break;
      case 2: // final glide
	InfoBoxManager::setType(item, 2, itnew);
	break;
      case 3: // aux
	InfoBoxManager::setType(item, 3, itnew);
	break;
      };
      StoreType(item, InfoBoxManager::getTypeAll(item));
    }
  }
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
static  DWORD dwPortIndex1 = 0;
static  DWORD dwSpeedIndex1 = 2;
static  DWORD dwPortIndex2 = 0;
static  DWORD dwSpeedIndex2 = 2;
static  int dwDeviceIndex1=0;
static  int dwDeviceIndex2=0;
static  TCHAR DeviceName[DEVNAMESIZE+1];
static  DWORD Speed = 1; // default is knots
static  DWORD TaskSpeed = 2; // default is kph
static  DWORD Distance = 2; // default is km
static  DWORD Lift = 0;
static  DWORD Altitude = 0; //default ft
static  TCHAR temptext[MAX_PATH];



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

  /*
  config_page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

  return;
  */

  wp = (WndProperty*)wf->FindByName(_T("prpUserLevel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Basic")));
    dfe->addEnumText(gettext(_T("Expert")));
    dfe->Set(XCSoarInterface::UserLevel);
    wp->RefreshDisplay();
  }

    const TCHAR *COMMPort[] = {_T("COM1"),_T("COM2"),_T("COM3"),_T("COM4"),
			       _T("COM5"),_T("COM6"),_T("COM7"),_T("COM8"),
			       _T("COM9"),_T("COM10"),_T("COM0")};
    const TCHAR *tSpeed[] = {_T("1200"),_T("2400"),_T("4800"),_T("9600"),
			     _T("19200"),_T("38400"),_T("57600"),_T("115200")};
//  DWORD dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};

  ReadPort1Settings(&dwPortIndex1,&dwSpeedIndex1);

  wp = (WndProperty*)wf->FindByName(_T("prpComPort1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (unsigned i = 0; i < 11; i++) {
      dfe->addEnumText((COMMPort[i]));
    }
    dfe->Set(dwPortIndex1);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComSpeed1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (unsigned i = 0; i < 8; i++) {
      dfe->addEnumText((tSpeed[i]));
    }
    dfe->Set(dwSpeedIndex1);
    wp->RefreshDisplay();
  }

  TCHAR deviceName1[MAX_PATH];
  TCHAR deviceName2[MAX_PATH];
  if (is_simulator()) {
    ReadDeviceSettings(0, deviceName1);
    ReadDeviceSettings(1, deviceName2);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComDevice1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (int i = 0; DeviceRegister[i] != NULL; i++) {
      devRegisterGetName(i, DeviceName);
      dfe->addEnumText((DeviceName));
      if (!is_simulator()) {
        if (devA() != NULL){
          if (_tcscmp(DeviceName, devA()->Name) == 0)
            dwDeviceIndex1 = i;
        }
      } else {
        if (_tcscmp(DeviceName, deviceName1) == 0)
          dwDeviceIndex1 = i;
      }
    }
    dfe->Sort(1);
    dfe->Set(dwDeviceIndex1);
    wp->RefreshDisplay();
  }

  ReadPort2Settings(&dwPortIndex2,&dwSpeedIndex2);

  wp = (WndProperty*)wf->FindByName(_T("prpComPort2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (unsigned i = 0; i < 11; i++) {
      dfe->addEnumText((COMMPort[i]));
    }
    dfe->Set(dwPortIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComSpeed2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (unsigned i = 0; i < 8; i++) {
      dfe->addEnumText((tSpeed[i]));
    }
    dfe->Set(dwSpeedIndex2);
    wp->RefreshDisplay();
  }


  wp = (WndProperty*)wf->FindByName(_T("prpComDevice2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (int i = 0; DeviceRegister[i] != NULL; i++) {
      devRegisterGetName(i, DeviceName);
      dfe->addEnumText((DeviceName));
      if (!is_simulator()) {
        if (devB() != NULL){
          if (_tcscmp(DeviceName, devB()->Name) == 0)
            dwDeviceIndex2 = i;
        }
      } else {
        if (_tcscmp(DeviceName, deviceName2) == 0)
          dwDeviceIndex2 = i;
      }
    }
    dfe->Sort(1);
    dfe->Set(dwDeviceIndex2);
    wp->RefreshDisplay();
  }

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
    wp->GetDataField()->SetAsFloat(iround(XCSoarInterface::SetSettingsComputer().ClipAltitude*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltWarningMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(XCSoarInterface::SetSettingsComputer().AltWarningMargin*ALTITUDEMODIFY));
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
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*XCSoarInterface::SettingsComputer().SafetyAltitudeArrival));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeBreakoff"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*XCSoarInterface::SettingsComputer().SafetyAltitudeBreakoff));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeTerrain"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*XCSoarInterface::SettingsComputer().SafetyAltitudeTerrain));
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
    wp->GetDataField()->Set(XCSoarInterface::SettingsComputer().AutoMacCreadyMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointsOutOfRange"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Ask")));
    dfe->addEnumText(gettext(_T("Include")));
    dfe->addEnumText(gettext(_T("Exclude")));
    wp->GetDataField()->Set(WaypointsOutOfRange);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAutoForceFinalGlide"),
                   XCSoarInterface::SettingsComputer().AutoForceFinalGlide);
  LoadFormProperty(*wf, _T("prpBlockSTF"),
                   XCSoarInterface::SettingsComputer().EnableBlockSTF);
  LoadFormProperty(*wf, _T("prpFAIFinishHeight"),
                   settings_task.EnableFAIFinishHeight);

  wp = (WndProperty*)wf->FindByName(_T("prpOLCRules"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Sprint")));
    dfe->addEnumText(gettext(_T("Triangle")));
    dfe->addEnumText(gettext(_T("Classic")));
    dfe->Set(XCSoarInterface::SettingsComputer().OLCRules);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpHandicap"),
                   XCSoarInterface::SettingsComputer().Handicap);

  if(GetFromRegistryD(szRegistrySpeedUnitsValue,Speed)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistrySpeedUnitsValue, Speed);
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
    dfe->Set(Units::CoordinateFormat);
    wp->RefreshDisplay();
  }

  if(GetFromRegistryD(szRegistryTaskSpeedUnitsValue,TaskSpeed)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryTaskSpeedUnitsValue, TaskSpeed);
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

  if(GetFromRegistryD(szRegistryDistanceUnitsValue,Distance)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryDistanceUnitsValue, Distance);
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

  if(GetFromRegistryD(szRegistryAltitudeUnitsValue,Altitude)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
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

  if(GetFromRegistryD(szRegistryLiftUnitsValue,Lift)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryLiftUnitsValue, Lift);
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

  wp = (WndProperty*)wf->FindByName(_T("prpThermalLocator"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("OFF")));
    dfe->addEnumText(gettext(_T("Circle at center")));
    dfe->addEnumText(gettext(_T("Pan to center")));
    dfe->Set(XCSoarInterface::SettingsComputer().EnableThermalLocator);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpSetSystemTimeFromGPS"),
                   XCSoarInterface::SettingsMap().SetSystemTimeFromGPS);
  LoadFormProperty(*wf, _T("prpAbortSafetyUseCurrent"),
                   GlidePolar::AbortSafetyUseCurrent);
  LoadFormProperty(*wf, _T("prpDisableAutoLogger"),
                   !XCSoarInterface::SettingsComputer().DisableAutoLogger);

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::SafetyMacCready*LIFTMODIFY);
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpRiskGamma"), GlidePolar::RiskGamma);
  LoadFormProperty(*wf, _T("prpAnimation"), XCSoarInterface::EnableAnimation);

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
    wp->GetDataField()->SetAsFloat(iround(SPEEDMODIFY*XCSoarInterface::SettingsComputer().SafetySpeed));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
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
    dfe->Set(POLARID);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryPolarFile, szPolarFile, MAX_PATH);
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

  GetRegistryString(szRegistryAirspaceFile, szAirspaceFile, MAX_PATH);
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

  GetRegistryString(szRegistryAdditionalAirspaceFile,
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

  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  _tcscpy(temptext,szWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.dat"));
    dfe->ScanDirectoryTop(_T("*.xcw"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryAdditionalWayPointFile,
		    szAdditionalWaypointFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(_T("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.dat"));
    dfe->ScanDirectoryTop(_T("*.xcw"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
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

  GetRegistryString(szRegistryTerrainFile, szTerrainFile, MAX_PATH);
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

  GetRegistryString(szRegistryTopologyFile, szTopologyFile, MAX_PATH);
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

  GetRegistryString(szRegistryAirfieldFile, szAirfieldFile, MAX_PATH);
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

  GetRegistryString(szRegistryLanguageFile, szLanguageFile, MAX_PATH);
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

  GetRegistryString(szRegistryStatusFile, szStatusFile, MAX_PATH);
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

  GetRegistryString(szRegistryInputFile, szInputFile, MAX_PATH);
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
    dfe->addEnumText(gettext(_T("HighScore Style")));
    dfe->addEnumText(gettext(_T("Keyboard")));
	dfe->Set(Appearance.TextInputStyle);
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

      if (Layout::landscape) {


        dfe->addEnumText(gettext(_T("vario+9box"))); // 0
        dfe->addEnumText(gettext(_T("(empty) A")));  // 1
        dfe->addEnumText(gettext(_T("(empty) B")));  // 2
        dfe->addEnumText(gettext(_T("(empty) C")));  // 3
        dfe->addEnumText(gettext(_T("8box left")));  // 4
        dfe->addEnumText(gettext(_T("8box right"))); // 5 VENTA3
        dfe->addEnumText(gettext(_T("(empty) D")));  // 6
        dfe->addEnumText(gettext(_T("5box right"))); // 7
        dfe->Set(Appearance.InfoBoxGeom);
        wp->RefreshDisplay();

      } else {
        // VENTA2 FIX portrait mode selection geometry
        dfe->addEnumText(gettext(_T("top+bottom"))); // 0
        dfe->addEnumText(gettext(_T("bottom")));     // 1
        dfe->addEnumText(gettext(_T("top")));        // 2
        dfe->addEnumText(gettext(_T("3 free"))); // 3
        dfe->addEnumText(gettext(_T("4 free")));       // 4
        dfe->addEnumText(gettext(_T("5 free")));      // 5
        dfe->addEnumText(gettext(_T("6 free")));      // 6

        //dfe->addEnumText(gettext(_T("left+right"))); // 3
        //dfe->addEnumText(gettext(_T("left")));       // 4
        //dfe->addEnumText(gettext(_T("right")));      // 5
        //dfe->addEnumText(gettext(_T("vario")));      // 6
        dfe->addEnumText(gettext(_T("7")));          // 7
        dfe->Set(Appearance.InfoBoxGeom);
        wp->RefreshDisplay();

      }
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

  wp = (WndProperty*)wf->FindByName(_T("prpVirtualKeys"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Disabled")));
    dfe->addEnumText(gettext(_T("Enabled")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(CommonInterface::VirtualKeys);
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
    dfb->Set(UseCustomFonts);
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
                   iround(XCSoarInterface::SettingsMap().TerrainContrast*100/255));

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


  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Cylinder")));
    dfe->addEnumText(gettext(_T("Line")));
    dfe->addEnumText(gettext(_T("FAI Sector")));
    dfe->Set(settings_task.FinishType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.FinishRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Cylinder")));
    dfe->addEnumText(gettext(_T("Line")));
    dfe->addEnumText(gettext(_T("FAI Sector")));
    dfe->Set(settings_task.StartType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.StartRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFAISector"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Cylinder")));
    dfe->addEnumText(gettext(_T("FAI Sector")));
    dfe->addEnumText(gettext(_T("DAe 0.5/10")));
    dfe->Set(settings_task.SectorType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.SectorRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Manual")));
    dfe->addEnumText(gettext(_T("Auto")));
    dfe->addEnumText(gettext(_T("Arm")));
    dfe->addEnumText(gettext(_T("Arm start")));
    dfe->Set(settings_task.AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFinishMinHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.FinishMinHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.StartMaxHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeightMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.StartMaxHeightMargin*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("AGL")));
    dfe->addEnumText(gettext(_T("MSL")));
    dfe->Set(settings_task.StartHeightRef);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.StartMaxSpeed*SPEEDMODIFY));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeedMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.StartMaxSpeedMargin*SPEEDMODIFY));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
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




void dlgConfigurationShowModal(void){

  WndProperty *wp;
  XCSoarInterface::StartHourglassCursor();

  if (!Layout::landscape) {
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

  if (!wf) {
    XCSoarInterface::StopHourglassCursor();
    return;
  }

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wConfig1    = ((WndFrame *)wf->FindByName(_T("frmSite")));
  wConfig2    = ((WndFrame *)wf->FindByName(_T("frmAirspace")));
  wConfig3    = ((WndFrame *)wf->FindByName(_T("frmDisplay")));
  wConfig4    = ((WndFrame *)wf->FindByName(_T("frmTerrain")));
  wConfig5    = ((WndFrame *)wf->FindByName(_T("frmFinalGlide")));
  wConfig6    = ((WndFrame *)wf->FindByName(_T("frmSafety")));
  wConfig7    = ((WndFrame *)wf->FindByName(_T("frmPolar")));
  wConfig8    = ((WndFrame *)wf->FindByName(_T("frmComm")));
  wConfig9    = ((WndFrame *)wf->FindByName(_T("frmUnits")));
  wConfig10    = ((WndFrame *)wf->FindByName(_T("frmInterface")));
  wConfig11    = ((WndFrame *)wf->FindByName(_T("frmAppearance")));
  wConfig12    = ((WndFrame *)wf->FindByName(_T("frmFonts")));
  wConfig13    = ((WndFrame *)wf->FindByName(_T("frmVarioAppearance")));
  wConfig14    = ((WndFrame *)wf->FindByName(_T("frmTask")));
  wConfig15    = ((WndFrame *)wf->FindByName(_T("frmTaskRules")));
  wConfig16    = ((WndFrame *)wf->FindByName(_T("frmInfoBoxCircling")));
  wConfig17    = ((WndFrame *)wf->FindByName(_T("frmInfoBoxCruise")));
  wConfig18    = ((WndFrame *)wf->FindByName(_T("frmInfoBoxFinalGlide")));
  wConfig19    = ((WndFrame *)wf->FindByName(_T("frmInfoBoxAuxiliary")));
  wConfig20    = ((WndFrame *)wf->FindByName(_T("frmLogger")));
  wConfig21    = ((WndFrame *)wf->FindByName(_T("frmWaypointEdit")));
  wConfig22    = ((WndFrame *)wf->FindByName(_T("frmSpecials")));

  assert(wConfig1!=NULL);
  assert(wConfig2!=NULL);
  assert(wConfig3!=NULL);
  assert(wConfig4!=NULL);
  assert(wConfig5!=NULL);
  assert(wConfig6!=NULL);
  assert(wConfig7!=NULL);
  assert(wConfig8!=NULL);
  assert(wConfig9!=NULL);
  assert(wConfig10!=NULL);
  assert(wConfig11!=NULL);
  assert(wConfig12!=NULL);
  assert(wConfig13!=NULL);
  assert(wConfig14!=NULL);
  assert(wConfig15!=NULL);
  assert(wConfig16!=NULL);
  assert(wConfig17!=NULL);
  assert(wConfig18!=NULL);
  assert(wConfig19!=NULL);
  assert(wConfig20!=NULL);
  assert(wConfig21!=NULL);
  assert(wConfig22!=NULL);

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

  settings_task = task.getSettings();

  setVariables();

  UpdateDeviceSetupButton(0, devA()->Name);
  UpdateDeviceSetupButton(1, devB()->Name);

  NextPage(0); // just to turn proper pages on/off

  changed = false;
  taskchanged = false;
  requirerestart = false;
  utcchanged = false;
  waypointneedsave = false;

  XCSoarInterface::StopHourglassCursor();
  wf->ShowModal();

  // TODO enhancement: implement a cancel button that skips all this
  // below after exit.

  changed |= SetValueRegistryOnChange(wf, _T("prpAbortSafetyUseCurrent"),
				      szRegistryAbortSafetyUseCurrent,
				      GlidePolar::AbortSafetyUseCurrent);

  wp = (WndProperty*)wf->FindByName(TEXT("prpDisableAutoLogger"));
  if (wp) { // GUI label is "Enable Auto Logger"
    if (!XCSoarInterface::SetSettingsComputer().DisableAutoLogger
          != wp->GetDataField()->GetAsBoolean()) {
      XCSoarInterface::SetSettingsComputer().DisableAutoLogger =
          !(wp->GetDataField()->GetAsBoolean());
      SetToRegistry(szRegistryDisableAutoLogger,
          XCSoarInterface::SetSettingsComputer().DisableAutoLogger);
      changed = true;
    }
  }
  double val;

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    val = wp->GetDataField()->GetAsFloat()/LIFTMODIFY;
    if (GlidePolar::SafetyMacCready != val) {
      GlidePolar::SafetyMacCready = val;
      SetToRegistry(szRegistrySafetyMacCready,
		    iround(GlidePolar::SafetyMacCready*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRiskGamma"));
  if (wp) {
    val = wp->GetDataField()->GetAsFloat();
    if (GlidePolar::RiskGamma != val) {
      GlidePolar::RiskGamma = val;
      SetToRegistry(szRegistryRiskGamma,
		    iround(GlidePolar::RiskGamma*10));
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpSetSystemTimeFromGPS"),
				      szRegistrySetSystemTimeFromGPS,
				      XCSoarInterface::SetSettingsMap().SetSystemTimeFromGPS);
  changed |= SetValueRegistryOnChange(wf, _T("prpEnableAnimation"),
				      szRegistryAnimation,
				      XCSoarInterface::EnableAnimation);
  changed |= SetValueRegistryOnChange(wf, _T("prpTrailDrift"),
				      szRegistryTrailDrift,
				      XCSoarInterface::SetSettingsMap().EnableTrailDrift);
  changed |= SetValueRegistryOnChange(wf, _T("prpThermalLocator"),
				      szRegistryThermalLocator,
				      XCSoarInterface::SetSettingsComputer().EnableThermalLocator);
  changed |= SetValueRegistryOnChange(wf, _T("prpTrail"),
				      szRegistrySnailTrail,
				      XCSoarInterface::SetSettingsMap().TrailActive);

// VENTA3: do not save VisualGlide to registry or profile

  wp = (WndProperty*)wf->FindByName(_T("prpPolarType"));
  if (wp) {
    if (POLARID != (unsigned)wp->GetDataField()->GetAsInteger()) {
      POLARID = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryPolarID, (int &)POLARID);
      GlidePolar::UpdatePolar(false, XCSoarInterface::SettingsComputer());
      POLARFILECHANGED = true;
      changed = true;
    }
  }

  short tmp = XCSoarInterface::SetSettingsComputer().AltitudeMode;
  changed |= SetValueRegistryOnChange(wf, _T("prpAirspaceDisplay"),
				      szRegistryAltMode, tmp);
  XCSoarInterface::SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)tmp;

  changed |= SetValueRegistryOnChange(wf, _T("prpLockSettingsInFlight"),
				      szRegistryLockSettingsInFlight,
				      XCSoarInterface::LockSettingsInFlight);

  changed |= SetValueRegistryOnChange(wf, _T("prpLoggerShortName"),
				      szRegistryLoggerShort,
				      XCSoarInterface::SetSettingsComputer().LoggerShortName);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableFLARMMap"),
				      szRegistryEnableFLARMMap,
				      XCSoarInterface::SetSettingsMap().EnableFLARMMap);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableFLARMGauge"),
				      szRegistryEnableFLARMGauge,
				      XCSoarInterface::SetSettingsMap().EnableFLARMGauge);

  wp = (WndProperty*)wf->FindByName(_T("prpDebounceTimeout"));
  if (wp) {
    if ((int)XCSoarInterface::debounceTimeout != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::debounceTimeout = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDebounceTimeout, (int)XCSoarInterface::debounceTimeout);
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAirspaceOutline"),
				      szRegistryAirspaceBlackOutline,
				      XCSoarInterface::SetSettingsMap().bAirspaceBlackOutline);

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoZoom"),
				      szRegistryAutoZoom,
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
      SetToRegistry(szRegistryUTCOffset, lival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpClipAltitude"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)XCSoarInterface::SetSettingsComputer().ClipAltitude != ival) {
      XCSoarInterface::SetSettingsComputer().ClipAltitude = ival;
      SetToRegistry(szRegistryClipAlt,XCSoarInterface::SetSettingsComputer().ClipAltitude);  // fixed 20060430/sgi was szRegistryAltMode
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltWarningMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)XCSoarInterface::SetSettingsComputer().AltWarningMargin != ival) {
      XCSoarInterface::SetSettingsComputer().AltWarningMargin = ival;
      SetToRegistry(szRegistryAltMargin,XCSoarInterface::SetSettingsComputer().AltWarningMargin);
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAirspaceWarnings"),
				      szRegistryAirspaceWarning,
				      XCSoarInterface::SetSettingsComputer().EnableAirspaceWarnings);

  changed |= SetValueRegistryOnChange(wf, _T("prpWarningTime"),
				      szRegistryWarningTime,
				      XCSoarInterface::SetSettingsComputer().WarningTime);

  changed |= SetValueRegistryOnChange(wf, _T("prpAcknowledgementTime"),
				      szRegistryAcknowledgementTime,
				      XCSoarInterface::SetSettingsComputer().AcknowledgementTime);

  changed |= SetValueRegistryOnChange(wf, _T("prpWaypointLabels"),
				      szRegistryDisplayText,
				      XCSoarInterface::SetSettingsMap().DisplayTextType);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableTerrain"),
				      szRegistryDrawTerrain,
				      XCSoarInterface::SetSettingsMap().EnableTerrain);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableTopology"),
				      szRegistryDrawTopology,
				      XCSoarInterface::SetSettingsMap().EnableTopology);

  changed |= SetValueRegistryOnChange(wf, _T("prpCirclingZoom"),
				      szRegistryCircleZoom,
				      XCSoarInterface::SetSettingsMap().CircleZoom);

  wp = (WndProperty*)wf->FindByName(_T("prpOrientation"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().DisplayOrientation != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().DisplayOrientation = (DisplayOrientation_t)wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDisplayUpValue,
		    XCSoarInterface::SettingsMap().DisplayOrientation);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMenuTimeout"));
  if (wp) {
    if (XCSoarInterface::MenuTimeoutMax != wp->GetDataField()->GetAsInteger()*2) {
      XCSoarInterface::MenuTimeoutMax = wp->GetDataField()->GetAsInteger()*2;
      SetToRegistry(szRegistryMenuTimeout,XCSoarInterface::MenuTimeoutMax);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeArrival"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (XCSoarInterface::SettingsComputer().SafetyAltitudeArrival != ival) {
      XCSoarInterface::SetSettingsComputer().SafetyAltitudeArrival = ival;
      SetToRegistry(szRegistrySafetyAltitudeArrival,
		    (DWORD)XCSoarInterface::SettingsComputer().SafetyAltitudeArrival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeBreakoff"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (XCSoarInterface::SettingsComputer().SafetyAltitudeBreakoff != ival) {
      XCSoarInterface::SetSettingsComputer().SafetyAltitudeBreakoff = ival;
      SetToRegistry(szRegistrySafetyAltitudeBreakOff,
		    (DWORD)XCSoarInterface::SettingsComputer().SafetyAltitudeBreakoff);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyAltitudeTerrain"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (XCSoarInterface::SettingsComputer().SafetyAltitudeTerrain != ival) {
      XCSoarInterface::SetSettingsComputer().SafetyAltitudeTerrain = ival;
      SetToRegistry(szRegistrySafetyAltitudeTerrain,
		    (DWORD)XCSoarInterface::SettingsComputer().SafetyAltitudeTerrain);
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoWind"),
				      szRegistryAutoWind,
				      XCSoarInterface::SetSettingsComputer().AutoWindMode);

  changed |= SetValueRegistryOnChange(wf, _T("prpWindArrowStyle"),
				      szRegistryWindArrowStyle,
				      XCSoarInterface::SetSettingsMap().WindArrowStyle);

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoMcMode"),
				      szRegistryAutoMcMode,
				      XCSoarInterface::SetSettingsComputer().AutoMacCreadyMode);

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointsOutOfRange"));
  if (wp) {
    if (WaypointsOutOfRange != wp->GetDataField()->GetAsInteger()) {
      WaypointsOutOfRange = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryWaypointsOutOfRange, WaypointsOutOfRange);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoForceFinalGlide"),
				      szRegistryAutoForceFinalGlide,
				      XCSoarInterface::SetSettingsComputer().AutoForceFinalGlide);

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableNavBaroAltitude"),
				      szRegistryEnableNavBaroAltitude,
				      XCSoarInterface::SetSettingsComputer().EnableNavBaroAltitude);

  changed |= SetValueRegistryOnChange(wf, _T("prpFinalGlideTerrain"),
				      szRegistryFinalGlideTerrain,
				      XCSoarInterface::SetSettingsComputer().FinalGlideTerrain);

  changed |= SetValueRegistryOnChange(wf, _T("prpBlockSTF"),
				      szRegistryBlockSTF,
				      XCSoarInterface::SetSettingsComputer().EnableBlockSTF);

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  if (wp) {
    if ((int)Speed != wp->GetDataField()->GetAsInteger()) {
      Speed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySpeedUnitsValue, Speed);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  if (wp) {
    if ((int)Units::CoordinateFormat != wp->GetDataField()->GetAsInteger()) {
      Units::CoordinateFormat = (CoordinateFormats_t)
        wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryLatLonUnits, Units::CoordinateFormat);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTaskSpeed"));
  if (wp) {
    if ((int)TaskSpeed != wp->GetDataField()->GetAsInteger()) {
      TaskSpeed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryTaskSpeedUnitsValue, TaskSpeed);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsDistance"));
  if (wp) {
    if ((int)Distance != wp->GetDataField()->GetAsInteger()) {
      Distance = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDistanceUnitsValue, Distance);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLift"));
  if (wp) {
    if ((int)Lift != wp->GetDataField()->GetAsInteger()) {
      Lift = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryLiftUnitsValue, Lift);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsAltitude"));
  if (wp) {
    if ((int)Altitude != wp->GetDataField()->GetAsInteger()) {
      Altitude = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
      Units::NotifyUnitChanged();
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFAIFinishHeight"));
  if (wp) {
    if (settings_task.EnableFAIFinishHeight != (wp->GetDataField()->GetAsInteger()>0)) {
      settings_task.EnableFAIFinishHeight = (wp->GetDataField()->GetAsInteger()>0);
      SetToRegistry(szRegistryFAIFinishHeight, settings_task.EnableFAIFinishHeight);
      changed = true;
      taskchanged = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpOLCRules"),
				      szRegistryOLCRules,
				      XCSoarInterface::SetSettingsComputer().OLCRules);

  changed |= SetValueRegistryOnChange(wf, _T("prpHandicap"),
				      szRegistryHandicap,
				      XCSoarInterface::SetSettingsComputer().Handicap);

  wp = (WndProperty*)wf->FindByName(_T("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szPolarFile)) {
      SetRegistryString(szRegistryPolarFile, temptext);
      POLARFILECHANGED = true;
      GlidePolar::UpdatePolar(false, XCSoarInterface::SettingsComputer());
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szWaypointFile)) {
      SetRegistryString(szRegistryWayPointFile, temptext);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAdditionalWaypointFile)) {
      SetRegistryString(szRegistryAdditionalWayPointFile, temptext);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAirspaceFile)) {
      SetRegistryString(szRegistryAirspaceFile, temptext);
      AIRSPACEFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAdditionalAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAdditionalAirspaceFile)) {
      SetRegistryString(szRegistryAdditionalAirspaceFile, temptext);
      AIRSPACEFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMapFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szMapFile)) {
      SetRegistryString(szRegistryMapFile, temptext);
      MAPFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szTerrainFile)) {
      SetRegistryString(szRegistryTerrainFile, temptext);
      TERRAINFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szTopologyFile)) {
      SetRegistryString(szRegistryTopologyFile, temptext);
      TOPOLOGYFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAirfieldFile)) {
      SetRegistryString(szRegistryAirfieldFile, temptext);
      AIRFIELDFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szLanguageFile)) {
      SetRegistryString(szRegistryLanguageFile, temptext);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szStatusFile)) {
      SetRegistryString(szRegistryStatusFile, temptext);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szInputFile)) {
      SetRegistryString(szRegistryInputFile, temptext);
      requirerestart = true;
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpBallastSecsToEmpty"),
				      szRegistryBallastSecsToEmpty,
				      XCSoarInterface::SetSettingsComputer().BallastSecsToEmpty);

  wp = (WndProperty*)wf->FindByName(_T("prpMaxManoeuveringSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if (XCSoarInterface::SettingsComputer().SafetySpeed != ival) {
      XCSoarInterface::SetSettingsComputer().SafetySpeed = ival;
      SetToRegistry(szRegistrySafteySpeed,(DWORD)XCSoarInterface::SettingsComputer().SafetySpeed);
      GlidePolar::UpdatePolar(false, XCSoarInterface::SettingsComputer());
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishLine"));
  if (wp) {
    if ((int)settings_task.FinishType != wp->GetDataField()->GetAsInteger()) {
      settings_task.FinishType = (FinishSectorType_t)wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFinishLine,settings_task.FinishType);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)settings_task.FinishRadius != ival) {
      settings_task.FinishRadius = ival;
      SetToRegistry(szRegistryFinishRadius,settings_task.FinishRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartLine"));
  if (wp) {
    if ((int)settings_task.StartType != wp->GetDataField()->GetAsInteger()) {
      settings_task.StartType = (StartSectorType_t)wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryStartLine,settings_task.StartType);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)settings_task.StartRadius != ival) {
      settings_task.StartRadius = ival;
      SetToRegistry(szRegistryStartRadius,settings_task.StartRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFAISector"));
  if (wp) {
    if ((int)settings_task.SectorType != wp->GetDataField()->GetAsInteger()) {
      settings_task.SectorType = (ASTSectorType_t)wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFAISector,settings_task.SectorType);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskSectorRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)settings_task.SectorRadius != ival) {
      settings_task.SectorRadius = ival;
      SetToRegistry(szRegistrySectorRadius,settings_task.SectorRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndFinalGlide"));
  if (wp) {
    if (Appearance.IndFinalGlide != (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndFinalGlide = (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndFinalGlide,(DWORD)(Appearance.IndFinalGlide));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppCompassAppearance"));
  if (wp) {
    if (Appearance.CompassAppearance != (CompassAppearance_t)
	(wp->GetDataField()->GetAsInteger())) {
      Appearance.CompassAppearance = (CompassAppearance_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppCompassAppearance,
		    (DWORD)(Appearance.CompassAppearance));
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
      SetToRegistry(szRegistryAppInfoBoxBorder,
		    (DWORD)(Appearance.InfoBoxBorder));
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
      SetToRegistry(szRegistryExtendedVisualGlide,
		    (DWORD)(XCSoarInterface::SettingsMap().ExtendedVisualGlide));
      changed = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpVirtualKeys")); // VENTA6
  if (wp) {
    if (CommonInterface::VirtualKeys != (VirtualKeys_t)
	(wp->GetDataField()->GetAsInteger())) {
      CommonInterface::VirtualKeys = (VirtualKeys_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryVirtualKeys,
		    (DWORD)(CommonInterface::VirtualKeys));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime")); // VENTA6
  if (wp) {
    if (XCSoarInterface::SettingsComputer().AverEffTime !=
	(wp->GetDataField()->GetAsInteger())) {
      XCSoarInterface::SetSettingsComputer().AverEffTime =
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAverEffTime,
		    (DWORD)(XCSoarInterface::SettingsComputer().AverEffTime));
      changed = true;
      requirerestart = true;
    }
  }


#if defined(PNA) || defined(FIVV)
// VENTA-ADDON GEOM
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    if (Appearance.InfoBoxGeom != (InfoBoxGeomAppearance_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.InfoBoxGeom = (InfoBoxGeomAppearance_t)
        (wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppInfoBoxGeom,
                    (DWORD)(Appearance.InfoBoxGeom));
      changed = true;
      requirerestart = true;
    }
  }
//
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
      SetToRegistry(szRegistryAppInfoBoxModel,
                    GlobalModelType);
      changed = true;
      requirerestart = true;
    }
  }
//
#endif

  //Fonts
  int UseCustomFontsold = UseCustomFonts;
  wp = (WndProperty*)wf->FindByName(_T("prpUseCustomFonts"));
  if (wp) {
    DataFieldBoolean * dfb = (DataFieldBoolean*) wp->GetDataField();
    if (dfb) {
      SetToRegistry(szRegistryUseCustomFonts, dfb->GetAsInteger());
      UseCustomFonts = dfb->GetAsInteger(); // global var
    }
  }
  if ( (UseCustomFontsold != UseCustomFonts) ||
    (UseCustomFonts && FontRegistryChanged) ) {
      changed = true;
      requirerestart = true;
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
      SetToRegistry(szRegistryAppStatusMessageAlignment,
		    (DWORD)(Appearance.StateMessageAlign));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTextInput"));
  if (wp)
  {
    if (Appearance.TextInputStyle != (TextInputStyle_t)(wp->GetDataField()->GetAsInteger()))
      {
	Appearance.TextInputStyle = (TextInputStyle_t)(wp->GetDataField()->GetAsInteger());
	SetToRegistry(szRegistryAppTextInputStyle, (DWORD)(Appearance.TextInputStyle));
	changed = true;
      }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndLandable,(DWORD)(Appearance.IndLandable));
      changed = true;
      requirerestart = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpEnableExternalTriggerCruise"),
				      szRegistryEnableExternalTriggerCruise,
				      XCSoarInterface::SetSettingsComputer().EnableExternalTriggerCruise);

  wp = (WndProperty*)wf->FindByName(_T("prpAppInverseInfoBox"));
  if (wp) {
    if ((int)(Appearance.InverseInfoBox) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InverseInfoBox = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppInverseInfoBox,Appearance.InverseInfoBox);
      requirerestart = true;
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpGliderScreenPosition"),
				      szRegistryGliderScreenPosition,
				      XCSoarInterface::SetSettingsMap().GliderScreenPosition);

  wp = (WndProperty*)wf->FindByName(_T("prpAppDefaultMapWidth"));
  if (wp) {
    if ((int)(Appearance.DefaultMapWidth) != wp->GetDataField()->GetAsInteger()) {
      Appearance.DefaultMapWidth = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppDefaultMapWidth,Appearance.DefaultMapWidth);
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
      SetToRegistry(szRegistryAppAveNeedle,Appearance.GaugeVarioAveNeedle);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxColors"));
  if (wp) {
    if ((int)(Appearance.InfoBoxColors) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InfoBoxColors = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppInfoBoxColors,Appearance.InfoBoxColors);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    if ((int)(Appearance.GaugeVarioSpeedToFly) != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioSpeedToFly = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioSpeedToFly,Appearance.GaugeVarioSpeedToFly);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppGaugeVarioAvgText"));
  if (wp) {
    if ((int)Appearance.GaugeVarioAvgText != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAvgText = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioAvgText,Appearance.GaugeVarioAvgText);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppGaugeVarioGross"));
  if (wp) {
    if ((int)Appearance.GaugeVarioGross != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioGross = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioGross,Appearance.GaugeVarioGross);
      changed = true;
      requirerestart = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf, _T("prpAppGaugeVarioMc"),
				      szRegistryAppGaugeVarioMc,
				      Appearance.GaugeVarioMc);

  changed |= SetValueRegistryOnChange(wf, _T("prpAppGaugeVarioBugs"),
				      szRegistryAppGaugeVarioBugs,
				      Appearance.GaugeVarioBugs);

  changed |= SetValueRegistryOnChange(wf, _T("prpAppGaugeVarioBallast"),
				      szRegistryAppGaugeVarioBallast,
				      Appearance.GaugeVarioBallast);

#ifdef HAVE_BLANK
  wp = (WndProperty*)wf->FindByName(_T("prpAutoBlank"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().EnableAutoBlank != (wp->GetDataField()->GetAsInteger()!=0)) {
      XCSoarInterface::SetSettingsMap().EnableAutoBlank = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAutoBlank, XCSoarInterface::SettingsMap().EnableAutoBlank);
      changed = true;
    }
  }
#endif

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoBacklight"), // VENTA4
				      szRegistryAutoBacklight,
				      CommonInterface::EnableAutoBacklight);

  changed |= SetValueRegistryOnChange(wf, _T("prpAutoSoundVolume"), // VENTA4
				      szRegistryAutoSoundVolume,
				      CommonInterface::EnableAutoSoundVolume);

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainContrast"));
  if (wp) {
    if (iround(XCSoarInterface::SettingsMap().TerrainContrast*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().TerrainContrast = (short)iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainContrast,XCSoarInterface::SettingsMap().TerrainContrast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainBrightness"));
  if (wp) {
    if (iround(XCSoarInterface::SettingsMap().TerrainBrightness*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().TerrainBrightness = (short)iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainBrightness,
		    XCSoarInterface::SettingsMap().TerrainBrightness);
      changed = true;
    }
  }

  changed |= SetValueRegistryOnChange(wf,
				      _T("prpTerrainRamp"),
				      szRegistryTerrainRamp,
				      XCSoarInterface::SetSettingsMap().TerrainRamp);

  wp = (WndProperty*)wf->FindByName(_T("prpFinishMinHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)settings_task.FinishMinHeight != ival) {
      settings_task.FinishMinHeight = ival;
      SetToRegistry(szRegistryFinishMinHeight,settings_task.FinishMinHeight);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)settings_task.StartMaxHeight != ival) {
      settings_task.StartMaxHeight = ival;
      SetToRegistry(szRegistryStartMaxHeight,settings_task.StartMaxHeight);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeightMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)settings_task.StartMaxHeightMargin != ival) {
      settings_task.StartMaxHeightMargin = ival;
      SetToRegistry(szRegistryStartMaxHeightMargin,settings_task.StartMaxHeightMargin);
      changed = true;
      taskchanged = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    if (settings_task.StartHeightRef != wp->GetDataField()->GetAsInteger()) {
      settings_task.StartHeightRef = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryStartHeightRef, settings_task.StartHeightRef);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if ((int)settings_task.StartMaxSpeed != ival) {
      settings_task.StartMaxSpeed = ival;
      SetToRegistry(szRegistryStartMaxSpeed,settings_task.StartMaxSpeed);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeedMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if ((int)settings_task.StartMaxSpeedMargin != ival) {
      settings_task.StartMaxSpeedMargin = ival;
      SetToRegistry(szRegistryStartMaxSpeedMargin,settings_task.StartMaxSpeedMargin);
      changed = true;
      taskchanged = true;
    }
  }

  tmp = settings_task.AutoAdvance;
  taskchanged |= SetValueRegistryOnChange(wf, _T("prpAutoAdvance"),
                                          szRegistryAutoAdvance,
                                          tmp);
  settings_task.AutoAdvance = (AutoAdvanceMode_t)tmp;

  changed |= SetValueRegistryOnChange(wf, _T("prpLoggerTimeStepCruise"),
				      szRegistryLoggerTimeStepCruise,
				      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCruise);

  changed |= SetValueRegistryOnChange(wf, _T("prpLoggerTimeStepCircling"),
				      szRegistryLoggerTimeStepCircling,
				      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCircling);

  wp = (WndProperty*)wf->FindByName(_T("prpComPort1"));
  if (wp) {
    if ((int)dwPortIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwPortIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComSpeed1"));
  if (wp) {
    if ((int)dwSpeedIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwSpeedIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComDevice1"));
  if (wp) {
    if (dwDeviceIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwDeviceIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
      devRegisterGetName(dwDeviceIndex1, DeviceName);
      WriteDeviceSettings(0, DeviceName);
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComPort2"));
  if (wp) {
    if ((int)dwPortIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwPortIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComSpeed2"));
  if (wp) {
    if ((int)dwSpeedIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwSpeedIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpComDevice2"));
  if (wp) {
    if (dwDeviceIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwDeviceIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
      devRegisterGetName(dwDeviceIndex2, DeviceName);
      WriteDeviceSettings(1, DeviceName);
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSnailWidthScale"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().SnailWidthScale != wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().SnailWidthScale = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySnailWidthScale,
		    XCSoarInterface::SettingsMap().SnailWidthScale);
      changed = true;
      requirerestart = true;
    }
  }

#ifdef FIVV
//  if ( XCSoarInterface::Calculated().OnGround == TRUE ) {
//	  wp = (WndProperty*)wf->FindByName(_T("prpGPSAltitudeOffset")); // VENTA3
//	  if (wp) GPSAltitudeOffset = wp->GetDataField()->GetAsInteger();
//  }
#endif


  if (COMPORTCHANGED) {
    WritePort1Settings(dwPortIndex1,dwSpeedIndex1);
    WritePort2Settings(dwPortIndex2,dwSpeedIndex2);
  }

  for (unsigned i = 0; i < 4; i++) {
    for (unsigned j = 0; j < numInfoWindows; j++) {
      GetInfoBoxSelector(j, i);
    }
  }

  if (waypointneedsave) {
    if(MessageBoxX(gettext(_T("Save changes to waypoint file?")),
                   gettext(_T("Waypoints edited")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      AskWaypointSave();

    }
  }

  if (taskchanged) {
    changed = true;
    task.setSettings(settings_task);
    task.RefreshTask(XCSoarInterface::SettingsComputer(),
                     XCSoarInterface::Basic());
  }

  if (!is_embedded() && COMPORTCHANGED) {
    requirerestart = true;
  }

  if (changed) {
    Profile::StoreRegistry();

    if (!requirerestart) {
      MessageBoxX (
		   gettext(_T("Changes to configuration saved.")),
		   _T(""), MB_OK);
    } else {

      MessageBoxX (
		   gettext(_T("Changes to configuration saved.  Restart XCSoar to apply changes.")),
		   _T(""), MB_OK);
    }
  }

  delete wf;

  wf = NULL;

}
