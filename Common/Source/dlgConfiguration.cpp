/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#if (NEWINFOBOX>0)


#include "stdafx.h"
#include <Aygshell.h>

#include "XCSoar.h"
#include "MapWindow.h"
#include "Terrain.h"
#include "GaugeFLARM.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "device.h"
#include "Process.h"

extern int UTCOffset;

static bool changed = false;
static bool taskchanged = false;
static bool requirerestart = false;
static bool utcchanged = false;
static int page=0;
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

extern bool EnableAnimation;

#define NUMPAGES 14

static void NextPage(int Step){
  page += Step;
  if (page>=NUMPAGES) { page=0; }
  if (page<0) { page=NUMPAGES-1; }
  switch(page) {
  case 0:
    wf->SetCaption(TEXT("1 Airspace"));
    break;
  case 1:
    wf->SetCaption(TEXT("2 Map Display"));
    break;
  case 2:
    wf->SetCaption(TEXT("3 Glide Computer"));
    break;
  case 3:
    wf->SetCaption(TEXT("4 Polar"));
    break;
  case 4:
    wf->SetCaption(TEXT("5 Devices"));
    break;
  case 5:
    wf->SetCaption(TEXT("6 Units"));
    break;
  case 6:
    wf->SetCaption(TEXT("7 Interface"));
    break;
  case 7:
    wf->SetCaption(TEXT("8 Appearance"));
    break;
  case 8:
    wf->SetCaption(TEXT("9 Vario Gauge"));
    break;
  case 9:
    wf->SetCaption(TEXT("10 Task"));
    break;
  case 10:
    wf->SetCaption(TEXT("11 InfoBox Circling"));
    break;
  case 11:
    wf->SetCaption(TEXT("12 InfoBox Cruise"));
    break;
  case 12:
    wf->SetCaption(TEXT("13 InfoBox Final Glide"));
    break;
  case 13:
    wf->SetCaption(TEXT("14 InfoBox Auxiliary"));
    break;
  case 14:
    wf->SetCaption(TEXT("15 Core files"));
    break;
  }
  wConfig1->SetVisible(page == 0);
  wConfig2->SetVisible(page == 1);
  wConfig3->SetVisible(page == 2);
  wConfig4->SetVisible(page == 3);
  wConfig5->SetVisible(page == 4);
  wConfig6->SetVisible(page == 5);
  wConfig7->SetVisible(page == 6);
  wConfig8->SetVisible(page == 7);
  wConfig9->SetVisible(page == 8);
  wConfig10->SetVisible(page == 9);
  wConfig11->SetVisible(page == 10);
  wConfig12->SetVisible(page == 11);
  wConfig13->SetVisible(page == 12);
  wConfig14->SetVisible(page == 13);
  wConfig15->SetVisible(page == 14);
}


extern bool dlgConfigurationVarioShowModal(void);


static void OnVarioClicked(WindowControl * Sender){
  changed = dlgConfigurationVarioShowModal();

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  wf->FocusNext(NULL);

}

void dlgAirspaceShowModal(bool);

static void OnAirspaceColoursClicked(WindowControl * Sender){
  dlgAirspaceShowModal(true);
}

static void OnAirspaceModeClicked(WindowControl * Sender){
  dlgAirspaceShowModal(false);
}

static void OnNextClicked(WindowControl * Sender){
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
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


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAirspaceColoursClicked),
  DeclearCallBackEntry(OnAirspaceModeClicked),
  DeclearCallBackEntry(OnUTCData),
  DeclearCallBackEntry(OnNextClicked),
  DeclearCallBackEntry(OnPrevClicked),
  DeclearCallBackEntry(OnVarioClicked),
  DeclearCallBackEntry(NULL)
};


extern SCREEN_INFO Data_Options[];
extern int NUMSELECTSTRINGS;
extern int InfoType[];

