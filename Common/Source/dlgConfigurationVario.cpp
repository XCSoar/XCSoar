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
#include "Port.h"

#include "WindowControls.h"
#include "Externs.h"
#include "McReady.h"
#include "Dialogs.h"
#include "dlgTools.h"
#include "device.h"

extern void dlgVegaDemoShowModal(void);

static bool changed = false;
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
static WndFrame *wConfig16=NULL;

#define NUMPAGES 16


static bool VegaConfigurationUpdated(TCHAR *name, bool first) {
  TCHAR updatename[100];
  TCHAR fullname[100];
  TCHAR propname[100];
  TCHAR requesttext[100];
  DWORD updated=0;
  long newval=0;
  long lvalue=0;
  DWORD dwvalue;
  
  WndProperty* wp;

#if (WINDOWSPC<1)
  if (first) {
    StepProgressDialog();
  }
#endif

  _stprintf(updatename, TEXT("Vega%sUpdated"), name);
  _stprintf(fullname, TEXT("Vega%s"), name);
  _stprintf(propname, TEXT("prp%s"), name);

  if (first) {
    SetToRegistry(updatename, 0);
    // we are not ready, haven't received value from vario
    // (do request here)
    _stprintf(requesttext,TEXT("PDVSC,R,%s"),name);
    VarioWriteNMEA(requesttext);
#ifndef _SIM_
    Sleep(250);
#endif
  }

  if (!(GetFromRegistry(fullname, (DWORD*)&lvalue)==ERROR_SUCCESS)) {
    // vario hasn't set the value in the registry yet,
    // so no sensible defaults
    return false;
  } else {
    if (first) {
      // at start, set from last known registry value, this
      // helps if variables haven't been modified.
      wp = (WndProperty*)wf->FindByName(propname);
      if (wp) {
	wp->GetDataField()->Set((int)lvalue);
	wp->RefreshDisplay();
      }
    }
  }

  if (GetFromRegistry(updatename, &updated)==ERROR_SUCCESS) {

    if (updated==1) {
      // value is updated externally, so set the property and can proceed
      // to editing values

      SetToRegistry(updatename, 2);

      wp = (WndProperty*)wf->FindByName(propname);
      if (wp) {
	wp->GetDataField()->Set((int)lvalue);
	wp->RefreshDisplay();
      }
    } 

    if (updated==2) {
      wp = (WndProperty*)wf->FindByName(propname);
      if (wp) {
	newval = (long)(wp->GetDataField()->GetAsInteger());
	if (newval != lvalue) {
	  // value has changed
	  SetToRegistry(updatename, 2);

	  dwvalue = *((DWORD*)&lvalue);

	  SetToRegistry(fullname, dwvalue);

	  changed = true;

	  // maybe represent all as text?
	  // note that this code currently won't work for longs

	  _stprintf(requesttext,TEXT("PDVSC,S,%s,%d"),name, newval);
	  VarioWriteNMEA(requesttext);
#ifndef _SIM_
	  Sleep(250);
#endif

	  return true;
	}
      }
    }
  }
  return false;
}


