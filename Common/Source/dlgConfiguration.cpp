/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Dialogs.h"
#include "XCSoar.h"
#include "externs.h"
#include "Parser.h"
#include "Utils2.h"
#include "externs.h"
#include "MapWindow.h"
#include "Terrain.h"
#include "GaugeFLARM.h"
#include "LocalPath.hpp"
#include "Utils.h"
#include "WindowControls.h"
#include "Statistics.h"
#include "Logger.h"
#include "McReady.h"
#include "dlgTools.h"
#include "device.h"
#include "Screen/Animation.hpp"
#include "Screen/Blank.hpp"
#include "Registry.hpp"
#include "Process.h"
#include "McReady.h"
#include "Math/FastMath.h"
#include "InfoBoxLayout.h"
#include "Waypointparser.h"
#include "Polar/BuiltIn.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/FileReader.hpp"

#include <assert.h>

static HFONT TempInfoWindowFont;
static HFONT TempTitleWindowFont;
static HFONT TempMapWindowFont;
static HFONT TempTitleSmallWindowFont;
static HFONT TempMapWindowBoldFont;
static HFONT TempCDIWindowFont; // New
static HFONT TempMapLabelFont;
static HFONT TempStatisticsFont;
static HFONT TempUseCustomFontsFont;

extern LOGFONT autoInfoWindowLogFont;
extern LOGFONT autoTitleWindowLogFont;
extern LOGFONT autoMapWindowLogFont;
extern LOGFONT autoTitleSmallWindowLogFont;
extern LOGFONT autoMapWindowBoldLogFont;
extern LOGFONT autoCDIWindowLogFont; // New
extern LOGFONT autoMapLabelLogFont;
extern LOGFONT autoStatisticsLogFont;

extern void InitializeOneFont (HFONT * theFont,
                               const TCHAR FontRegKey[] ,
                               LOGFONT autoLogFont,
                               LOGFONT * LogFontUsed);

extern bool dlgFontEditShowModal(const TCHAR * FontDescription,
                          const TCHAR * FontRegKey,
                          LOGFONT autoLogFont);

int UserLevel = 0;

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
      _stprintf(val, gettext(TEXT("(blank)")));
    }
    _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Pilot name")), val);
    buttonPilotName->SetCaption(text);
  }
  if (buttonAircraftType) {
    GetRegistryString(szRegistryAircraftType, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(TEXT("(blank)")));
    }
    _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Aircraft type")), val);
    buttonAircraftType->SetCaption(text);
  }
  if (buttonAircraftRego) {
    GetRegistryString(szRegistryAircraftRego, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(TEXT("(blank)")));
    }
    _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Competition ID")), val);
    buttonAircraftRego->SetCaption(text);
  }
  if (buttonLoggerID) {
    GetRegistryString(szRegistryLoggerID, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(val, gettext(TEXT("(blank)")));
    }
    _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Logger ID")), val);
    buttonLoggerID->SetCaption(text);
  }
}

static void NextPage(int Step){
  config_page += Step;
  if (config_page>=NUMPAGES) { config_page=0; }
  if (config_page<0) { config_page=NUMPAGES-1; }
  switch(config_page) {
  case 0:
    wf->SetCaption(gettext(TEXT("1 Site")));
    break;
  case 1:
    wf->SetCaption(gettext(TEXT("2 Airspace")));
    break;
  case 2:
    wf->SetCaption(gettext(TEXT("3 Map Display")));
    break;
  case 3:
    wf->SetCaption(gettext(TEXT("4 Terrain Display")));
    break;
  case 4:
    wf->SetCaption(gettext(TEXT("5 Glide Computer")));
    break;
  case 5:
    wf->SetCaption(gettext(TEXT("6 Safety factors")));
    break;
  case 6:
    wf->SetCaption(gettext(TEXT("7 Polar")));
    break;
  case 7:
    wf->SetCaption(gettext(TEXT("8 Devices")));
    break;
  case 8:
    wf->SetCaption(gettext(TEXT("9 Units")));
    break;
  case 9:
    wf->SetCaption(gettext(TEXT("10 Interface")));
    break;
  case 10:
    wf->SetCaption(gettext(TEXT("11 Appearance")));
    break;
  case 11:
    wf->SetCaption(gettext(TEXT("12 Fonts")));
    break;
  case 12:
    wf->SetCaption(gettext(TEXT("13 Vario Gauge and FLARM")));
    break;
  case 13:
    wf->SetCaption(gettext(TEXT("14 Task")));
    break;
  case 14:
    wf->SetCaption(gettext(TEXT("15 Task rules")));
    break;
  case 15:
    wf->SetCaption(gettext(TEXT("16 InfoBox Cruise")));
    break;
  case 16:
    wf->SetCaption(gettext(TEXT("17 InfoBox Circling")));
    break;
  case 17:
    wf->SetCaption(gettext(TEXT("18 InfoBox Final Glide")));
    break;
  case 18:
    wf->SetCaption(gettext(TEXT("19 InfoBox Auxiliary")));
    break;
  case 19:
    wf->SetCaption(gettext(TEXT("20 Logger")));
    break;
  case 20:
    wf->SetCaption(gettext(TEXT("21 Waypoint Edit")));
    break;
  case 21:
    wf->SetCaption(gettext(TEXT("22 Experimental features")));
    break;
  }
  if ((config_page>=15) && (config_page<=18)) {
    if (buttonCopy) {
      buttonCopy->SetVisible(true);
    }
    if (buttonPaste) {
      buttonPaste->SetVisible(true);
    }
  } else {
    if (buttonCopy) {
      buttonCopy->SetVisible(false);
    }
    if (buttonPaste) {
      buttonPaste->SetVisible(false);
    }
  }
  wConfig1->SetVisible(config_page == 0);
  wConfig2->SetVisible(config_page == 1);
  wConfig3->SetVisible(config_page == 2);
  wConfig4->SetVisible(config_page == 3);
  wConfig5->SetVisible(config_page == 4);
  wConfig6->SetVisible(config_page == 5);
  wConfig7->SetVisible(config_page == 6);
  wConfig8->SetVisible(config_page == 7);
  wConfig9->SetVisible(config_page == 8);
  wConfig10->SetVisible(config_page == 9);
  wConfig11->SetVisible(config_page == 10);
  wConfig12->SetVisible(config_page == 11);
  wConfig13->SetVisible(config_page == 12);
  wConfig14->SetVisible(config_page == 13);
  wConfig15->SetVisible(config_page == 14);
  wConfig16->SetVisible(config_page == 15);
  wConfig17->SetVisible(config_page == 16);
  wConfig18->SetVisible(config_page == 17);
  wConfig19->SetVisible(config_page == 18);
  wConfig20->SetVisible(config_page == 19);
  wConfig21->SetVisible(config_page == 20);
  wConfig22->SetVisible(config_page == 21);
}



static void OnSetupDeviceAClicked(WindowControl * Sender){
  (void)Sender;

  #if (ToDo)
    devA()->DoSetup();
    wf->FocusNext(NULL);
  #endif

// this is a hack, devices dont jet support device dependant setup dialogs

#ifndef _SIM_
    if ((devA() == NULL) ||
	(_tcscmp(devA()->Name,TEXT("Vega")) != 0)) {
      return;
    }
#endif

    changed = dlgConfigurationVarioShowModal();

    // this is a hack to get the dialog to retain focus because
    // the progress dialog in the vario configuration somehow causes
    // focus problems
    wf->FocusNext(NULL);
}


static void OnSetupDeviceBClicked(WindowControl * Sender){
  (void)Sender;

  #if (ToDo)
    devB()->DoSetup();
    wf->FocusNext(NULL);
  #endif

// this is a hack, devices dont jet support device dependant setup dialogs

#ifndef _SIM_
    if ((devB() == NULL) ||
	(_tcscmp(devB()->Name,TEXT("Vega")) != 0)) {
      return;
    }
#endif

    changed = dlgConfigurationVarioShowModal();

    // this is a hack to get the dialog to retain focus because
    // the progress dialog in the vario configuration somehow causes
    // focus problems
    wf->FocusNext(NULL);
}

