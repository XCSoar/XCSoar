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


#include "stdafx.h"
#include <Aygshell.h>

#include "XCSoar.h"
#include "MapWindow.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"

static bool changed = false;
static bool taskchanged = false;
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

static void NextPage(int Step){
  page += Step;
  if (page>13) { page=0; }
  if (page<0) { page=13; }
  switch(page) {
  case 0:
    wf->SetCaption(TEXT("1 Airspace"));
    break;
  case 1:
    wf->SetCaption(TEXT("2 Display"));
    break;
  case 2:
    wf->SetCaption(TEXT("3 Final Glide"));
    break;
  case 3:
    wf->SetCaption(TEXT("4 Polar"));
    break;
  case 4:
    wf->SetCaption(TEXT("5 Comms"));
    break;
  case 5:
    wf->SetCaption(TEXT("6 Units"));
    break;
  case 6:
    wf->SetCaption(TEXT("7 Appearance"));
    break;
  case 7:
    wf->SetCaption(TEXT("8 Vario gauge"));
    break;
  case 8:
    wf->SetCaption(TEXT("9 Task"));
    break;
  case 9:
    wf->SetCaption(TEXT("10 InfoBox Circling"));
    break;
  case 10:
    wf->SetCaption(TEXT("11 InfoBox Cruise"));
    break;
  case 11:
    wf->SetCaption(TEXT("12 InfoBox Final Glide"));
    break;
  case 12:
    wf->SetCaption(TEXT("13 InfoBox Auxiliary"));
    break;
  case 13:
    wf->SetCaption(TEXT("14 Core files"));
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

/*
int enumval = 0;

static void OnTestEnumData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static int lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = enumval;
      Sender->Set(enumval);
    break;
    case DataField::daPut:
      enumval = Sender->GetAsInteger();
    break;
    case DataField::daChange:
    break;
  }
}
*/

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnNextClicked),
  DeclearCallBackEntry(OnPrevClicked),
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
		      "\\NOR Flash\\dlgConfiguration.xml", 
		      hWndMainWindow);

  if (!wf) return;

  //  wf->SetKeyDownNotify(FormKeyDown);
  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wConfig1    = ((WndFrame *)wf->FindByName(TEXT("frmAirspace")));
  wConfig2    = ((WndFrame *)wf->FindByName(TEXT("frmDisplay")));
  wConfig3    = ((WndFrame *)wf->FindByName(TEXT("frmFinalGlide")));
  wConfig4    = ((WndFrame *)wf->FindByName(TEXT("frmPolar")));
  wConfig5    = ((WndFrame *)wf->FindByName(TEXT("frmComm")));
  wConfig6    = ((WndFrame *)wf->FindByName(TEXT("frmUnits")));
  wConfig7    = ((WndFrame *)wf->FindByName(TEXT("frmAppearance")));
  wConfig8    = ((WndFrame *)wf->FindByName(TEXT("frmVarioAppearance")));
  wConfig9    = ((WndFrame *)wf->FindByName(TEXT("frmTask")));
  wConfig10    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCircling")));
  wConfig11    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCruise")));
  wConfig12    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxFinalGlide")));
  wConfig13    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxAuxiliary")));
  wConfig14    = ((WndFrame *)wf->FindByName(TEXT("frmFiles")));

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

  // TODO: units conversions, display units
  // TODO: COM parameters
  // TODO: airspace colors
  // TODO: all appearance variables
  // TODO: warning of need to restart for language/input/appearance

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

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(ClipAltitude);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltWarningMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(AltWarningMargin);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceOutline"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::bAirspaceBlackOutline);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceWarnings"));
  if (wp) {
    bool aw = AIRSPACEWARNINGS;
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
    dfe->addEnumText(TEXT("None"));
    dfe->addEnumText(TEXT("First 3"));
    dfe->addEnumText(TEXT("First 5"));
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
    dfe->addEnumText(TEXT("Track circling"));
    dfe->Set(DisplayOrientation);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeArrival"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SAFETYALTITUDEARRIVAL);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeBreakoff"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SAFETYALTITUDEBREAKOFF);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeTerrain"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SAFETYALTITUDETERRAIN);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinalGlideTerrain"));
  if (wp) {
    wp->GetDataField()->Set(FinalGlideTerrain);
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


  // TODO handle change
  wp = (WndProperty*)wf->FindByName(TEXT("prpTrail"));
  //           SetToRegistry(szRegistrySnailTrail,TrailActive);
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
    wp->GetDataField()->SetAsFloat(SAFTEYSPEED);
    wp->RefreshDisplay();
  }

  int i;
  // TODO handle change
  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<NUMPOLARS; i++) {
      dfe->addEnumText(PolarLabels[i]);
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
    //    dfe->ScanFiles(TEXT("\\My Documents\\"),TEXT("*.plr"));
    dfe->ScanDirectories(NULL,TEXT("*.plr"));
    dfe->Lookup(szPolarFile);
    wp->RefreshDisplay();
  }

  TCHAR szAirspaceFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAirspaceFile, szAirspaceFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.txt"));
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
    //    dfe->ScanFiles(TEXT("\\My Documents\\"),TEXT("*.plr"));
    dfe->ScanDirectories(NULL,TEXT("*.txt"));
    dfe->Lookup(szAdditionalAirspaceFile);
    wp->RefreshDisplay();
  }

  TCHAR szWaypointFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.dat"));
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
    dfe->ScanDirectories(NULL,TEXT("*.dat"));
    dfe->Lookup(szAdditionalWaypointFile);
    wp->RefreshDisplay();
  }

  TCHAR szTerrainFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryTerrainFile, szTerrainFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.dat"));
    dfe->Lookup(szTerrainFile);
    wp->RefreshDisplay();
  }

  TCHAR szTopologyFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryTopologyFile, szTopologyFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.tpl"));
    dfe->Lookup(szTopologyFile);
    wp->RefreshDisplay();
  }

  TCHAR szAirfieldFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAirfieldFile, szAirfieldFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.txt"));
    dfe->Lookup(szAirfieldFile);
    wp->RefreshDisplay();
  }

  TCHAR szLanguageFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryLanguageFile, szLanguageFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.xcl"));
    dfe->Lookup(szLanguageFile);
    wp->RefreshDisplay();
  }

  TCHAR szStatusFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryStatusFile, szStatusFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.xcs"));
    dfe->Lookup(szStatusFile);
    wp->RefreshDisplay();
  }

  TCHAR szInputFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryInputFile, szInputFile, MAX_PATH);
  wp = (WndProperty*)wf->FindByName(TEXT("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectories(NULL,TEXT("*.xci"));
    dfe->Lookup(szInputFile);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndFinalGlide"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Appearance.IndFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Appearance.IndLandable);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInverseInfoBox"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.InverseInfoBox);
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
    dfe->addEnumText(TEXT("Sector"));
    dfe->Set(FAISector);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SectorRadius);
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

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  // TODO: implement a cancel button that skips all this below after exit.

  changed = false;
  taskchanged = false;

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceDisplay"));
  if (wp) {
    if (AltitudeMode != wp->GetDataField()->GetAsInteger()) {
      AltitudeMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltMode,AltitudeMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceOutline"));
  if (wp) {
    if (MapWindow::bAirspaceBlackOutline != wp->GetDataField()->GetAsBoolean()) {
      MapWindow::bAirspaceBlackOutline = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAirspaceBlackOutline,
		    MapWindow::bAirspaceBlackOutline);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    if (ClipAltitude != wp->GetDataField()->GetAsInteger()) {
      ClipAltitude = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltMode,ClipAltitude);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltWarningMargin"));
  if (wp) {
    if (AltWarningMargin != wp->GetDataField()->GetAsInteger()) {
      AltWarningMargin = wp->GetDataField()->GetAsInteger();
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeArrival"));
  if (wp) {
    if (SAFETYALTITUDEARRIVAL != wp->GetDataField()->GetAsInteger()) {
      SAFETYALTITUDEARRIVAL = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySafetyAltitudeArrival,
		    (DWORD)SAFETYALTITUDEARRIVAL);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeBreakoff"));
  if (wp) {
    if (SAFETYALTITUDEBREAKOFF != wp->GetDataField()->GetAsInteger()) {
      SAFETYALTITUDEBREAKOFF = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySafetyAltitudeBreakOff,
		    (DWORD)SAFETYALTITUDEBREAKOFF);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeTerrain"));
  if (wp) {
    if (SAFETYALTITUDETERRAIN != wp->GetDataField()->GetAsInteger()) {
      SAFETYALTITUDETERRAIN = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySafetyAltitudeTerrain,
		    (DWORD)SAFETYALTITUDETERRAIN);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsSpeed"));
  if (wp) {
    if (Speed != wp->GetDataField()->GetAsInteger()) {
      Speed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySpeedUnitsValue, Speed);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsDistance"));
  if (wp) {
    if (Distance != wp->GetDataField()->GetAsInteger()) {
      Distance = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDistanceUnitsValue, Distance);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLift"));
  if (wp) {
    if (Lift != wp->GetDataField()->GetAsInteger()) {
      Lift = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryLiftUnitsValue, Lift);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsAltitude"));
  if (wp) {
    if (Altitude != wp->GetDataField()->GetAsInteger()) {
      Altitude = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
      changed = true;
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
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxManoeuveringSpeed"));
  if (wp) {
    if (SAFTEYSPEED != wp->GetDataField()->GetAsInteger()) {
      SAFTEYSPEED = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySafteySpeed,(DWORD)SAFTEYSPEED);
      GlidePolar::SetBallast();
      changed = true;
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
    if (StartRadius*2 != wp->GetDataField()->GetAsInteger()) {
      StartRadius = wp->GetDataField()->GetAsInteger()/2;
      SetToRegistry(szRegistryStartRadius,StartRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    if (FAISector != wp->GetDataField()->GetAsInteger()) {
      FAISector = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFAISector,FAISector);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    if (SectorRadius != wp->GetDataField()->GetAsInteger()) {
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndLandable,(DWORD)(Appearance.IndLandable));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInverseInfoBox"));
  if (wp) {
    if (Appearance.InverseInfoBox != wp->GetDataField()->GetAsInteger()) {
      Appearance.InverseInfoBox = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppInverseInfoBox,Appearance.InverseInfoBox);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    if (Appearance.GaugeVarioSpeedToFly != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioSpeedToFly = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppGaugeVarioSpeedToFly,Appearance.GaugeVarioSpeedToFly);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioAvgText"));
  if (wp) {
    if (Appearance.GaugeVarioAvgText != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAvgText = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppGaugeVarioAvgText,Appearance.GaugeVarioAvgText);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioMc"));
  if (wp) {
    if (Appearance.GaugeVarioMc != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioMc = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppGaugeVarioMc,Appearance.GaugeVarioMc);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBugs"));
  if (wp) {
    if (Appearance.GaugeVarioBugs != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioBugs = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppGaugeVarioBugs,Appearance.GaugeVarioBugs);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBallast"));
  if (wp) {
    if (Appearance.GaugeVarioBallast != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioBallast = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppGaugeVarioBallast,Appearance.GaugeVarioBallast);
      changed = true;
    }
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

  if (changed) {
#ifdef GNAV
    SaveRegistryToFile(TEXT("\\NOR Flash\\xcsoar-registry.prf"));
#else
    SaveRegistryToFile(TEXT("xcsoar-registry.prf"));
#endif
  };

  delete wf;

  wf = NULL;

}