static void UpdateParameters(bool first) {
  VegaConfigurationUpdated(TEXT("HasPressureTE"), first);
  VegaConfigurationUpdated(TEXT("HasPressurePitot"), first);
  VegaConfigurationUpdated(TEXT("HasPressureStatic"), first);
  VegaConfigurationUpdated(TEXT("HasPressureStall"), first);
  VegaConfigurationUpdated(TEXT("HasAccelerometer"), first);
  VegaConfigurationUpdated(TEXT("HasTemperature"), first);
  VegaConfigurationUpdated(TEXT("FlarmConnected"), first);

  VegaConfigurationUpdated(TEXT("TotalEnergyMixingRatio"), first);
  VegaConfigurationUpdated(TEXT("CalibrationAirSpeed"), first);
  VegaConfigurationUpdated(TEXT("CalibrationTEStatic"), first);
  VegaConfigurationUpdated(TEXT("CalibrationTEDynamic"), first);

  //  VegaConfigurationUpdated(TEXT("PDAPower"), first);
  VegaConfigurationUpdated(TEXT("ToneClimbComparisonType"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftDetectionType"), first);
  VegaConfigurationUpdated(TEXT("ToneAveragerVarioTimeScale"), first);
  VegaConfigurationUpdated(TEXT("ToneAveragerCruiseTimeScale"), first);
  VegaConfigurationUpdated(TEXT("ToneMeanVolumeCircling"), first);
  VegaConfigurationUpdated(TEXT("ToneMeanVolumeCruise"), first);
  VegaConfigurationUpdated(TEXT("ToneBaseFrequencyOffset"), first);

  VegaConfigurationUpdated(TEXT("ToneDeadbandCirclingType"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCirclingHigh"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCirclingLow"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCruiseType"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCruiseHigh"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCruiseLow"), first);

  VegaConfigurationUpdated(TEXT("ToneCruiseFasterBeepType"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPitchScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPitchScale"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPeriodScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPeriodScale"), first);

  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerBeepType"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPitchScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPitchScale"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPeriodScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPeriodScale"), first);

  VegaConfigurationUpdated(TEXT("ToneCruiseLiftBeepType"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPitchScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPitchScale"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPeriodScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPeriodScale"), first);

  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiBeepType"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPitchScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPitchScale"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPeriodScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPeriodScale"), first);

  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowBeepType"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPitchScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPitchScale"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPeriodScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPeriodScale"), first);

  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingBeepType"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPitchScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPitchScale"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPeriodScheme"), first);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPeriodScale"), first);

  VegaConfigurationUpdated(TEXT("UTCOffset"), first);
  VegaConfigurationUpdated(TEXT("LogIGC"), first);
  VegaConfigurationUpdated(TEXT("IGCLoggerInterval"), first);
  VegaConfigurationUpdated(TEXT("MuteVarioOnPlay"), first);
  VegaConfigurationUpdated(TEXT("MuteVarioOnCom"), first);
  VegaConfigurationUpdated(TEXT("VarioRelativeMuteVol"), first);
  VegaConfigurationUpdated(TEXT("VoiceRelativeMuteVol"), first);
  VegaConfigurationUpdated(TEXT("MuteComSpkThreshold"), first);
  VegaConfigurationUpdated(TEXT("MuteComPhnThreshold"), first);
  VegaConfigurationUpdated(TEXT("MinUrgentVolume"), first);
  VegaConfigurationUpdated(TEXT("FlarmMaxObjectsReported"), first);
  VegaConfigurationUpdated(TEXT("FlarmMaxObjectsReportedOnCircling"), first);
  VegaConfigurationUpdated(TEXT("FlarmUserInterface"), first);
  VegaConfigurationUpdated(TEXT("KeepOnStraightFlightMode"), first);
  VegaConfigurationUpdated(TEXT("DontReportTrafficModeChanges"), first);
  VegaConfigurationUpdated(TEXT("DontReportGliderType"), first);
  VegaConfigurationUpdated(TEXT("FlarmPrivacyFlag"), first);
  VegaConfigurationUpdated(TEXT("FlarmAircraftType"), first);

  VegaConfigurationUpdated(TEXT("FlarmInfoRepeatTime"), first);
  VegaConfigurationUpdated(TEXT("FlarmCautionRepeatTime"), first);
  VegaConfigurationUpdated(TEXT("FlarmWarningRepeatTime"), first);
  VegaConfigurationUpdated(TEXT("GearOnDelay"), first);
  VegaConfigurationUpdated(TEXT("GearOffDelay"), first);
  VegaConfigurationUpdated(TEXT("GearRepeatTime"), first);
  VegaConfigurationUpdated(TEXT("PlyMaxComDelay"), first);
  VegaConfigurationUpdated(TEXT("BatLowDelay"), first);
  VegaConfigurationUpdated(TEXT("BatEmptyDelay"), first);
  VegaConfigurationUpdated(TEXT("BatRepeatTime"), first);

  VegaConfigurationUpdated(TEXT("NeedleGaugeType"), first);

}


static void NextPage(int Step){
  page += Step;
  if (page>=NUMPAGES) { page=0; }
  if (page<0) { page=NUMPAGES-1; }
  switch(page) {
  case 0:
    wf->SetCaption(TEXT(" 1 Hardware"));
    break;
  case 1:
    wf->SetCaption(TEXT(" 2 Calibration"));
    break;
  case 2:
    wf->SetCaption(TEXT(" 3 Audio Modes"));
    break;
  case 3:
    wf->SetCaption(TEXT(" 4 Deadband"));
    break;
  case 4:
    wf->SetCaption(TEXT(" 5 Tones: Cruise Faster"));
    break;
  case 5:
    wf->SetCaption(TEXT(" 6 Tones: Cruise Slower"));
    break;
  case 6:
    wf->SetCaption(TEXT(" 7 Tones: Cruise in Lift"));
    break;
  case 7:
    wf->SetCaption(TEXT(" 8 Tones: Circling, climbing fast"));
    break;
  case 8:
    wf->SetCaption(TEXT(" 9 Tones: Circling, climbing slow"));
    break;
  case 9:
    wf->SetCaption(TEXT("10 Tones: Circling, descending"));
    break;
  case 10:
    wf->SetCaption(TEXT("11 Vario flight logger"));
    break;
  case 11:
    wf->SetCaption(TEXT("12 Audio mixer"));
    break;
  case 12:
    wf->SetCaption(TEXT("13 FLARM Alerts"));
    break;
  case 13:
    wf->SetCaption(TEXT("14 FLARM Identification"));
    break;
  case 14:
    wf->SetCaption(TEXT("15 FLARM Repeats"));
    break;
  case 15:
    wf->SetCaption(TEXT("16 Alerts"));
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
  wConfig16->SetVisible(page == 15); 

  UpdateParameters(false);

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

static void OnSaveClicked(WindowControl * Sender){
  VarioWriteNMEA(TEXT("PDVSC,S,StoreToEeprom,2"));
}

static void OnDemoClicked(WindowControl * Sender){
  // retrieve changes from form
  UpdateParameters(false);
  dlgVegaDemoShowModal();
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
  DeclearCallBackEntry(OnDemoClicked),
  DeclearCallBackEntry(OnSaveClicked),
  DeclearCallBackEntry(NULL)
};


static void FillAudioEnums(TCHAR* name) {
  WndProperty *wp;
  TCHAR fullname[100];

  _stprintf(fullname,TEXT("prpTone%sBeepType"),name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Silence"));
    dfe->addEnumText(TEXT("Short"));
    dfe->addEnumText(TEXT("Medium"));
    dfe->addEnumText(TEXT("Long"));
    dfe->addEnumText(TEXT("Continuous"));
    dfe->addEnumText(TEXT("Short double"));
    wp->RefreshDisplay();
  }

  _stprintf(fullname,TEXT("prpTone%sPitchScheme"),name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Constant high"));
    dfe->addEnumText(TEXT("Constant medium"));
    dfe->addEnumText(TEXT("Constant low"));
    dfe->addEnumText(TEXT("Speed percent"));
    dfe->addEnumText(TEXT("Speed error"));
    dfe->addEnumText(TEXT("Vario gross"));
    dfe->addEnumText(TEXT("Vario net"));
    dfe->addEnumText(TEXT("Vario relative"));
    dfe->addEnumText(TEXT("Vario gross/relative"));
    wp->RefreshDisplay();
  }

  _stprintf(fullname,TEXT("prpTone%sPeriodScheme"),name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Constant high"));
    dfe->addEnumText(TEXT("Constant medium"));
    dfe->addEnumText(TEXT("Constant low"));
    dfe->addEnumText(TEXT("Speed percent"));
    dfe->addEnumText(TEXT("Speed error"));
    dfe->addEnumText(TEXT("Vario gross"));
    dfe->addEnumText(TEXT("Vario net"));
    dfe->addEnumText(TEXT("Vario relative"));
    dfe->addEnumText(TEXT("Vario gross/relative"));
    dfe->addEnumText(TEXT("Intermittent"));
    wp->RefreshDisplay();
  }

  _stprintf(fullname,TEXT("prpTone%sPitchScale"),name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("+Linear"));
    dfe->addEnumText(TEXT("+Low end"));
    dfe->addEnumText(TEXT("+High end"));
    dfe->addEnumText(TEXT("-Linear"));
    dfe->addEnumText(TEXT("-Low end"));
    dfe->addEnumText(TEXT("-High end"));
    wp->RefreshDisplay();
  }

  _stprintf(fullname,TEXT("prpTone%sPeriodScale"),name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("+Linear"));
    dfe->addEnumText(TEXT("+Low end"));
    dfe->addEnumText(TEXT("+High end"));
    dfe->addEnumText(TEXT("-Linear"));
    dfe->addEnumText(TEXT("-Low end"));
    dfe->addEnumText(TEXT("-High end"));
    wp->RefreshDisplay();
  }

}



bool dlgConfigurationVarioShowModal(void){

  WndProperty *wp;

  changed = false;

  wf = dlgLoadFromXML(CallBackTable, 
		      "\\NOR Flash\\dlgVario.xml", 
		      hWndMainWindow,
		      TEXT("IDR_XML_VARIO"));

  if (!wf) return false;
  
  ((WndButton *)wf->FindByName(TEXT("cmdClose")))
    ->SetOnClickNotify(OnCloseClicked);

  wConfig1    = ((WndFrame *)wf->FindByName(TEXT("frmHardware")));
  wConfig2    = ((WndFrame *)wf->FindByName(TEXT("frmCalibration")));
  wConfig3    = ((WndFrame *)wf->FindByName(TEXT("frmAudioModes")));
  wConfig4    = ((WndFrame *)wf->FindByName(TEXT("frmAudioDeadband")));
  wConfig5    = ((WndFrame *)wf->FindByName(TEXT("frmCruiseFaster")));
  wConfig6    = ((WndFrame *)wf->FindByName(TEXT("frmCruiseSlower")));
  wConfig7    = ((WndFrame *)wf->FindByName(TEXT("frmCruiseLift")));
  wConfig8    = ((WndFrame *)wf->FindByName(TEXT("frmCirclingClimbingHi")));
  wConfig9    = ((WndFrame *)wf->FindByName(TEXT("frmCirclingClimbingLow")));
  wConfig10    = ((WndFrame *)wf->FindByName(TEXT("frmCirclingDescending")));
  wConfig11    = ((WndFrame *)wf->FindByName(TEXT("frmLogger")));
  wConfig12    = ((WndFrame *)wf->FindByName(TEXT("frmMixer")));
  wConfig13    = ((WndFrame *)wf->FindByName(TEXT("frmFlarmAlerts")));
  wConfig14    = ((WndFrame *)wf->FindByName(TEXT("frmFlarmIdentification")));
  wConfig15    = ((WndFrame *)wf->FindByName(TEXT("frmFlarmRepeats")));
  wConfig16    = ((WndFrame *)wf->FindByName(TEXT("frmAlerts")));

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
  ASSERT(wConfig16!=NULL);

  //// populate enums

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneClimbComparisonType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Gross>0"));
    dfe->addEnumText(TEXT("Gross>MacCready"));
    dfe->addEnumText(TEXT("Gross>Average"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneCruiseLiftDetectionType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Disabled"));
    dfe->addEnumText(TEXT("Relative>0"));
    dfe->addEnumText(TEXT("Relative>MacCready/2"));
    dfe->addEnumText(TEXT("Gross>0"));
    dfe->addEnumText(TEXT("Net>MacCready/2"));
    dfe->addEnumText(TEXT("Relative>MacCready"));
    dfe->addEnumText(TEXT("Net>MacCready"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneAveragerVarioTimeScale"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT(" 0.0s"));
    dfe->addEnumText(TEXT(" 0.8s"));
    dfe->addEnumText(TEXT(" 1.7s"));
    dfe->addEnumText(TEXT(" 3.5s"));
    dfe->addEnumText(TEXT(" 7.5s"));
    dfe->addEnumText(TEXT("15.0s"));
    dfe->addEnumText(TEXT("30.0s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneAveragerCruiseTimeScale"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT(" 0.0s"));
    dfe->addEnumText(TEXT(" 0.8s"));
    dfe->addEnumText(TEXT(" 1.7s"));
    dfe->addEnumText(TEXT(" 3.5s"));
    dfe->addEnumText(TEXT(" 7.5s"));
    dfe->addEnumText(TEXT("15.0s"));
    dfe->addEnumText(TEXT("30.0s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneDeadbandCirclingType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Step"));
    dfe->addEnumText(TEXT("Ramp"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneDeadbandCruiseType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Step"));
    dfe->addEnumText(TEXT("Ramp"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlarmUserInterface"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("LED+Buzzer"));
    dfe->addEnumText(TEXT("None"));
    dfe->addEnumText(TEXT("Buzzer"));
    dfe->addEnumText(TEXT("LED"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlarmAircraftType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Undefined"));
    dfe->addEnumText(TEXT("Glider"));
    dfe->addEnumText(TEXT("Tow plane"));
    dfe->addEnumText(TEXT("Helicopter"));
    dfe->addEnumText(TEXT("Parachute"));
    dfe->addEnumText(TEXT("Drop plane"));
    dfe->addEnumText(TEXT("Fixed hangglider"));
    dfe->addEnumText(TEXT("Soft paraglider"));
    dfe->addEnumText(TEXT("Powered aircraft"));
    dfe->addEnumText(TEXT("Jet aircraft"));
    dfe->addEnumText(TEXT("UFO"));
    dfe->addEnumText(TEXT("Baloon"));
    dfe->addEnumText(TEXT("Blimp, Zeppelin"));
    dfe->addEnumText(TEXT("UAV (Drone)"));
    dfe->addEnumText(TEXT("Static"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }
  
  FillAudioEnums(TEXT("CruiseFaster"));
  FillAudioEnums(TEXT("CruiseSlower"));
  FillAudioEnums(TEXT("CruiseLift"));
  FillAudioEnums(TEXT("CirclingClimbingHi"));
  FillAudioEnums(TEXT("CirclingClimbingLow"));
  FillAudioEnums(TEXT("CirclingDescending"));

  wp = (WndProperty*)wf->FindByName(TEXT("prpNeedleGaugeType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("None"));
    dfe->addEnumText(TEXT("LX"));
    dfe->addEnumText(TEXT("Analog"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  /////////

#if (WINDOWSPC<1)
  CreateProgressDialog(gettext(TEXT("Reading vario settings...")));
  // Need step size finer than default 10
  SetProgressStepSize(2);
#endif

  UpdateParameters(true);

#if (WINDOWSPC<1)
  CloseProgressDialog();
#endif

  /////////

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  UpdateParameters(false);

  delete wf;

  wf = NULL;

  return changed;
}


/*
"HasPressureTE" bool
"HasPressurePitot" bool
"HasPressureStatic" bool
"HasPressureStall" bool
"HasAccelerometer" bool
"HasTemperature" bool

"TotalEnergyMixingRatio"
"CalibrationAirSpeed"
"CalibrationTEStatic"
"CalibrationTEDynamic"

"ToneAveragerVarioTimeScale"
"ToneAveragerCruiseTimeScale"
"ToneClimbComparisonType"
"ToneCruiseLiftDetectionType"

"ToneCruiseFasterBeepType"
"ToneCruiseFasterPitchScheme"
"ToneCruiseFasterPitchScale"
"ToneCruiseFasterPeriodScheme"
"ToneCruiseFasterPeriodScale"

"ToneCruiseSlowerBeepType"
"ToneCruiseSlowerPitchScheme"
"ToneCruiseSlowerPitchScale"
"ToneCruiseSlowerPeriodScheme"
"ToneCruiseSlowerPeriodScale"

"ToneCruiseLiftBeepType"
"ToneCruiseLiftPitchScheme"
"ToneCruiseLiftPitchScale"
"ToneCruiseLiftPeriodScheme"
"ToneCruiseLiftPeriodScale"

"ToneCirclingClimbingHiBeepType"
"ToneCirclingClimbingHiPitchScheme"
"ToneCirclingClimbingHiPitchScale"
"ToneCirclingClimbingHiPeriodScheme"
"ToneCirclingClimbingHiPeriodScale"

"ToneCirclingClimbingLowBeepType"
"ToneCirclingClimbingLowPitchScheme"
"ToneCirclingClimbingLowPitchScale"
"ToneCirclingClimbingLowPeriodScheme"
"ToneCirclingClimbingLowPeriodScale"

"ToneCirclingDescendingBeepType"
"ToneCirclingDescendingPitchScheme"
"ToneCirclingDescendingPitchScale"
"ToneCirclingDescendingPeriodScheme"
"ToneCirclingDescendingPeriodScale"

"ToneDeadbandCirclingType"
"ToneDeadbandCruiseType"
"ToneDeadbandCirclingHigh"
"ToneDeadbandCirclingLow"
"ToneDeadbandCruiseHigh"
"ToneDeadbandCruiseLow"
"ToneMeanVolumeCruise"
"ToneMeanVolumeCircling"
"ToneBaseFrequencyOffset"

Devices
"FlarmConnected" bool 
"EnablePDASupply" bool NOT for GNAV

Vega logger
"UTCOffset" -12 12
"LogIGC" bool
"IGCLoggerInterval" 1 12

Mixer
"MuteVarioOnPlay" bool
"MuteVarioOnCom" bool
"VarioRelativeMuteVol" 0 254
"VoiceRelativeMuteVol" 0 254
"MuteComSpkThreshold" 0 254
"MuteComPhnThreshold" 0 254
"MinUrgentVolume" 0 250

FLARM alerts
"FlarmMaxObjectsReported" 1 15
"FlarmMaxObjectsReportedOnCircling" 1 4
"FlarmUserInterface" enum 0 3
"KeepOnStraightFlightMode" bool
"DontReportTrafficModeChanges" bool
"DontReportGliderType" bool

FLARM identification
"FlarmDeviceID" text? 0xFFFFFF
"FlarmPrivacyFlag" bool
"FlarmAircraftType" enum 1 15

Advanced FLARM setup
"FlarmInfoRepeatTime" 1 2000
"FlarmCautionRepeatTime" 1 2000
"FlarmWarningRepeatTime" 1 2000

Advanced alerts
"GearOnDelay" 1 2000
"GearOffDelay" 1 2000
"GearRepeatTime" 1 100
"PlyMaxComDelay" 0 100
"BatLowDelay" 0 100
"BatEmptyDelay" 0 100
"BatRepeatTime" 0 100
*/

#endif