void SetInfoBoxSelector(TCHAR *name, int item, int mode)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (int i=0; i<NUMSELECTSTRINGS; i++) {
      dfe->addEnumText(Data_Options[i].Description);
    }
    int it=0;

    switch(mode) {
    case 0: // cruise
      it = (InfoType[item]>>8)& 0xff;
      break;
    case 1: // climb
      it = (InfoType[item])& 0xff;
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


void GetInfoBoxSelector(TCHAR *name, int item, int mode)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    int itnew = wp->GetDataField()->GetAsInteger();
    int it=0;

    switch(mode) {
    case 0: // cruise
      it = (InfoType[item]>>8)& 0xff;
      break;
    case 1: // climb
      it = (InfoType[item])& 0xff;
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




extern TCHAR *PolarLabels[];

void dlgConfigurationShowModal(void){

  WndProperty *wp;

  wf = dlgLoadFromXML(CallBackTable,
		      LocalPathS(TEXT("dlgConfiguration.xml")),
		      hWndMainWindow,
		      TEXT("IDR_XML_CONFIGURATION"));

  if (!wf) return;

  //  wf->SetKeyDownNotify(FormKeyDown);
  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wConfig1    = ((WndFrame *)wf->FindByName(TEXT("frmAirspace")));
  wConfig2    = ((WndFrame *)wf->FindByName(TEXT("frmDisplay")));
  wConfig3    = ((WndFrame *)wf->FindByName(TEXT("frmFinalGlide")));
  wConfig4    = ((WndFrame *)wf->FindByName(TEXT("frmPolar")));
  wConfig5    = ((WndFrame *)wf->FindByName(TEXT("frmComm")));
  wConfig6    = ((WndFrame *)wf->FindByName(TEXT("frmUnits")));
  wConfig7    = ((WndFrame *)wf->FindByName(TEXT("frmInterface")));
  wConfig8    = ((WndFrame *)wf->FindByName(TEXT("frmAppearance")));
  wConfig9    = ((WndFrame *)wf->FindByName(TEXT("frmVarioAppearance")));
  wConfig10    = ((WndFrame *)wf->FindByName(TEXT("frmTask")));
  wConfig11    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCircling")));
  wConfig12    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCruise")));
  wConfig13    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxFinalGlide")));
  wConfig14    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxAuxiliary")));
  wConfig15    = ((WndFrame *)wf->FindByName(TEXT("frmFiles")));

  ASSERT(wConfig1!=NULL);
  ASSERT(wConfig2!=NULL);
  ASSERT(wConfig3!=NULL);
  ASSERT(wConfig4!=NULL);
  ASSERT(wConfig5!=NULL);
  ASSERT(wConfig6!=NULL);
  ASSERT(wConfig7!=NULL);
  ASSERT(wConfig8!=NULL);
  ASSERT(wConfig9!=NULL);
  ASSERT(wConfig10!=NULL);
  ASSERT(wConfig11!=NULL);
  ASSERT(wConfig12!=NULL);
  ASSERT(wConfig13!=NULL);
  ASSERT(wConfig14!=NULL);
  ASSERT(wConfig15!=NULL);

  //////////
  /*
  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

  return;
  */
  ///////////////////

  // TODO: all appearance variables

  TCHAR *COMMPort[] = {TEXT("COM1"),TEXT("COM2"),TEXT("COM3"),TEXT("COM4"),TEXT("COM5"),TEXT("COM6"),TEXT("COM7"),TEXT("COM8"),TEXT("COM9"),TEXT("COM10")};
  TCHAR *tSpeed[] = {TEXT("1200"),TEXT("2400"),TEXT("4800"),TEXT("9600"),TEXT("19200"),TEXT("38400"),TEXT("57600"),TEXT("115200")};
  DWORD dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};

  DWORD dwPortIndex1 = 0;
  DWORD dwSpeedIndex1 = 2;
  DWORD dwPortIndex2 = 0;
  DWORD dwSpeedIndex2 = 2;

  int i;
  int dwDeviceIndex1=0;
  int dwDeviceIndex2=0;
  TCHAR DeviceName[DEVNAMESIZE+1];

  ReadPort1Settings(&dwPortIndex1,&dwSpeedIndex1);

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<10; i++) {
      dfe->addEnumText(COMMPort[i]);
    }
    dfe->Set(dwPortIndex1);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<8; i++) {
      dfe->addEnumText(tSpeed[i]);
    }
    dfe->Set(dwSpeedIndex1);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Generic"));
    for (i=0; i<DeviceRegisterCount; i++) {
      decRegisterGetName(i, DeviceName);
      dfe->addEnumText(DeviceName);
      if (devA() != NULL){
	if (_tcscmp(DeviceName, devA()->Name) == 0)
	  dwDeviceIndex1 = i+1;
      }
    }
    dfe->Set(dwDeviceIndex1);
    wp->RefreshDisplay();
  }

  ReadPort2Settings(&dwPortIndex2,&dwSpeedIndex2);

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<10; i++) {
      dfe->addEnumText(COMMPort[i]);
    }
    dfe->Set(dwPortIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<8; i++) {
      dfe->addEnumText(tSpeed[i]);
    }
    dfe->Set(dwSpeedIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Generic"));
    for (i=0; i<DeviceRegisterCount; i++) {
      decRegisterGetName(i, DeviceName);
      dfe->addEnumText(DeviceName);
      if (devB() != NULL){
	if (_tcscmp(DeviceName, devB()->Name) == 0)
	  dwDeviceIndex2 = i+1;
      }
    }
    dfe->Set(dwDeviceIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("All on"));
    dfe->addEnumText(TEXT("Clip"));
    dfe->addEnumText(TEXT("Auto"));
    dfe->addEnumText(TEXT("All below"));
    dfe->Set(AltitudeMode);
    wp->RefreshDisplay();
  }
  //

  wp = (WndProperty*)wf->FindByName(TEXT("prpUTCOffset"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(UTCOffset/3600.0);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpDebounceTimeout"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(debounceTimeout);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMDisplay"));
  if (wp) {
    wp->GetDataField()->Set(EnableFLARMDisplay);
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
    dfe->addEnumText(TEXT("Names"));
    dfe->addEnumText(TEXT("Numbers"));
    dfe->addEnumText(TEXT("First 5"));
    dfe->addEnumText(TEXT("None"));
    dfe->addEnumText(TEXT("First 3"));
    dfe->addEnumText(TEXT("Names in task"));
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
    dfe->addEnumText(TEXT("Track up"));
    dfe->addEnumText(TEXT("North up"));
    dfe->addEnumText(TEXT("North circling"));
    dfe->addEnumText(TEXT("Target circling"));
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
    wp->GetDataField()->Set(FinalGlideTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
  if (wp) {
    wp->GetDataField()->Set(EnableAutoWind);
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

  DWORD Speed = 0;
  DWORD Distance = 0;
  DWORD Lift = 0;
  DWORD Altitude = 0;

  GetFromRegistry(szRegistrySpeedUnitsValue,&Speed);
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Statue"));
    dfe->addEnumText(TEXT("Nautical"));
    dfe->addEnumText(TEXT("Metric"));
    dfe->Set(Speed);
    wp->RefreshDisplay();
  }

  GetFromRegistry(szRegistryDistanceUnitsValue,&Distance);
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsDistance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Statute"));
    dfe->addEnumText(TEXT("Nautical"));
    dfe->addEnumText(TEXT("Metric"));
    dfe->Set(Distance);
    wp->RefreshDisplay();
  }

  GetFromRegistry(szRegistryAltitudeUnitsValue,&Altitude);
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsAltitude"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Feet"));
    dfe->addEnumText(TEXT("Meters"));
    dfe->Set(Altitude);
    wp->RefreshDisplay();
  }

  GetFromRegistry(szRegistryLiftUnitsValue,&Lift);
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLift"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Knots"));
    dfe->addEnumText(TEXT("M/s"));
    dfe->Set(Lift);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::EnableTrailDrift);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSetSystemTimeFromGPS"));
  if (wp) {
    wp->GetDataField()->Set(SetSystemTimeFromGPS);
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
    dfe->addEnumText(TEXT("Off"));
    dfe->addEnumText(TEXT("Long"));
    dfe->addEnumText(TEXT("Short"));
    dfe->Set(TrailActive);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxManoeuveringSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(SPEEDMODIFY*SAFTEYSPEED));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
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
      TCHAR *name;
      name = GetWinPilotPolarInternalName(i);
      if (!name) {
	ok=false;
      } else {
	dfe->addEnumText(name);
      }
      i++;
    }
    dfe->Set(POLARID);
    wp->RefreshDisplay();
  }

  TCHAR szPolarFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryPolarFile, szPolarFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.plr"));
    dfe->Lookup(szPolarFile);
    wp->RefreshDisplay();
  }

  TCHAR szAirspaceFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAirspaceFile, szAirspaceFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(szAirspaceFile);
    wp->RefreshDisplay();
  }

  TCHAR szAdditionalAirspaceFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAdditionalAirspaceFile,
		    szAdditionalAirspaceFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(szAdditionalAirspaceFile);
    wp->RefreshDisplay();
  }

  TCHAR szWaypointFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->Lookup(szWaypointFile);
    wp->RefreshDisplay();
  }

  TCHAR szAdditionalWaypointFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAdditionalWayPointFile,
		    szAdditionalWaypointFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->Lookup(szAdditionalWaypointFile);
    wp->RefreshDisplay();
  }

  TCHAR szTerrainFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryTerrainFile, szTerrainFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->Lookup(szTerrainFile);
    wp->RefreshDisplay();
  }

  TCHAR szTopologyFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryTopologyFile, szTopologyFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.tpl"));
    dfe->Lookup(szTopologyFile);
    wp->RefreshDisplay();
  }

  TCHAR szAirfieldFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAirfieldFile, szAirfieldFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(szAirfieldFile);
    wp->RefreshDisplay();
  }

  TCHAR szLanguageFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryLanguageFile, szLanguageFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcl"));
    dfe->Lookup(szLanguageFile);
    wp->RefreshDisplay();
  }

  TCHAR szStatusFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryStatusFile, szStatusFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcs"));
    dfe->Lookup(szStatusFile);
    wp->RefreshDisplay();
  }

  TCHAR szInputFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryInputFile, szInputFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xci"));
    dfe->Lookup(szInputFile);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppStatusMessageAlignment"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Center"));
    dfe->addEnumText(TEXT("Topleft"));
    dfe->Set(Appearance.StateMessageAlligne);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppCompassAppearance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Normal"));
    dfe->addEnumText(TEXT("White outline"));
    dfe->Set(Appearance.CompassAppearance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndFinalGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Default"));
    dfe->addEnumText(TEXT("Alternate"));
    dfe->Set(Appearance.IndFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Winpilot"));
    dfe->addEnumText(TEXT("Alternate"));
    dfe->Set(Appearance.IndLandable);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxColors"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.InfoBoxColors);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Line"));
    dfe->Set(FinishLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(FinishRadius*2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Line"));
    dfe->Set(StartLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(StartRadius*2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("FAI Sector"));
    dfe->addEnumText(TEXT("DAe 0.5/10"));
    dfe->Set(SectorType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SectorRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Manual"));
    dfe->addEnumText(TEXT("Auto"));
    dfe->addEnumText(TEXT("Arm"));
    dfe->addEnumText(TEXT("Arm start"));
    dfe->Set(AutoAdvance);
    wp->RefreshDisplay();
  }

  ////

  SetInfoBoxSelector(TEXT("prpInfoBoxCruise0"),0,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise1"),1,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise2"),2,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise3"),3,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise4"),4,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise5"),5,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise6"),6,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise7"),7,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise8"),8,0);

  SetInfoBoxSelector(TEXT("prpInfoBoxCircling0"),0,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling1"),1,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling2"),2,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling3"),3,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling4"),4,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling5"),5,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling6"),6,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling7"),7,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling8"),8,1);

  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide0"),0,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide1"),1,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide2"),2,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide3"),3,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide4"),4,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide5"),5,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide6"),6,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide7"),7,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide8"),8,2);

  SetInfoBoxSelector(TEXT("prpInfoBoxAux0"),0,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux1"),1,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux2"),2,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux3"),3,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux4"),4,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux5"),5,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux6"),6,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux7"),7,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux8"),8,3);

  // TODO: configuration of Appearance

  ////

  NextPage(0); // JMW just to turn proper pages on/off

  changed = false;
  taskchanged = false;
  requirerestart = false;
  utcchanged = false;

  wf->ShowModal();

  // TODO: implement a cancel button that skips all this below after exit.

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

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrail"));
  if (wp) {
    if (TrailActive != wp->GetDataField()->GetAsInteger()) {
      TrailActive = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySnailTrail, TrailActive);
      changed = true;
    }
  }

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
      SetToRegistry(szRegistryAltMode,AltitudeMode);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMDisplay"));
  if (wp) {
    if (EnableFLARMDisplay !=
	wp->GetDataField()->GetAsBoolean()) {
      EnableFLARMDisplay = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryEnableFLARMDisplay,
		    EnableFLARMDisplay);
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
    ival = iround(wp->GetDataField()->GetAsInteger()*3600.0);
    if ((UTCOffset != ival)||(utcchanged)) {
      UTCOffset = ival;

      // have to do this because registry variables can't be negative!
      int lival = UTCOffset;
      if (lival<0) { lival+= 24; }
      SetToRegistry(szRegistryUTCOffset, lival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (ClipAltitude != ival) {
      ClipAltitude = ival;
      SetToRegistry(szRegistryAltMode,ClipAltitude);
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
    if (EnableAutoWind != wp->GetDataField()->GetAsBoolean()) {
      EnableAutoWind = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAutoWind, EnableAutoWind);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinalGlideTerrain"));
  if (wp) {
    if (FinalGlideTerrain != wp->GetDataField()->GetAsBoolean()) {
      FinalGlideTerrain = wp->GetDataField()->GetAsBoolean();
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

  TCHAR* temptext = 0;

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
    if (_tcscmp(temptext,szAdditionalAirspaceFile)) {
      SetRegistryString(szRegistryAdditionalAirspaceFile, temptext);
      AIRSPACEFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
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
    temptext = dfe->GetPathFile();
    if (_tcscmp(temptext,szInputFile)) {
      SetRegistryString(szRegistryInputFile, temptext);
      requirerestart = true;
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
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    if ((int)FinishRadius*2 != wp->GetDataField()->GetAsInteger()) {
      FinishRadius = wp->GetDataField()->GetAsInteger()/2;
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
    if ((int)StartRadius*2 != wp->GetDataField()->GetAsInteger()) {
      StartRadius = wp->GetDataField()->GetAsInteger()/2;
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
    if ((int)SectorRadius != wp->GetDataField()->GetAsInteger()) {
      SectorRadius = wp->GetDataField()->GetAsInteger();
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndLandable,(DWORD)(Appearance.IndLandable));
      changed = true;
      requirerestart = true;
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainContrast"));
  if (wp) {
    if (iround(TerrainContrast*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      TerrainContrast = iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainContrast,TerrainContrast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainBrightness"));
  if (wp) {
    if (iround(TerrainBrightness*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      TerrainBrightness = iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainBrightness,TerrainBrightness);
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
      decRegisterGetName(dwDeviceIndex1-1, DeviceName);
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
      decRegisterGetName(dwDeviceIndex2-1, DeviceName);
      WriteDeviceSettings(1, DeviceName);
    }
  }

  if (COMPORTCHANGED) {
    WritePort1Settings(dwPortIndex1,dwSpeedIndex1);
    WritePort2Settings(dwPortIndex2,dwSpeedIndex2);
  }

  GetInfoBoxSelector(TEXT("prpInfoBoxCruise0"),0,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise1"),1,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise2"),2,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise3"),3,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise4"),4,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise5"),5,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise6"),6,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise7"),7,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise8"),8,0);

  GetInfoBoxSelector(TEXT("prpInfoBoxCircling0"),0,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling1"),1,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling2"),2,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling3"),3,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling4"),4,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling5"),5,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling6"),6,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling7"),7,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling8"),8,1);

  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide0"),0,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide1"),1,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide2"),2,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide3"),3,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide4"),4,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide5"),5,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide6"),6,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide7"),7,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide8"),8,2);

  GetInfoBoxSelector(TEXT("prpInfoBoxAux0"),0,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux1"),1,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux2"),2,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux3"),3,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux4"),4,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux5"),5,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux6"),6,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux7"),7,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux8"),8,3);

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
      MessageBoxX (hWndMainWindow,
		   gettext(TEXT("Changes to configuration saved.")),
		   TEXT(""), MB_OK);
    } else {

      MessageBoxX (hWndMainWindow,
		   gettext(TEXT("Changes to configuration saved.  Restart XCSoar.")),
		   TEXT(""), MB_OK);
    }
  }

  delete wf;

  wf = NULL;

}

// extern TCHAR szRegistryAirspaceBlackOutline[];
// NUMAIRSPACECOLORS
// AIRSPACECLASSCOUNT
// SetRegistryColour(CTR, NewColor);
/*
 AIRSPACECLASSCOUNT

  { CLASSA,     _T("Class A")},
  { CLASSB,     _T("Class B")},
  { CLASSC,     _T("Class C")},
  { CLASSD,     _T("Class D")},
  { CLASSE,     _T("Class E")},
  { CLASSF,     _T("Class F")},
  { PROHIBITED, _T("Prohibited areas")},
  { DANGER,     _T("Danger areas")},
  { RESTRICT,   _T("Restricted areas")},
  { CTR,        _T("CTR")},
  { NOGLIDER,   _T("No Gliders")},
  { WAVE,       _T("Wave")},
  { OTHER,      _T("Other")},
  { AATASK,     _T("AAT")}
*/

#endif