static void UpdateDeviceSetupButton(int DeviceIdx, TCHAR *Name){

  WndButton *wb;

  if (DeviceIdx == 0){

    wb = ((WndButton *)wf->FindByName(TEXT("cmdSetupDeviceA")));
    if (wb != NULL) {
      if (_tcscmp(Name, TEXT("Vega")) == 0)
        wb->SetVisible(true);
      else
        wb->SetVisible(false);
    }

  }

  if (DeviceIdx == 1){

    wb = ((WndButton *)wf->FindByName(TEXT("cmdSetupDeviceB")));
    if (wb != NULL) {
      if (_tcscmp(Name, TEXT("Vega")) == 0)
        wb->SetVisible(true);
      else
        wb->SetVisible(false);
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
      wp = (WndProperty*)wf->FindByName(TEXT("prpUserLevel"));
      if (wp) {
        if (wp->GetDataField()->GetAsInteger() != UserLevel) {
          UserLevel = wp->GetDataField()->GetAsInteger();
          changed = true;
          SetToRegistry(szRegistryUserLevel,UserLevel);
          wf->FilterAdvanced(UserLevel>0);
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
                        TEXT("THIS FONT IS NOT CUSTOMIZABLE"),
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
  wp = (WndProperty*)wf->FindByName(TEXT("cmdInfoWindowFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdTitleWindowFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdMapWindowFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdTitleSmallWindowFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdMapWindowBoldFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdCDIWindowFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdMapLabelFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdStatisticsFont"));
  if (wp) {
    wp->SetVisible(bVisible);
  }
}


static void RefreshFonts(void) {

  WndProperty * wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpUseCustomFonts"));
  if (wp) {
    bool bUseCustomFonts= ((DataFieldBoolean*)(wp->GetDataField()))->GetAsBoolean();
    ResetFonts(bUseCustomFonts);
    wp->SetFont(TempUseCustomFontsFont); // this font is never customized
    wp->SetVisible(false);
    wp->SetVisible(true);
    ShowFontEditButtons(bUseCustomFonts);

  }

// now set SampleTexts on the Fonts frame
  wp = (WndProperty*)wf->FindByName(TEXT("prpInfoWindowFont"));
  if (wp) {
    wp->SetFont(TempInfoWindowFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTitleWindowFont"));
  if (wp) {
    wp->SetFont(TempTitleWindowFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpMapWindowFont"));
  if (wp) {
    wp->SetFont(TempMapWindowFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpTitleSmallWindowFont"));
  if (wp) {
    wp->SetFont(TempTitleSmallWindowFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpMapWindowBoldFont"));
  if (wp) {
    wp->SetFont(TempMapWindowBoldFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpCDIWindowFont"));
  if (wp) {
    wp->SetFont(TempCDIWindowFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpMapLabelFont"));
  if (wp) {
    wp->SetFont(TempMapLabelFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpStatisticsFont"));
  if (wp) {
    wp->SetFont(TempStatisticsFont);
    wp->SetVisible(false);
    wp->SetVisible(true);
  }

  // now fix the rest of the dlgConfiguration fonts:
  wf->SetFont(TempMapWindowBoldFont);
  wf->SetTitleFont(TempMapWindowBoldFont);

  wp = (WndProperty*)wf->FindByName(TEXT("cmdNext"));
  if (wp) {
    wp->SetFont(TempCDIWindowFont);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("cmdPrev"));
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
  GetFontDescription(FontDesc, TEXT("prpInfoWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontInfoWindowFont,
                            autoInfoWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}

static void OnEditTitleWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpTitleWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontTitleWindowFont,
                            autoTitleWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditMapWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpMapWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontMapWindowFont,
                            autoMapWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditTitleSmallWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpTitleSmallWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontTitleSmallWindowFont,
                            autoTitleSmallWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditMapWindowBoldFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpMapWindowBoldFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontMapWindowBoldFont,
                            autoMapWindowBoldLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditCDIWindowFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpCDIWindowFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontCDIWindowFont,
                            autoCDIWindowLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditMapLabelFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpMapLabelFont"), MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc,
                            szRegistryFontMapLabelFont,
                            autoMapLabelLogFont)) {
    FontRegistryChanged=true;
    RefreshFonts();
  }
}
static void OnEditStatisticsFontClicked(WindowControl *Sender) {
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];
  GetFontDescription(FontDesc, TEXT("prpStatisticsFont"), MAX_EDITFONT_DESC_LEN);
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
    dlgTextEntryShowModal(Temp,100);
    SetRegistryString(szRegistryAircraftRego,Temp);
    changed = true;
  }
  UpdateButtons();
}


static void OnAircraftTypeClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonAircraftType) {
    GetRegistryString(szRegistryAircraftType,Temp,100);
    dlgTextEntryShowModal(Temp,100);
    SetRegistryString(szRegistryAircraftType,Temp);
    changed = true;
  }
  UpdateButtons();
}


static void OnPilotNameClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonPilotName) {
    GetRegistryString(szRegistryPilotName,Temp,100);
    dlgTextEntryShowModal(Temp,100);
    SetRegistryString(szRegistryPilotName,Temp);
    changed = true;
  }
  UpdateButtons();
}


static void OnLoggerIDClicked(WindowControl *Sender) {
	(void)Sender;
  TCHAR Temp[100];
  if (buttonLoggerID) {
    GetRegistryString(szRegistryLoggerID,Temp,100);
    dlgTextEntryShowModal(Temp,100);
    SetRegistryString(szRegistryLoggerID,Temp);
    changed = true;
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
  _tcscpy(name,TEXT("prpInfoBox"));
  switch (mode) {
  case 0:
    _tcscat(name,TEXT("Circling"));
    break;
  case 1:
    _tcscat(name,TEXT("Cruise"));
    break;
  case 2:
    _tcscat(name,TEXT("FinalGlide"));
    break;
  case 3:
    _tcscat(name,TEXT("Aux"));
    break;
  }
  TCHAR buf[3];
  _stprintf(buf,TEXT("%1d"), item);
  _tcscat(name,buf);
}

static void OnCopy(WindowControl *Sender) {
  (void)Sender;
  int mode = page2mode();
  TCHAR name[80];
  if ((mode<0)||(mode>3)) {
    return;
  }

  for (int item=0; item<numInfoWindows; item++) {
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

  if(MessageBoxX(gettext(TEXT("Overwrite?")),
		 gettext(TEXT("InfoBox paste")),
		 MB_YESNO|MB_ICONQUESTION) == IDYES) {

    for (int item=0; item<numInfoWindows; item++) {
      InfoBoxPropName(name, item, mode);
      WndProperty *wp;
      wp = (WndProperty*)wf->FindByName(name);
      if (wp && (cpyInfoBox[item]>=0)&&(cpyInfoBox[item]<NUMSELECTSTRINGS)) {
	wp->GetDataField()->Set(cpyInfoBox[item]);
	wp->RefreshDisplay();
      }
    }
  }
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
	(void)lParam;
	(void)Sender;
  switch(wParam & 0xffff){
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static void SetLocalTime(void) {
  WndProperty* wp;
  TCHAR temp[20];
  Units::TimeToText(temp,
		    (int)TimeLocal((int)(GPS_INFO.Time)));

  wp = (WndProperty*)wf->FindByName(TEXT("prpLocalTime"));
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
      if (UTCOffset != ival) {
	UTCOffset = ival;
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
      if (Sender->GetAsString() != NULL && _tcscmp(Sender->GetAsString(), TEXT("")) != 0){
        // then ... set Polar Tape to Winpilot

        wp = (WndProperty *)wf->FindByName(TEXT("prpPolarType"));

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
      wp = (WndProperty *)wf->FindByName(TEXT("prpPolarFile"));

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
  edit_waypoint.Latitude = GPS_INFO.Latitude;
  edit_waypoint.Longitude = GPS_INFO.Longitude;
  edit_waypoint.FileNum = 0; // default, put into primary waypoint file
  edit_waypoint.Flags = 0;
  edit_waypoint.Comment[0] = 0;
  edit_waypoint.Name[0] = 0;
  edit_waypoint.Details = 0;
  edit_waypoint.Number = NumberOfWayPoints;
  dlgWaypointEditShowModal(&edit_waypoint);
  if (_tcslen(edit_waypoint.Name)>0) {
    LockTaskData();
    WAYPOINT *new_waypoint = GrowWaypointList();
    if (new_waypoint) {
      memcpy(new_waypoint,&edit_waypoint,sizeof(WAYPOINT));
      new_waypoint->Details= 0;
      waypointneedsave = true;
    }
    UnlockTaskData();
  }
}


static void OnWaypointEditClicked(WindowControl * Sender){
	(void)Sender;
  int res;
  res = dlgWayPointSelect();
  if (res != -1){
    dlgWaypointEditShowModal(&WayPointList[res]);
    waypointneedsave = true;
  }
}

static void AskWaypointSave(void) {
  if (WaypointsOutOfRange==2) {

    if(MessageBoxX(gettext(TEXT("Waypoints excluded, save anyway?")),
                   gettext(TEXT("Waypoints outside terrain")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      WaypointWriteFiles();

      WAYPOINTFILECHANGED= true;
      changed = true;

    }
  } else {

    WaypointWriteFiles();

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
  res = dlgWayPointSelect();
  if (res != -1){
    if(MessageBoxX(WayPointList[res].Name,
                   gettext(TEXT("Delete Waypoint?")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      LockTaskData();
      WayPointList[res].FileNum = -1;
      UnlockTaskData();
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


extern SCREEN_INFO Data_Options[];
extern int InfoType[];



static void SetInfoBoxSelector(int item, int mode)
{
  TCHAR name[80];
  InfoBoxPropName(name, item, mode);

  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (int i=0; i<NUMSELECTSTRINGS; i++) {
      dfe->addEnumText(gettext(Data_Options[i].Description));
    }
    dfe->Sort(0);

    int it=0;

    switch(mode) {
    case 1: // climb
      it = (InfoType[item])& 0xff;
      break;
    case 0: // cruise
      it = (InfoType[item]>>8)& 0xff;
      break;
    case 2: // final glide
      it = (InfoType[item]>>16)& 0xff;
      break;
    case 3: // aux
      it = (InfoType[item]>>24)& 0xff;
      break;
    };
    dfe->Set(it);
    wp->RefreshDisplay();
  }
}


static void GetInfoBoxSelector(int item, int mode)
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
      it = (InfoType[item])& 0xff;
      break;
    case 0: // cruise
      it = (InfoType[item]>>8)& 0xff;
      break;
    case 2: // final glide
      it = (InfoType[item]>>16)& 0xff;
      break;
    case 3: // aux
      it = (InfoType[item]>>24)& 0xff;
      break;
    };

    if (it != itnew) {

      changed = true;

      switch(mode) {
      case 0: // cruise
	InfoType[item] &= 0xffff00ff;
	InfoType[item] += (itnew<<8);
	break;
      case 1: // climb
	InfoType[item] &= 0xffffff00;
	InfoType[item] += itnew;
	break;
      case 2: // final glide
	InfoType[item] &= 0xff00ffff;
	InfoType[item] += (itnew<<16);
	break;
      case 3: // aux
	InfoType[item] &= 0x00ffffff;
	InfoType[item] += (itnew<<24);
	break;
      };
      StoreType(item,InfoType[item]);
    }
  }
}




//////////

extern const TCHAR *PolarLabels[];

static  TCHAR szPolarFile[MAX_PATH] = TEXT("\0");
static  TCHAR szAirspaceFile[MAX_PATH] = TEXT("\0");
static  TCHAR szAdditionalAirspaceFile[MAX_PATH] = TEXT("\0");
static  TCHAR szWaypointFile[MAX_PATH] = TEXT("\0");
static  TCHAR szAdditionalWaypointFile[MAX_PATH] = TEXT("\0");
static  TCHAR szTerrainFile[MAX_PATH] = TEXT("\0");
static  TCHAR szTopologyFile[MAX_PATH] = TEXT("\0");
static  TCHAR szAirfieldFile[MAX_PATH] = TEXT("\0");
static  TCHAR szLanguageFile[MAX_PATH] = TEXT("\0");
static  TCHAR szStatusFile[MAX_PATH] = TEXT("\0");
static  TCHAR szInputFile[MAX_PATH] = TEXT("\0");
static  TCHAR szMapFile[MAX_PATH] = TEXT("\0");
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

  buttonPilotName = ((WndButton *)wf->FindByName(TEXT("cmdPilotName")));
  if (buttonPilotName) {
    buttonPilotName->SetOnClickNotify(OnPilotNameClicked);
  }
  buttonAircraftType = ((WndButton *)wf->FindByName(TEXT("cmdAircraftType")));
  if (buttonAircraftType) {
    buttonAircraftType->SetOnClickNotify(OnAircraftTypeClicked);
  }
  buttonAircraftRego = ((WndButton *)wf->FindByName(TEXT("cmdAircraftRego")));
  if (buttonAircraftRego) {
    buttonAircraftRego->SetOnClickNotify(OnAircraftRegoClicked);
  }
  buttonLoggerID = ((WndButton *)wf->FindByName(TEXT("cmdLoggerID")));
  if (buttonLoggerID) {
    buttonLoggerID->SetOnClickNotify(OnLoggerIDClicked);
  }

  buttonCopy = ((WndButton *)wf->FindByName(TEXT("cmdCopy")));
  if (buttonCopy) {
    buttonCopy->SetOnClickNotify(OnCopy);
  }
  buttonPaste = ((WndButton *)wf->FindByName(TEXT("cmdPaste")));
  if (buttonPaste) {
    buttonPaste->SetOnClickNotify(OnPaste);
  }

  UpdateButtons();

  //////////
  /*
  config_page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

  return;
  */

  ///////////////////

  wp = (WndProperty*)wf->FindByName(TEXT("prpUserLevel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Basic")));
    dfe->addEnumText(gettext(TEXT("Expert")));
    dfe->Set(UserLevel);
    wp->RefreshDisplay();
  }

  ///////////////////

    const TCHAR *COMMPort[] = {TEXT("COM1"),TEXT("COM2"),TEXT("COM3"),TEXT("COM4"),
			       TEXT("COM5"),TEXT("COM6"),TEXT("COM7"),TEXT("COM8"),
			       TEXT("COM9"),TEXT("COM10"),TEXT("COM0")};
    const TCHAR *tSpeed[] = {TEXT("1200"),TEXT("2400"),TEXT("4800"),TEXT("9600"),
			     TEXT("19200"),TEXT("38400"),TEXT("57600"),TEXT("115200")};
//  DWORD dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};

  int i;

  ReadPort1Settings(&dwPortIndex1,&dwSpeedIndex1);

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<11; i++) {
      dfe->addEnumText((COMMPort[i]));
    }
    dfe->Set(dwPortIndex1);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<8; i++) {
      dfe->addEnumText((tSpeed[i]));
    }
    dfe->Set(dwSpeedIndex1);
    wp->RefreshDisplay();
  }

#ifdef _SIM_
  TCHAR deviceName1[MAX_PATH];
  TCHAR deviceName2[MAX_PATH];
  ReadDeviceSettings(0, deviceName1);
  ReadDeviceSettings(1, deviceName2);
#endif

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<DeviceRegisterCount; i++) {
      devRegisterGetName(i, DeviceName);
      dfe->addEnumText((DeviceName));
#ifndef _SIM_
      if (devA() != NULL){
	if (_tcscmp(DeviceName, devA()->Name) == 0)
	  dwDeviceIndex1 = i;
      }
#else
      if (_tcscmp(DeviceName, deviceName1) == 0)
        dwDeviceIndex1 = i;
#endif
    }
    dfe->Sort(1);
    dfe->Set(dwDeviceIndex1);
    wp->RefreshDisplay();
  }

  ReadPort2Settings(&dwPortIndex2,&dwSpeedIndex2);

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<11; i++) {
      dfe->addEnumText((COMMPort[i]));
    }
    dfe->Set(dwPortIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<8; i++) {
      dfe->addEnumText((tSpeed[i]));
    }
    dfe->Set(dwSpeedIndex2);
    wp->RefreshDisplay();
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<DeviceRegisterCount; i++) {
      devRegisterGetName(i, DeviceName);
      dfe->addEnumText((DeviceName));
#ifndef _SIM_
      if (devB() != NULL){
	if (_tcscmp(DeviceName, devB()->Name) == 0)
	  dwDeviceIndex2 = i;
      }
#else
      if (_tcscmp(DeviceName, deviceName2) == 0)
        dwDeviceIndex2 = i;
#endif
    }
    dfe->Sort(1);
    dfe->Set(dwDeviceIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("All on")));
    dfe->addEnumText(gettext(TEXT("Clip")));
    dfe->addEnumText(gettext(TEXT("Auto")));
    dfe->addEnumText(gettext(TEXT("All below")));
    dfe->Set(AltitudeMode);
    wp->RefreshDisplay();
  }
  //

  wp = (WndProperty*)wf->FindByName(TEXT("prpUTCOffset"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(UTCOffset/1800.0)/2.0);
    wp->RefreshDisplay();
  }
  SetLocalTime();

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ClipAltitude*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltWarningMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(AltWarningMargin*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoZoom"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::AutoZoom);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceOutline"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::bAirspaceBlackOutline);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLockSettingsInFlight"));
  if (wp) {
    wp->GetDataField()->Set(LockSettingsInFlight);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerShortName"));
  if (wp) {
    wp->GetDataField()->Set(LoggerShortName);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDebounceTimeout"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(debounceTimeout);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMMap"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("OFF")));
    dfe->addEnumText(gettext(TEXT("ON/Fixed")));
    dfe->addEnumText(gettext(TEXT("ON/Scaled")));
    dfe->Set(EnableFLARMMap);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMGauge"));
  if (wp) {
    wp->GetDataField()->Set(EnableFLARMGauge);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceWarnings"));
  if (wp) {
    bool aw = AIRSPACEWARNINGS != 0;
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWarningTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(WarningTime);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAcknowledgementTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(AcknowledgementTime);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointLabels"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Names")));
    dfe->addEnumText(gettext(TEXT("Numbers")));
    dfe->addEnumText(gettext(TEXT("First 5")));
    dfe->addEnumText(gettext(TEXT("None")));
    dfe->addEnumText(gettext(TEXT("First 3")));
    dfe->addEnumText(gettext(TEXT("Names in task")));
    dfe->Set(DisplayTextType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTerrain"));
  if (wp) {
    wp->GetDataField()->Set(EnableTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTopology"));
  if (wp) {
    wp->GetDataField()->Set(EnableTopology);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpCirclingZoom"));
  if (wp) {
    wp->GetDataField()->Set(CircleZoom);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOrientation"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Track up")));
    dfe->addEnumText(gettext(TEXT("North up")));
    dfe->addEnumText(gettext(TEXT("North circling")));
    dfe->addEnumText(gettext(TEXT("Target circling")));
    dfe->addEnumText(gettext(TEXT("North/track")));
    dfe->Set(DisplayOrientation);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMenuTimeout"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(MenuTimeoutMax/2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeArrival"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*SAFETYALTITUDEARRIVAL));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeBreakoff"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*SAFETYALTITUDEBREAKOFF));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeTerrain"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*SAFETYALTITUDETERRAIN));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinalGlideTerrain"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("OFF")));
    dfe->addEnumText(gettext(TEXT("Line")));
    dfe->addEnumText(gettext(TEXT("Shade")));
    dfe->Set(FinalGlideTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableNavBaroAltitude"));
  if (wp) {
    wp->GetDataField()->Set(EnableNavBaroAltitude);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWindArrowStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Arrow head")));
    dfe->addEnumText(gettext(TEXT("Full arrow")));
    wp->GetDataField()->Set(MapWindow::WindArrowStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Manual")));
    dfe->addEnumText(gettext(TEXT("Circling")));
    dfe->addEnumText(gettext(TEXT("ZigZag")));
    dfe->addEnumText(gettext(TEXT("Both")));
    wp->GetDataField()->Set(AutoWindMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoMcMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Final glide")));
    dfe->addEnumText(gettext(TEXT("Average climb")));
    dfe->addEnumText(gettext(TEXT("Both")));
    wp->GetDataField()->Set(AutoMcMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointsOutOfRange"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Ask")));
    dfe->addEnumText(gettext(TEXT("Include")));
    dfe->addEnumText(gettext(TEXT("Exclude")));
    wp->GetDataField()->Set(WaypointsOutOfRange);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoForceFinalGlide"));
  if (wp) {
    wp->GetDataField()->Set(AutoForceFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBlockSTF"));
  if (wp) {
    wp->GetDataField()->Set(EnableBlockSTF);
    wp->RefreshDisplay();
  }

  ////

  wp = (WndProperty*)wf->FindByName(TEXT("prpFAIFinishHeight"));
  if (wp) {
    wp->GetDataField()->Set(EnableFAIFinishHeight);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOLCRules"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Sprint")));
    dfe->addEnumText(gettext(TEXT("Triangle")));
    dfe->addEnumText(gettext(TEXT("Classic")));
    dfe->Set(OLCRules);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpHandicap"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(Handicap);
    wp->RefreshDisplay();
  }

  ////

  if(GetFromRegistry(szRegistrySpeedUnitsValue,&Speed)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistrySpeedUnitsValue, Speed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Statue")));
    dfe->addEnumText(gettext(TEXT("Nautical")));
    dfe->addEnumText(gettext(TEXT("Metric")));
    dfe->Set(Speed);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLatLon"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("DDMMSS"));
    dfe->addEnumText(TEXT("DDMMSS.ss"));
    dfe->addEnumText(TEXT("DDMM.mmm"));
    dfe->addEnumText(TEXT("DD.dddd"));
    dfe->Set(Units::CoordinateFormat);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryTaskSpeedUnitsValue,&TaskSpeed)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryTaskSpeedUnitsValue, TaskSpeed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsTaskSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Statute")));
    dfe->addEnumText(gettext(TEXT("Nautical")));
    dfe->addEnumText(gettext(TEXT("Metric")));
    dfe->Set(TaskSpeed);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryDistanceUnitsValue,&Distance)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryDistanceUnitsValue, Distance);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsDistance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Statute")));
    dfe->addEnumText(gettext(TEXT("Nautical")));
    dfe->addEnumText(gettext(TEXT("Metric")));
    dfe->Set(Distance);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryAltitudeUnitsValue,&Altitude)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsAltitude"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Feet")));
    dfe->addEnumText(gettext(TEXT("Meters")));
    dfe->Set(Altitude);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryLiftUnitsValue,&Lift)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryLiftUnitsValue, Lift);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLift"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Knots")));
    dfe->addEnumText(gettext(TEXT("M/s")));
    dfe->Set(Lift);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::EnableTrailDrift);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpThermalLocator"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("OFF")));
    dfe->addEnumText(gettext(TEXT("Circle at center")));
    dfe->addEnumText(gettext(TEXT("Pan to center")));
    dfe->Set(EnableThermalLocator);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSetSystemTimeFromGPS"));
  if (wp) {
    wp->GetDataField()->Set(SetSystemTimeFromGPS);
    wp->RefreshDisplay();
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpAbortSafetyUseCurrent"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::AbortSafetyUseCurrent);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDisableAutoLogger"));
  if (wp) {
    wp->GetDataField()->Set(!DisableAutoLogger);
    wp->RefreshDisplay();
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyMacCready"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::SafetyMacCready*LIFTMODIFY);
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRiskGamma"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::RiskGamma);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAnimation"));
  if (wp) {
    wp->GetDataField()->Set(EnableAnimation);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrail"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Off")));
    dfe->addEnumText(gettext(TEXT("Long")));
    dfe->addEnumText(gettext(TEXT("Short")));
    dfe->addEnumText(gettext(TEXT("Full")));
    dfe->Set(TrailActive);
    wp->RefreshDisplay();
  }

  // VENTA3 VisualGlide
  wp = (WndProperty*)wf->FindByName(TEXT("prpVGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Off")));
    dfe->addEnumText(gettext(TEXT("Steady")));
    dfe->addEnumText(gettext(TEXT("Moving")));
    dfe->Set(VisualGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxManoeuveringSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(SPEEDMODIFY*SAFTEYSPEED));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastSecsToEmpty"));
  if (wp) {
    wp->GetDataField()->Set(BallastSecsToEmpty);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<NUMPOLARS; i++) {
      dfe->addEnumText(PolarLabels[i]);
    }
    i=0;
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
  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.plr"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryAirspaceFile, szAirspaceFile, MAX_PATH);
  _tcscpy(temptext,szAirspaceFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryAdditionalAirspaceFile,
		    szAdditionalAirspaceFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalAirspaceFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  _tcscpy(temptext,szWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->ScanDirectoryTop(TEXT("*.xcw"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryAdditionalWayPointFile,
		    szAdditionalWaypointFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->ScanDirectoryTop(TEXT("*.xcw"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
  _tcscpy(temptext,szMapFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpMapFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcm"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryTerrainFile, szTerrainFile, MAX_PATH);
  _tcscpy(temptext,szTerrainFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->ScanDirectoryTop(TEXT("*.jp2"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryTopologyFile, szTopologyFile, MAX_PATH);
  _tcscpy(temptext,szTopologyFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.tpl"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryAirfieldFile, szAirfieldFile, MAX_PATH);
  _tcscpy(temptext,szAirfieldFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryLanguageFile, szLanguageFile, MAX_PATH);
  _tcscpy(temptext,szLanguageFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcl"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryStatusFile, szStatusFile, MAX_PATH);
  _tcscpy(temptext,szStatusFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcs"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  GetRegistryString(szRegistryInputFile, szInputFile, MAX_PATH);
  _tcscpy(temptext,szInputFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xci"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppStatusMessageAlignment"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Center")));
    dfe->addEnumText(gettext(TEXT("Topleft")));
    dfe->Set(Appearance.StateMessageAlligne);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTextInput"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("HighScore Style")));
    dfe->addEnumText(gettext(TEXT("Keyboard")));
	dfe->Set(Appearance.TextInputStyle);
    wp->RefreshDisplay();
  }



  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxBorder"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Box")));
    dfe->addEnumText(gettext(TEXT("Tab")));
    dfe->Set(Appearance.InfoBoxBorder);
    wp->RefreshDisplay();
  }

#if defined(PNA) || defined(FIVV)
// VENTA-ADDON Geometry change config menu 11
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxGeom"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();

    if (InfoBoxLayout::landscape) {


	dfe->addEnumText(gettext(TEXT("vario+9box"))); // 0
	dfe->addEnumText(gettext(TEXT("(empty) A")));  // 1
	dfe->addEnumText(gettext(TEXT("(empty) B")));  // 2
	dfe->addEnumText(gettext(TEXT("(empty) C")));  // 3
	dfe->addEnumText(gettext(TEXT("8box left")));  // 4
	dfe->addEnumText(gettext(TEXT("8box right"))); // 5 VENTA3
	dfe->addEnumText(gettext(TEXT("(empty) D")));  // 6
	dfe->addEnumText(gettext(TEXT("5box right"))); // 7
	dfe->Set(Appearance.InfoBoxGeom);
	wp->RefreshDisplay();

    } else {
// VENTA2 FIX portrait mode selection geometry
	dfe->addEnumText(gettext(TEXT("top+bottom"))); // 0
	dfe->addEnumText(gettext(TEXT("bottom")));     // 1
	dfe->addEnumText(gettext(TEXT("top")));        // 2
	dfe->addEnumText(gettext(TEXT("3 free"))); // 3
	dfe->addEnumText(gettext(TEXT("4 free")));       // 4
	dfe->addEnumText(gettext(TEXT("5 free")));      // 5
	dfe->addEnumText(gettext(TEXT("6 free")));      // 6

	//dfe->addEnumText(gettext(TEXT("left+right"))); // 3
	//dfe->addEnumText(gettext(TEXT("left")));       // 4
	//dfe->addEnumText(gettext(TEXT("right")));      // 5
	//dfe->addEnumText(gettext(TEXT("vario")));      // 6
	dfe->addEnumText(gettext(TEXT("7")));          // 7
	dfe->Set(Appearance.InfoBoxGeom);
	wp->RefreshDisplay();

    }
  }
//
#endif

#ifdef PNA
// VENTA-ADDON Model change config menu 11
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxModel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Generic")));
    dfe->addEnumText(gettext(TEXT("HP31x")));
    dfe->addEnumText(gettext(TEXT("MedionP5")));
    dfe->addEnumText(gettext(TEXT("MIO")));
    dfe->addEnumText(gettext(TEXT("Nokia500"))); // VENTA3
    dfe->addEnumText(gettext(TEXT("PN6000")));

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
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxModel"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
// VENTA2- PC model
#if (WINDOWSPC>0)
    dfe->addEnumText(gettext(TEXT("PC/normal")));
    #ifdef FIVV
   	 wp->SetVisible(true); // no more gaps in menus
    #else
    wp->SetVisible(false); // currently no need to display default
    #endif
#else
    dfe->addEnumText(gettext(TEXT("PDA/normal")));
    #ifdef FIVV
    wp->SetVisible(true);
    #else
    wp->SetVisible(false);
    #endif
#endif
        dfe->Set(0);
    wp->RefreshDisplay();
  }
#endif

  wp = (WndProperty*)wf->FindByName(TEXT("prpExtendedVisualGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Normal")));
    dfe->addEnumText(gettext(TEXT("Extended")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(ExtendedVisualGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVirtualKeys"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Disabled")));
    dfe->addEnumText(gettext(TEXT("Enabled")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(VirtualKeys);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAverEffTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("15 seconds")));
    dfe->addEnumText(gettext(TEXT("30 seconds")));
    dfe->addEnumText(gettext(TEXT("60 seconds")));
    dfe->addEnumText(gettext(TEXT("90 seconds")));
    dfe->addEnumText(gettext(TEXT("2 minutes")));
    dfe->addEnumText(gettext(TEXT("3 minutes")));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(AverEffTime);
    wp->RefreshDisplay();
  }

// Fonts
  wp = (WndProperty*)wf->FindByName(TEXT("prpUseCustomFonts"));
  if (wp) {
    DataFieldBoolean * dfb = (DataFieldBoolean*) wp->GetDataField();
    dfb->Set(UseCustomFonts);
    ShowFontEditButtons(dfb->GetAsBoolean());
    wp->RefreshDisplay();
    RefreshFonts();
  }
  FontRegistryChanged=false;


// end fonts

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppCompassAppearance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Normal")));
    dfe->addEnumText(gettext(TEXT("White outline")));
    dfe->Set(Appearance.CompassAppearance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndFinalGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Default")));
    dfe->addEnumText(gettext(TEXT("Alternate")));
    dfe->Set(Appearance.IndFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Winpilot")));
    dfe->addEnumText(gettext(TEXT("Alternate")));
    dfe->Set(Appearance.IndLandable);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableExternalTriggerCruise"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("OFF")));
    dfe->addEnumText(gettext(TEXT("Flap")));
    dfe->addEnumText(gettext(TEXT("SC")));
    dfe->Set(EnableExternalTriggerCruise);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInverseInfoBox"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.InverseInfoBox);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppDefaultMapWidth"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Appearance.DefaultMapWidth);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpGliderScreenPosition"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(MapWindow::GliderScreenPosition);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainContrast"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(TerrainContrast*100/255));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainBrightness"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(TerrainBrightness*100/255));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainRamp"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Low lands")));
    dfe->addEnumText(gettext(TEXT("Mountainous")));
    dfe->addEnumText(gettext(TEXT("Imhof 7")));
    dfe->addEnumText(gettext(TEXT("Imhof 4")));
    dfe->addEnumText(gettext(TEXT("Imhof 12")));
    dfe->addEnumText(gettext(TEXT("Imhof Atlas")));
    dfe->addEnumText(gettext(TEXT("ICAO")));
    dfe->Set(TerrainRamp);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxColors"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.InfoBoxColors);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppAveNeedle"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioAveNeedle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioSpeedToFly);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioAvgText"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioAvgText);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioGross"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioGross);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioMc"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioMc);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBugs"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioBugs);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBallast"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioBallast);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoBlank"));
  if (wp) {
#ifdef GNAV
    wp->SetVisible(false);
#endif
#if (WINDOWSPC>0)
    wp->SetVisible(false);
#endif
    wp->GetDataField()->Set(EnableAutoBlank);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoBacklight")); // VENTA4
  if (wp) {
    wp->SetVisible(false);
#ifdef PNA
    if (GlobalModelType == MODELTYPE_PNA_HP31X )
    	wp->SetVisible(true);
#endif
    wp->GetDataField()->Set(EnableAutoBacklight);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoSoundVolume")); // VENTA4
  if (wp) {
    wp->SetVisible(false);
#if ( !defined(WINDOWSPC) || WINDOWSPC==0 )
    	wp->SetVisible(true);
#endif
    wp->GetDataField()->Set(EnableAutoSoundVolume);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Cylinder")));
    dfe->addEnumText(gettext(TEXT("Line")));
    dfe->addEnumText(gettext(TEXT("FAI Sector")));
    dfe->Set(FinishLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(FinishRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Cylinder")));
    dfe->addEnumText(gettext(TEXT("Line")));
    dfe->addEnumText(gettext(TEXT("FAI Sector")));
    dfe->Set(StartLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(StartRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Cylinder")));
    dfe->addEnumText(gettext(TEXT("FAI Sector")));
    dfe->addEnumText(gettext(TEXT("DAe 0.5/10")));
    dfe->Set(SectorType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(SectorRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("Manual")));
    dfe->addEnumText(gettext(TEXT("Auto")));
    dfe->addEnumText(gettext(TEXT("Arm")));
    dfe->addEnumText(gettext(TEXT("Arm start")));
    dfe->Set(AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinishMinHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(FinishMinHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(StartMaxHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxHeightMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(StartMaxHeightMargin*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartHeightRef"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(TEXT("AGL")));
    dfe->addEnumText(gettext(TEXT("MSL")));
    dfe->Set(StartHeightRef);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(StartMaxSpeed*SPEEDMODIFY));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxSpeedMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(StartMaxSpeedMargin*SPEEDMODIFY));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCruise"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(LoggerTimeStepCruise);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCircling"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(LoggerTimeStepCircling);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSnailWidthScale"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(MapWindow::SnailWidthScale);
    wp->RefreshDisplay();
  }

#ifdef FIVV
  wp = (WndProperty*)wf->FindByName(TEXT("prpGPSAltitudeOffset")); // VENTA3 GPSAltitudeOffset
  if (wp) {
    wp->GetDataField()->SetAsFloat(GPSAltitudeOffset);
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }
#endif


  ////
  for (i=0; i<4; i++) {
    for (int j=0; j<numInfoWindows; j++) {
      SetInfoBoxSelector(j, i);
    }
  }
}




void dlgConfigurationShowModal(void){

  WndProperty *wp;

  StartHourglassCursor();

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgConfiguration_L.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_CONFIGURATION_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgConfiguration.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_CONFIGURATION"));
  }

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wConfig1    = ((WndFrame *)wf->FindByName(TEXT("frmSite")));
  wConfig2    = ((WndFrame *)wf->FindByName(TEXT("frmAirspace")));
  wConfig3    = ((WndFrame *)wf->FindByName(TEXT("frmDisplay")));
  wConfig4    = ((WndFrame *)wf->FindByName(TEXT("frmTerrain")));
  wConfig5    = ((WndFrame *)wf->FindByName(TEXT("frmFinalGlide")));
  wConfig6    = ((WndFrame *)wf->FindByName(TEXT("frmSafety")));
  wConfig7    = ((WndFrame *)wf->FindByName(TEXT("frmPolar")));
  wConfig8    = ((WndFrame *)wf->FindByName(TEXT("frmComm")));
  wConfig9    = ((WndFrame *)wf->FindByName(TEXT("frmUnits")));
  wConfig10    = ((WndFrame *)wf->FindByName(TEXT("frmInterface")));
  wConfig11    = ((WndFrame *)wf->FindByName(TEXT("frmAppearance")));
  wConfig12    = ((WndFrame *)wf->FindByName(TEXT("frmFonts")));
  wConfig13    = ((WndFrame *)wf->FindByName(TEXT("frmVarioAppearance")));
  wConfig14    = ((WndFrame *)wf->FindByName(TEXT("frmTask")));
  wConfig15    = ((WndFrame *)wf->FindByName(TEXT("frmTaskRules")));
  wConfig16    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCircling")));
  wConfig17    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCruise")));
  wConfig18    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxFinalGlide")));
  wConfig19    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxAuxiliary")));
  wConfig20    = ((WndFrame *)wf->FindByName(TEXT("frmLogger")));
  wConfig21    = ((WndFrame *)wf->FindByName(TEXT("frmWaypointEdit")));
  wConfig22    = ((WndFrame *)wf->FindByName(TEXT("frmSpecials")));

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

  wf->FilterAdvanced(UserLevel>0);

// VENTA2- FIVV special version
#if !defined(PNA) && !defined(FIVV)
  // JMW we don't want these for non-PDA platforms yet
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxGeom"));
  if (wp) {
    wp->SetVisible(false);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxModel"));
  if (wp) {
    wp->SetVisible(false);
  }
#endif

  for (int item=0; item<10; item++) {
    cpyInfoBox[item] = -1;
  }

  setVariables();

  UpdateDeviceSetupButton(0, devA()->Name);
  UpdateDeviceSetupButton(1, devB()->Name);

  NextPage(0); // just to turn proper pages on/off

  changed = false;
  taskchanged = false;
  requirerestart = false;
  utcchanged = false;
  waypointneedsave = false;

  StopHourglassCursor();
  wf->ShowModal();

  // TODO enhancement: implement a cancel button that skips all this
  // below after exit.

  wp = (WndProperty*)wf->FindByName(TEXT("prpAbortSafetyUseCurrent"));
  if (wp) {
    if (GlidePolar::AbortSafetyUseCurrent
	!= wp->GetDataField()->GetAsBoolean()) {
      GlidePolar::AbortSafetyUseCurrent =
	wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAbortSafetyUseCurrent,
		    GlidePolar::AbortSafetyUseCurrent);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDisableAutoLogger"));
  if (wp) {
    if (!DisableAutoLogger
	!= wp->GetDataField()->GetAsBoolean()) {
      DisableAutoLogger =
	!(wp->GetDataField()->GetAsBoolean());
      SetToRegistry(szRegistryDisableAutoLogger,
		    DisableAutoLogger);
      changed = true;
    }
  }

  double val;

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyMacCready"));
  if (wp) {
    val = wp->GetDataField()->GetAsFloat()/LIFTMODIFY;
    if (GlidePolar::SafetyMacCready != val) {
      GlidePolar::SafetyMacCready = val;
      SetToRegistry(szRegistrySafetyMacCready,
		    iround(GlidePolar::SafetyMacCready*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRiskGamma"));
  if (wp) {
    val = wp->GetDataField()->GetAsFloat();
    if (GlidePolar::RiskGamma != val) {
      GlidePolar::RiskGamma = val;
      SetToRegistry(szRegistryRiskGamma,
		    iround(GlidePolar::RiskGamma*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSetSystemTimeFromGPS"));
  if (wp) {
    if (SetSystemTimeFromGPS != wp->GetDataField()->GetAsBoolean()) {
      SetSystemTimeFromGPS = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistrySetSystemTimeFromGPS, SetSystemTimeFromGPS);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAnimation"));
  if (wp) {
    if (EnableAnimation != wp->GetDataField()->GetAsBoolean()) {
      EnableAnimation = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAnimation, EnableAnimation);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
  if (wp) {
    if (MapWindow::EnableTrailDrift != wp->GetDataField()->GetAsBoolean()) {
      MapWindow::EnableTrailDrift = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryTrailDrift, MapWindow::EnableTrailDrift);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpThermalLocator"));
  if (wp) {
    if (EnableThermalLocator != wp->GetDataField()->GetAsInteger()) {
      EnableThermalLocator = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryThermalLocator, EnableThermalLocator);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrail"));
  if (wp) {
    if (TrailActive != wp->GetDataField()->GetAsInteger()) {
      TrailActive = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySnailTrail, TrailActive);
      changed = true;
    }
  }

// VENTA3: do not save VisualGlide to registry or profile

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarType"));
  if (wp) {
    if (POLARID != wp->GetDataField()->GetAsInteger()) {
      POLARID = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryPolarID, POLARID);
      GlidePolar::SetBallast();
      POLARFILECHANGED = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceDisplay"));
  if (wp) {
    if (AltitudeMode != wp->GetDataField()->GetAsInteger()) {
      AltitudeMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltMode, AltitudeMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLockSettingsInFlight"));
  if (wp) {
    if (LockSettingsInFlight !=
	wp->GetDataField()->GetAsBoolean()) {
      LockSettingsInFlight = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryLockSettingsInFlight,
		    LockSettingsInFlight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerShortName"));
  if (wp) {
    if (LoggerShortName !=
	wp->GetDataField()->GetAsBoolean()) {
      LoggerShortName = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryLoggerShort,
		    LoggerShortName);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMMap"));
  if (wp) {
    if ((int)EnableFLARMMap !=
	wp->GetDataField()->GetAsInteger()) {
      EnableFLARMMap = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryEnableFLARMMap,
		    EnableFLARMMap);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMGauge"));
  if (wp) {
    if (EnableFLARMGauge !=
	wp->GetDataField()->GetAsBoolean()) {
      EnableFLARMGauge = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryEnableFLARMGauge,
		    EnableFLARMGauge);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDebounceTimeout"));
  if (wp) {
    if (debounceTimeout != wp->GetDataField()->GetAsInteger()) {
      debounceTimeout = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDebounceTimeout, debounceTimeout);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceOutline"));
  if (wp) {
    if (MapWindow::bAirspaceBlackOutline !=
	wp->GetDataField()->GetAsBoolean()) {
      MapWindow::bAirspaceBlackOutline = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAirspaceBlackOutline,
		    MapWindow::bAirspaceBlackOutline);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoZoom"));
  if (wp) {
    if (MapWindow::AutoZoom !=
	wp->GetDataField()->GetAsBoolean()) {
      MapWindow::AutoZoom = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAutoZoom,
		    MapWindow::AutoZoom);
      changed = true;
    }
  }

  int ival;

  wp = (WndProperty*)wf->FindByName(TEXT("prpUTCOffset"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()*3600.0);
    if ((UTCOffset != ival)||(utcchanged)) {
      UTCOffset = ival;

      // have to do this because registry variables can't be negative!
      int lival = UTCOffset;
      if (lival<0) { lival+= 24*3600; }
      SetToRegistry(szRegistryUTCOffset, lival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (ClipAltitude != ival) {
      ClipAltitude = ival;
      SetToRegistry(szRegistryClipAlt,ClipAltitude);  // fixed 20060430/sgi was szRegistryAltMode
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltWarningMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (AltWarningMargin != ival) {
      AltWarningMargin = ival;
      SetToRegistry(szRegistryAltMargin,AltWarningMargin);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceWarnings"));
  if (wp) {
    if (AIRSPACEWARNINGS != wp->GetDataField()->GetAsInteger()) {
      AIRSPACEWARNINGS = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAirspaceWarning,(DWORD)AIRSPACEWARNINGS);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWarningTime"));
  if (wp) {
    if (WarningTime != wp->GetDataField()->GetAsInteger()) {
      WarningTime = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryWarningTime,(DWORD)WarningTime);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAcknowledgementTime"));
  if (wp) {
    if (AcknowledgementTime != wp->GetDataField()->GetAsInteger()) {
      AcknowledgementTime = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAcknowledgementTime,
		    (DWORD)AcknowledgementTime);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointLabels"));
  if (wp) {
    if (DisplayTextType != wp->GetDataField()->GetAsInteger()) {
      DisplayTextType = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDisplayText,DisplayTextType);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTerrain"));
  if (wp) {
    if (EnableTerrain != wp->GetDataField()->GetAsBoolean()) {
      EnableTerrain = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryDrawTerrain, EnableTerrain);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTopology"));
  if (wp) {
    if (EnableTopology != wp->GetDataField()->GetAsBoolean()) {
      EnableTopology = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryDrawTopology, EnableTopology);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpCirclingZoom"));
  if (wp) {
    if (CircleZoom != wp->GetDataField()->GetAsBoolean()) {
      CircleZoom = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryCircleZoom, CircleZoom);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOrientation"));
  if (wp) {
    if (DisplayOrientation != wp->GetDataField()->GetAsInteger()) {
      DisplayOrientation = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDisplayUpValue,DisplayOrientation);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMenuTimeout"));
  if (wp) {
    if (MenuTimeoutMax != wp->GetDataField()->GetAsInteger()*2) {
      MenuTimeoutMax = wp->GetDataField()->GetAsInteger()*2;
      SetToRegistry(szRegistryMenuTimeout,MenuTimeoutMax);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeArrival"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (SAFETYALTITUDEARRIVAL != ival) {
      SAFETYALTITUDEARRIVAL = ival;
      SetToRegistry(szRegistrySafetyAltitudeArrival,
		    (DWORD)SAFETYALTITUDEARRIVAL);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeBreakoff"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (SAFETYALTITUDEBREAKOFF != ival) {
      SAFETYALTITUDEBREAKOFF = ival;
      SetToRegistry(szRegistrySafetyAltitudeBreakOff,
		    (DWORD)SAFETYALTITUDEBREAKOFF);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeTerrain"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (SAFETYALTITUDETERRAIN != ival) {
      SAFETYALTITUDETERRAIN = ival;
      SetToRegistry(szRegistrySafetyAltitudeTerrain,
		    (DWORD)SAFETYALTITUDETERRAIN);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
  if (wp) {
    if (AutoWindMode != wp->GetDataField()->GetAsInteger()) {
      AutoWindMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAutoWind, AutoWindMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWindArrowStyle"));
  if (wp) {
    if (MapWindow::WindArrowStyle != wp->GetDataField()->GetAsInteger()) {
      MapWindow::WindArrowStyle = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryWindArrowStyle, MapWindow::WindArrowStyle);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoMcMode"));
  if (wp) {
    if (AutoMcMode != wp->GetDataField()->GetAsInteger()) {
      AutoMcMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAutoMcMode, AutoMcMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointsOutOfRange"));
  if (wp) {
    if (WaypointsOutOfRange != wp->GetDataField()->GetAsInteger()) {
      WaypointsOutOfRange = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryWaypointsOutOfRange, WaypointsOutOfRange);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoForceFinalGlide"));
  if (wp) {
    if (AutoForceFinalGlide != wp->GetDataField()->GetAsBoolean()) {
      AutoForceFinalGlide = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAutoForceFinalGlide, AutoForceFinalGlide);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableNavBaroAltitude"));
  if (wp) {
    if (EnableNavBaroAltitude != wp->GetDataField()->GetAsBoolean()) {
      EnableNavBaroAltitude = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryEnableNavBaroAltitude, EnableNavBaroAltitude);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinalGlideTerrain"));
  if (wp) {
    if (FinalGlideTerrain != wp->GetDataField()->GetAsInteger()) {
      FinalGlideTerrain = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFinalGlideTerrain, FinalGlideTerrain);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBlockSTF"));
  if (wp) {
    if (EnableBlockSTF != wp->GetDataField()->GetAsBoolean()) {
      EnableBlockSTF = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryBlockSTF, EnableBlockSTF);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsSpeed"));
  if (wp) {
    if ((int)Speed != wp->GetDataField()->GetAsInteger()) {
      Speed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySpeedUnitsValue, Speed);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLatLon"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsTaskSpeed"));
  if (wp) {
    if ((int)TaskSpeed != wp->GetDataField()->GetAsInteger()) {
      TaskSpeed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryTaskSpeedUnitsValue, TaskSpeed);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsDistance"));
  if (wp) {
    if ((int)Distance != wp->GetDataField()->GetAsInteger()) {
      Distance = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDistanceUnitsValue, Distance);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLift"));
  if (wp) {
    if ((int)Lift != wp->GetDataField()->GetAsInteger()) {
      Lift = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryLiftUnitsValue, Lift);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsAltitude"));
  if (wp) {
    if ((int)Altitude != wp->GetDataField()->GetAsInteger()) {
      Altitude = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
      Units::NotifyUnitChanged();
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFAIFinishHeight"));
  if (wp) {
    if (EnableFAIFinishHeight != (wp->GetDataField()->GetAsInteger()>0)) {
      EnableFAIFinishHeight = (wp->GetDataField()->GetAsInteger()>0);
      SetToRegistry(szRegistryFAIFinishHeight, EnableFAIFinishHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOLCRules"));
  if (wp) {
    if (OLCRules != wp->GetDataField()->GetAsInteger()) {
      OLCRules = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryOLCRules, OLCRules);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpHandicap"));
  if (wp) {
    int val  = wp->GetDataField()->GetAsInteger();
    if (Handicap != val) {
      Handicap = val;
      SetToRegistry(szRegistryHandicap, Handicap);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szPolarFile)) {
      SetRegistryString(szRegistryPolarFile, temptext);
      POLARFILECHANGED = true;
      GlidePolar::SetBallast();
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalWaypointFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalAirspaceFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpMapFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpTopologyFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirfieldFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpLanguageFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpStatusFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpInputFile"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastSecsToEmpty"));
  if (wp) {
    ival = wp->GetDataField()->GetAsInteger();
    if (BallastSecsToEmpty != ival) {
      BallastSecsToEmpty = ival;
      SetToRegistry(szRegistryBallastSecsToEmpty,(DWORD)BallastSecsToEmpty);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxManoeuveringSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if (SAFTEYSPEED != ival) {
      SAFTEYSPEED = ival;
      SetToRegistry(szRegistrySafteySpeed,(DWORD)SAFTEYSPEED);
      GlidePolar::SetBallast();
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    if (FinishLine != wp->GetDataField()->GetAsInteger()) {
      FinishLine = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFinishLine,FinishLine);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)FinishRadius != ival) {
      FinishRadius = ival;
      SetToRegistry(szRegistryFinishRadius,FinishRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    if (StartLine != wp->GetDataField()->GetAsInteger()) {
      StartLine = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryStartLine,StartLine);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)StartRadius != ival) {
      StartRadius = ival;
      SetToRegistry(szRegistryStartRadius,StartRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    if ((int)SectorType != wp->GetDataField()->GetAsInteger()) {
      SectorType = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFAISector,SectorType);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)SectorRadius != ival) {
      SectorRadius = ival;
      SetToRegistry(szRegistrySectorRadius,SectorRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndFinalGlide"));
  if (wp) {
    if (Appearance.IndFinalGlide != (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndFinalGlide = (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndFinalGlide,(DWORD)(Appearance.IndFinalGlide));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppCompassAppearance"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxBorder"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpExtendedVisualGlide")); // VENTA4
  if (wp) {
    if (ExtendedVisualGlide != (ExtendedVisualGlide_t)
	(wp->GetDataField()->GetAsInteger())) {
      ExtendedVisualGlide = (ExtendedVisualGlide_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryExtendedVisualGlide,
		    (DWORD)(ExtendedVisualGlide));
      changed = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpVirtualKeys")); // VENTA6
  if (wp) {
    if (VirtualKeys != (VirtualKeys_t)
	(wp->GetDataField()->GetAsInteger())) {
      VirtualKeys = (VirtualKeys_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryVirtualKeys,
		    (DWORD)(VirtualKeys));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAverEffTime")); // VENTA6
  if (wp) {
    if (AverEffTime !=
	(wp->GetDataField()->GetAsInteger())) {
      AverEffTime =
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAverEffTime,
		    (DWORD)(AverEffTime));
      changed = true;
      InitLDRotary(&rotaryLD);
    }
  }


#if defined(PNA) || defined(FIVV)
// VENTA-ADDON GEOM
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxGeom"));
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
  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxModel"));
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
  wp = (WndProperty*)wf->FindByName(TEXT("prpUseCustomFonts"));
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
  DeleteObject(TempUseCustomFontsFont);

  DeleteObject (TempInfoWindowFont);
  DeleteObject (TempTitleWindowFont);
  DeleteObject (TempMapWindowFont);
  DeleteObject (TempTitleSmallWindowFont);
  DeleteObject (TempMapWindowBoldFont);
  DeleteObject (TempCDIWindowFont);
  DeleteObject (TempMapLabelFont);
  DeleteObject (TempStatisticsFont);


  wp = (WndProperty*)wf->FindByName(TEXT("prpAppStatusMessageAlignment"));
  if (wp) {
    if (Appearance.StateMessageAlligne != (StateMessageAlligne_t)
	(wp->GetDataField()->GetAsInteger())) {
      Appearance.StateMessageAlligne = (StateMessageAlligne_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppStatusMessageAlignment,
		    (DWORD)(Appearance.StateMessageAlligne));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTextInput"));
  if (wp)
  {
    if (Appearance.TextInputStyle != (TextInputStyle_t)(wp->GetDataField()->GetAsInteger()))
      {
	Appearance.TextInputStyle = (TextInputStyle_t)(wp->GetDataField()->GetAsInteger());
	SetToRegistry(szRegistryAppTextInputStyle, (DWORD)(Appearance.TextInputStyle));
	changed = true;
      }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndLandable,(DWORD)(Appearance.IndLandable));
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableExternalTriggerCruise"));
  if (wp) {
    if ((int)(EnableExternalTriggerCruise) !=
	wp->GetDataField()->GetAsInteger()) {
      EnableExternalTriggerCruise = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryEnableExternalTriggerCruise,
		    EnableExternalTriggerCruise);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInverseInfoBox"));
  if (wp) {
    if ((int)(Appearance.InverseInfoBox) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InverseInfoBox = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppInverseInfoBox,Appearance.InverseInfoBox);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpGliderScreenPosition"));
  if (wp) {
    if (MapWindow::GliderScreenPosition !=
	wp->GetDataField()->GetAsInteger()) {
      MapWindow::GliderScreenPosition = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryGliderScreenPosition,
		    MapWindow::GliderScreenPosition);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppDefaultMapWidth"));
  if (wp) {
    if ((int)(Appearance.DefaultMapWidth) !=
	wp->GetDataField()->GetAsInteger()) {
      Appearance.DefaultMapWidth = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppDefaultMapWidth,Appearance.DefaultMapWidth);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppAveNeedle"));
  if (wp) {
    if ((int)(Appearance.GaugeVarioAveNeedle) !=
	wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAveNeedle =
        (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppAveNeedle,Appearance.GaugeVarioAveNeedle);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxColors"));
  if (wp) {
    if ((int)(Appearance.InfoBoxColors) !=
	wp->GetDataField()->GetAsInteger()) {
      Appearance.InfoBoxColors = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppInfoBoxColors,Appearance.InfoBoxColors);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    if ((int)(Appearance.GaugeVarioSpeedToFly) != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioSpeedToFly = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioSpeedToFly,Appearance.GaugeVarioSpeedToFly);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioAvgText"));
  if (wp) {
    if ((int)Appearance.GaugeVarioAvgText != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAvgText = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioAvgText,Appearance.GaugeVarioAvgText);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioGross"));
  if (wp) {
    if ((int)Appearance.GaugeVarioGross != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioGross = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioGross,Appearance.GaugeVarioGross);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioMc"));
  if (wp) {
    if ((int)Appearance.GaugeVarioMc != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioMc = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioMc,Appearance.GaugeVarioMc);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBugs"));
  if (wp) {
    if ((int)Appearance.GaugeVarioBugs != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioBugs = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioBugs,Appearance.GaugeVarioBugs);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBallast"));
  if (wp) {
    if ((int)Appearance.GaugeVarioBallast != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioBallast = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioBallast,Appearance.GaugeVarioBallast);
      changed = true;
    }
  }

#ifdef HAVE_BLANK
  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoBlank"));
  if (wp) {
    if (EnableAutoBlank != (wp->GetDataField()->GetAsInteger()!=0)) {
      EnableAutoBlank = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAutoBlank, EnableAutoBlank);
      changed = true;
    }
  }
#endif

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoBacklight")); // VENTA4
  if (wp) {
    if (EnableAutoBacklight != (wp->GetDataField()->GetAsInteger()!=0)) {
      EnableAutoBacklight = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAutoBacklight, EnableAutoBacklight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoSoundVolume")); // VENTA4
  if (wp) {
    if (EnableAutoSoundVolume != (wp->GetDataField()->GetAsInteger()!=0)) {
      EnableAutoSoundVolume = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAutoSoundVolume, EnableAutoSoundVolume);
      changed = true;
    }
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainContrast"));
  if (wp) {
    if (iround(TerrainContrast*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      TerrainContrast = (short)iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainContrast,TerrainContrast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainBrightness"));
  if (wp) {
    if (iround(TerrainBrightness*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      TerrainBrightness = (short)iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainBrightness,TerrainBrightness);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainRamp"));
  if (wp) {
    if (TerrainRamp != wp->GetDataField()->GetAsInteger()) {
      TerrainRamp = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryTerrainRamp, TerrainRamp);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinishMinHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)FinishMinHeight != ival) {
      FinishMinHeight = ival;
      SetToRegistry(szRegistryFinishMinHeight,FinishMinHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)StartMaxHeight != ival) {
      StartMaxHeight = ival;
      SetToRegistry(szRegistryStartMaxHeight,StartMaxHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxHeightMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)StartMaxHeightMargin != ival) {
      StartMaxHeightMargin = ival;
      SetToRegistry(szRegistryStartMaxHeightMargin,StartMaxHeightMargin);
      changed = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpStartHeightRef"));
  if (wp) {
    if (StartHeightRef != wp->GetDataField()->GetAsInteger()) {
      StartHeightRef = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryStartHeightRef, StartHeightRef);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if ((int)StartMaxSpeed != ival) {
      StartMaxSpeed = ival;
      SetToRegistry(szRegistryStartMaxSpeed,StartMaxSpeed);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxSpeedMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if ((int)StartMaxSpeedMargin != ival) {
      StartMaxSpeedMargin = ival;
      SetToRegistry(szRegistryStartMaxSpeedMargin,StartMaxSpeedMargin);
      changed = true;
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    if (AutoAdvance != wp->GetDataField()->GetAsInteger()) {
      AutoAdvance = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAutoAdvance,
		    AutoAdvance);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCruise"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger());
    if (LoggerTimeStepCruise != ival) {
      LoggerTimeStepCruise = ival;
      SetToRegistry(szRegistryLoggerTimeStepCruise,LoggerTimeStepCruise);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCircling"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger());
    if (LoggerTimeStepCircling != ival) {
      LoggerTimeStepCircling = ival;
      SetToRegistry(szRegistryLoggerTimeStepCircling,LoggerTimeStepCircling);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort1"));
  if (wp) {
    if ((int)dwPortIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwPortIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed1"));
  if (wp) {
    if ((int)dwSpeedIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwSpeedIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice1"));
  if (wp) {
    if (dwDeviceIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwDeviceIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
      devRegisterGetName(dwDeviceIndex1, DeviceName);
      WriteDeviceSettings(0, DeviceName);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort2"));
  if (wp) {
    if ((int)dwPortIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwPortIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed2"));
  if (wp) {
    if ((int)dwSpeedIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwSpeedIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice2"));
  if (wp) {
    if (dwDeviceIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwDeviceIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
      devRegisterGetName(dwDeviceIndex2, DeviceName);
      WriteDeviceSettings(1, DeviceName);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSnailWidthScale"));
  if (wp) {
    if (MapWindow::SnailWidthScale != wp->GetDataField()->GetAsInteger()) {
      MapWindow::SnailWidthScale = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySnailWidthScale,MapWindow::SnailWidthScale);
      changed = true;
      requirerestart = true;
    }
  }

#ifdef FIVV
//  if ( CALCULATED_INFO.OnGround == TRUE ) {
	  wp = (WndProperty*)wf->FindByName(TEXT("prpGPSAltitudeOffset")); // VENTA3
	  if (wp) GPSAltitudeOffset = wp->GetDataField()->GetAsInteger();
//  }
#endif


  if (COMPORTCHANGED) {
    WritePort1Settings(dwPortIndex1,dwSpeedIndex1);
    WritePort2Settings(dwPortIndex2,dwSpeedIndex2);
  }

  int i,j;
  for (i=0; i<4; i++) {
    for (j=0; j<numInfoWindows; j++) {
      GetInfoBoxSelector(j, i);
    }
  }

  if (waypointneedsave) {
    if(MessageBoxX(gettext(TEXT("Save changes to waypoint file?")),
                   gettext(TEXT("Waypoints edited")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {

      AskWaypointSave();

    }
  }

  if (taskchanged) {
    RefreshTask();
  }

#if (WINDOWSPC>0)
  if (COMPORTCHANGED) {
    requirerestart = true;
  }
#endif

  if (changed) {
    StoreRegistry();

    if (!requirerestart) {
      MessageBoxX (
		   gettext(TEXT("Changes to configuration saved.")),
		   TEXT(""), MB_OK);
    } else {

      MessageBoxX (
		   gettext(TEXT("Changes to configuration saved.  Restart XCSoar to apply changes.")),
		   TEXT(""), MB_OK);
    }
  }

  delete wf;

  wf = NULL;

}

