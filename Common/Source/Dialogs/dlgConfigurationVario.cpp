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
#include "Dialogs/Message.hpp"
#include "McReady.h"
#include "Device/device.h"
#include "InfoBoxLayout.h"
#include "Registry.hpp"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"

#include <assert.h>

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
static WndFrame *wConfig17=NULL;
static WndFrame *wConfig18=NULL;
static WndFrame *wConfig19=NULL;

#define NUMPAGES 19


static bool VegaConfigurationUpdated(const TCHAR *name, bool first,
                                     bool setvalue=false,
                                     long ext_setvalue=0) {
  TCHAR updatename[100];
  TCHAR fullname[100];
  TCHAR propname[100];
  TCHAR requesttext[100];
  DWORD updated=0;
  unsigned newval=0;
  unsigned lvalue=0;

  WndProperty* wp;

#ifndef WINDOWSPC
  if (first) {
    XCSoarInterface::StepProgressDialog();
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

    if (!is_simulator())
      Sleep(250);
  }

  if (setvalue) {
    wp = (WndProperty*)wf->FindByName(propname);
    if (wp) {
      wp->GetDataField()->Set((int)ext_setvalue);
      wp->RefreshDisplay();
    }
    _stprintf(requesttext,TEXT("PDVSC,S,%s,%d"),name, ext_setvalue);
    VarioWriteNMEA(requesttext);

    if (!is_simulator())
      Sleep(250);

    return true;
  }

  if (!(GetFromRegistry(fullname, lvalue)==ERROR_SUCCESS)) {
    // vario hasn't set the value in the registry yet,
    // so no sensible defaults
    return false;
  } else {

    // hack, fix the -1 (plug and play settings)
    if (_tcscmp(name, TEXT("HasTemperature")) == 0){
      if (lvalue >= 255)
        lvalue = 2;
    }
    if (first) {
      // at start, set from last known registry value, this
      // helps if variables haven't been modified.
      wp = (WndProperty*)wf->FindByName(propname);
      if (wp) {
	wp->GetDataField()->Set(lvalue);
	wp->RefreshDisplay();
      }
    }
  }

  if (GetFromRegistryD(updatename, updated)==ERROR_SUCCESS) {

    if (updated==1) {
      // value is updated externally, so set the property and can proceed
      // to editing values

      SetToRegistry(updatename, 2);

      wp = (WndProperty*)wf->FindByName(propname);
      if (wp) {
	wp->GetDataField()->Set(lvalue);
	wp->RefreshDisplay();
      }
    } else if (updated==2) {
      wp = (WndProperty*)wf->FindByName(propname);
      if (wp) {
	newval = (wp->GetDataField()->GetAsInteger());
	if (newval != lvalue) {
	  // value has changed
	  SetToRegistry(updatename, 2);
	  SetToRegistry(fullname, (DWORD)lvalue);

	  changed = true;

	  // maybe represent all as text?
	  // note that this code currently won't work for longs

	  // hack, fix the -1 (plug and play settings)
	  if (_tcscmp(name, TEXT("HasTemperature")) == 0){
	    if (newval == 2)
	      newval = 255;
	  }

	  _stprintf(requesttext,TEXT("PDVSC,S,%s,%d"),name, newval);
	  VarioWriteNMEA(requesttext);

          if (!is_simulator())
            Sleep(250);

	  return true;
	}
      }
    }
  }
  return false;
}


typedef struct VEGA_SCHEME_t
{
  long ToneClimbComparisonType;
  long ToneLiftComparisonType;

  long ToneCruiseFasterBeepType;
  long ToneCruiseFasterPitchScheme;
  long ToneCruiseFasterPitchScale;
  long ToneCruiseFasterPeriodScheme;
  long ToneCruiseFasterPeriodScale;

  long ToneCruiseSlowerBeepType;
  long ToneCruiseSlowerPitchScheme;
  long ToneCruiseSlowerPitchScale;
  long ToneCruiseSlowerPeriodScheme;
  long ToneCruiseSlowerPeriodScale;

  long ToneCruiseLiftBeepType;
  long ToneCruiseLiftPitchScheme;
  long ToneCruiseLiftPitchScale;
  long ToneCruiseLiftPeriodScheme;
  long ToneCruiseLiftPeriodScale;

  long ToneCirclingClimbingHiBeepType;
  long ToneCirclingClimbingHiPitchScheme;
  long ToneCirclingClimbingHiPitchScale;
  long ToneCirclingClimbingHiPeriodScheme;
  long ToneCirclingClimbingHiPeriodScale;

  long ToneCirclingClimbingLowBeepType;
  long ToneCirclingClimbingLowPitchScheme;
  long ToneCirclingClimbingLowPitchScale;
  long ToneCirclingClimbingLowPeriodScheme;
  long ToneCirclingClimbingLowPeriodScale;

  long ToneCirclingDescendingBeepType;
  long ToneCirclingDescendingPitchScheme;
  long ToneCirclingDescendingPitchScale;
  long ToneCirclingDescendingPeriodScheme;
  long ToneCirclingDescendingPeriodScale;

} VEGA_SCHEME;

// Value used for comparison in climb tone
#define  X_NONE                       0
#define  X_MCCREADY                   1
#define  X_AVERAGE                    2

// Condition for detecting lift in cruise mode
#define  Y_NONE 0
#define  Y_RELATIVE_ZERO 1
#define  Y_RELATIVE_MCCREADY_HALF 2
#define  Y_GROSS_ZERO 3
#define  Y_NET_MCCREADY_HALF 4
#define  Y_RELATIVE_MCCREADY 5
#define  Y_NET_MCCREADY 6

// Beep types
#define  BEEPTYPE_SILENCE 0
#define  BEEPTYPE_SHORT 1
#define  BEEPTYPE_MEDIUM 2
#define  BEEPTYPE_LONG 3
#define  BEEPTYPE_CONTINUOUS 4
#define  BEEPTYPE_SHORTDOUBLE 5

// Pitch value schemes
#define  PITCH_CONST_HI 0
#define  PITCH_CONST_MEDIUM 1
#define  PITCH_CONST_LO 2
#define  PITCH_SPEED_PERCENT 3
#define  PITCH_SPEED_ERROR 4
#define  PITCH_VARIO_GROSS 5
#define  PITCH_VARIO_NET 6
#define  PITCH_VARIO_RELATIVE 7
#define  PITCH_VARIO_GROSSRELATIVE 8

// Beep period value schemes
#define  PERIOD_CONST_HI 0
#define  PERIOD_CONST_MEDIUM 1
#define  PERIOD_CONST_LO 2
#define  PERIOD_SPEED_PERCENT 3
#define  PERIOD_SPEED_ERROR 4
#define  PERIOD_VARIO_GROSS 5
#define  PERIOD_VARIO_NET 6
#define  PERIOD_VARIO_RELATIVE 7
#define  PERIOD_VARIO_GROSSRELATIVE 8
#define  PERIOD_CONST_INTERMITTENT 9

// Scaling schemes applied to pitch and period
#define  SCALE_LINEAR 0
#define  SCALE_LOWEND 1
#define  SCALE_HIGHEND 2
#define  SCALE_LINEAR_NEG 3
#define  SCALE_LOWEND_NEG 4
#define  SCALE_HIGHEND_NEG 5

VEGA_SCHEME VegaSchemes[4]= {
  // Vega
  {X_NONE, Y_RELATIVE_MCCREADY_HALF,
   BEEPTYPE_LONG, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_INTERMITTENT, SCALE_LINEAR,
   BEEPTYPE_SHORTDOUBLE, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_INTERMITTENT, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_RELATIVE, SCALE_LINEAR, PERIOD_VARIO_RELATIVE, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_MEDIUM, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_CONTINUOUS, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_HI, SCALE_LINEAR},

  // Borgelt
  {X_AVERAGE, Y_RELATIVE_MCCREADY,
   BEEPTYPE_LONG, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_INTERMITTENT, SCALE_LINEAR,
   BEEPTYPE_SHORTDOUBLE, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_INTERMITTENT, SCALE_LINEAR,
   BEEPTYPE_MEDIUM, PITCH_VARIO_RELATIVE, SCALE_LINEAR, PERIOD_VARIO_RELATIVE, SCALE_LINEAR,
   BEEPTYPE_MEDIUM, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_LONG, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_CONTINUOUS, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_HI, SCALE_LINEAR},

  // Cambridge
  {X_NONE, Y_RELATIVE_ZERO, // should be net>zero
   BEEPTYPE_CONTINUOUS, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_HI, SCALE_LINEAR,
   BEEPTYPE_SHORTDOUBLE, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_LO, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_RELATIVE, SCALE_LINEAR, PERIOD_VARIO_RELATIVE, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_CONTINUOUS, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_HI, SCALE_LINEAR},

  // Zander
  {X_NONE, Y_RELATIVE_ZERO, // should be net>zero
   BEEPTYPE_CONTINUOUS, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_HI, SCALE_LINEAR,
   BEEPTYPE_SHORTDOUBLE, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_LO, SCALE_LINEAR,
   BEEPTYPE_CONTINUOUS, PITCH_VARIO_RELATIVE, SCALE_LINEAR, PERIOD_CONST_LO, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_LO, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_LO, SCALE_LINEAR,
   BEEPTYPE_LONG, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_MEDIUM, SCALE_LINEAR},

};

static void SetParametersScheme(int schemetype) {

  if(MessageBoxX(
		 gettext(TEXT("Set new audio scheme?  Old values will be lost.")),
		 gettext(TEXT("Vega Audio")),
		 MB_YESNO|MB_ICONQUESTION) != IDYES)
    return;


  VegaConfigurationUpdated(TEXT("ToneClimbComparisonType"), false, true,
			   VegaSchemes[schemetype].ToneClimbComparisonType);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftDetectionType"), false, true,
			   VegaSchemes[schemetype].ToneLiftComparisonType);

  VegaConfigurationUpdated(TEXT("ToneCruiseFasterBeepType"), false, true,
			   VegaSchemes[schemetype].ToneCruiseFasterBeepType);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPitchScheme"), false, true,
			   VegaSchemes[schemetype].ToneCruiseFasterPitchScheme);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPitchScale"), false, true,
			   VegaSchemes[schemetype].ToneCruiseFasterPitchScale);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPeriodScheme"), false, true,
			   VegaSchemes[schemetype].ToneCruiseFasterPeriodScheme);
  VegaConfigurationUpdated(TEXT("ToneCruiseFasterPeriodScale"), false, true,
			   VegaSchemes[schemetype].ToneCruiseFasterPeriodScale);

  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerBeepType"), false, true,
			   VegaSchemes[schemetype].ToneCruiseSlowerBeepType);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPitchScheme"), false, true,
			   VegaSchemes[schemetype].ToneCruiseSlowerPitchScheme);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPitchScale"), false, true,
			   VegaSchemes[schemetype].ToneCruiseSlowerPitchScale);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPeriodScheme"), false, true,
			   VegaSchemes[schemetype].ToneCruiseSlowerPeriodScheme);
  VegaConfigurationUpdated(TEXT("ToneCruiseSlowerPeriodScale"), false, true,
			   VegaSchemes[schemetype].ToneCruiseSlowerPeriodScale);

  VegaConfigurationUpdated(TEXT("ToneCruiseLiftBeepType"), false, true,
			   VegaSchemes[schemetype].ToneCruiseLiftBeepType);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPitchScheme"), false, true,
			   VegaSchemes[schemetype].ToneCruiseLiftPitchScheme);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPitchScale"), false, true,
			   VegaSchemes[schemetype].ToneCruiseLiftPitchScale);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPeriodScheme"), false, true,
			   VegaSchemes[schemetype].ToneCruiseLiftPeriodScheme);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftPeriodScale"), false, true,
			   VegaSchemes[schemetype].ToneCruiseLiftPeriodScale);

  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiBeepType"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingHiBeepType);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPitchScheme"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingHiPitchScheme);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPitchScale"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingHiPitchScale);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPeriodScheme"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingHiPeriodScheme);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingHiPeriodScale"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingHiPeriodScale);

  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowBeepType"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingLowBeepType);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPitchScheme"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingLowPitchScheme);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPitchScale"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingLowPitchScale);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPeriodScheme"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingLowPeriodScheme);
  VegaConfigurationUpdated(TEXT("ToneCirclingClimbingLowPeriodScale"), false, true,
			   VegaSchemes[schemetype].ToneCirclingClimbingLowPeriodScale);

  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingBeepType"), false, true,
			   VegaSchemes[schemetype].ToneCirclingDescendingBeepType);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPitchScheme"), false, true,
			   VegaSchemes[schemetype].ToneCirclingDescendingPitchScheme);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPitchScale"), false, true,
			   VegaSchemes[schemetype].ToneCirclingDescendingPitchScale);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPeriodScheme"), false, true,
			   VegaSchemes[schemetype].ToneCirclingDescendingPeriodScheme);
  VegaConfigurationUpdated(TEXT("ToneCirclingDescendingPeriodScale"), false, true,
			   VegaSchemes[schemetype].ToneCirclingDescendingPeriodScale);

  MessageBoxX (
	       gettext(TEXT("Audio scheme updated.")),
	       gettext(TEXT("Vega Audio")), MB_OK);

}


static void UpdateParametersScheme(bool first) {

  VegaConfigurationUpdated(TEXT("ToneClimbComparisonType"), first);
  VegaConfigurationUpdated(TEXT("ToneCruiseLiftDetectionType"), first);

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
  VegaConfigurationUpdated(TEXT("CalibrationTEProbe"), first);

  VegaConfigurationUpdated(TEXT("AccelerometerSlopeX"), first);
  VegaConfigurationUpdated(TEXT("AccelerometerSlopeY"), first);
  VegaConfigurationUpdated(TEXT("AccelerometerOffsetX"), first);
  VegaConfigurationUpdated(TEXT("AccelerometerOffsetY"), first);

  //  VegaConfigurationUpdated(TEXT("PDAPower"), first);
  VegaConfigurationUpdated(TEXT("ToneAveragerVarioTimeScale"), first);
  VegaConfigurationUpdated(TEXT("ToneAveragerCruiseTimeScale"), first);
  VegaConfigurationUpdated(TEXT("ToneMeanVolumeCircling"), first);
  VegaConfigurationUpdated(TEXT("ToneMeanVolumeCruise"), first);
  VegaConfigurationUpdated(TEXT("ToneBaseFrequencyOffset"), first);
  VegaConfigurationUpdated(TEXT("TonePitchScale"), first);

  VegaConfigurationUpdated(TEXT("ToneDeadbandCirclingType"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCirclingHigh"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCirclingLow"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCruiseType"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCruiseHigh"), first);
  VegaConfigurationUpdated(TEXT("ToneDeadbandCruiseLow"), first);

  UpdateParametersScheme(first);

  VegaConfigurationUpdated(TEXT("VarioTimeConstantCircling"), first);
  VegaConfigurationUpdated(TEXT("VarioTimeConstantCruise"), first);

  VegaConfigurationUpdated(TEXT("UTCOffset"), first);
  VegaConfigurationUpdated(TEXT("IGCLoging"), first);
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
  VegaConfigurationUpdated(TEXT("DontReportTraficModeChanges"), first);
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

  VegaConfigurationUpdated(TEXT("VelocityNeverExceed"), first);
  VegaConfigurationUpdated(TEXT("VelocitySafeTerrain"), first);
  VegaConfigurationUpdated(TEXT("TerrainSafetyHeight"), first);
  VegaConfigurationUpdated(TEXT("VelocityManoeuvering"), first);
  VegaConfigurationUpdated(TEXT("VelocityAirbrake"), first);
  VegaConfigurationUpdated(TEXT("VelocityFlap"), first);
  VegaConfigurationUpdated(TEXT("LedBrightness"), first);

  VegaConfigurationUpdated(TEXT("BaudRateA"), first);

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
  case 16:
    wf->SetCaption(TEXT("17 Airframe Limits"));
    break;
  case 17:
    wf->SetCaption(TEXT("18 Audio Schemes"));
    break;
  case 18:
    wf->SetCaption(TEXT("19 Display"));
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
  wConfig17->SetVisible(page == 16);
  wConfig18->SetVisible(page == 17);
  wConfig19->SetVisible(page == 18);

  UpdateParameters(false);

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
  UpdateParameters(false);  // 20060801:sgi make shure changes are
                            // sent to device
  wf->SetModalResult(mrOK);
}

static void OnSaveClicked(WindowControl * Sender){
	(void)Sender;
  UpdateParameters(false);  // 20060801:sgi make shure changes are
                            // sent to device
  VarioWriteNMEA(TEXT("PDVSC,S,StoreToEeprom,2"));

  if (!is_simulator())
    Sleep(500);
}

static void OnDemoClicked(WindowControl * Sender){
	(void)Sender;
  // retrieve changes from form
  UpdateParameters(false);
  dlgVegaDemoShowModal();
}


static void OnSchemeVegaClicked(WindowControl * Sender){
	(void)Sender;
  SetParametersScheme(0);
}

static void OnSchemeBorgeltClicked(WindowControl * Sender){
	(void)Sender;
  SetParametersScheme(1);
}

static void OnSchemeCambridgeClicked(WindowControl * Sender){
	(void)Sender;
  SetParametersScheme(2);
}

static void OnSchemeZanderClicked(WindowControl * Sender){
	(void)Sender;
  SetParametersScheme(3);
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
	(void)lParam; (void)Sender;
  switch(wParam & 0xffff){
    // JMW NO! This disables editing! //   case VK_LEFT:
    case '6':
      ((WndButton *)wf->FindByName(TEXT("cmdPrev")))->set_focus();
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    // JMW NO! This disables editing!  //  case VK_RIGHT:
    case '7':
      ((WndButton *)wf->FindByName(TEXT("cmdNext")))->set_focus();
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnDemoClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnSchemeVegaClicked),
  DeclareCallBackEntry(OnSchemeBorgeltClicked),
  DeclareCallBackEntry(OnSchemeCambridgeClicked),
  DeclareCallBackEntry(OnSchemeZanderClicked),
  DeclareCallBackEntry(NULL)
};


static void FillAudioEnums(const TCHAR* name) {
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


static void FillAllAudioEnums(void) {
  FillAudioEnums(TEXT("CruiseFaster"));
  FillAudioEnums(TEXT("CruiseSlower"));
  FillAudioEnums(TEXT("CruiseLift"));
  FillAudioEnums(TEXT("CirclingClimbingHi"));
  FillAudioEnums(TEXT("CirclingClimbingLow"));
  FillAudioEnums(TEXT("CirclingDescending"));
}

static void FillEnums(void) {
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpBaudRateA"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Auto"));
    dfe->addEnumText(TEXT("4800"));
    dfe->addEnumText(TEXT("9600"));
    dfe->addEnumText(TEXT("19200"));
    dfe->addEnumText(TEXT("38400"));
    dfe->addEnumText(TEXT("57600"));
    dfe->addEnumText(TEXT("115200"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpHasTemperature"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("OFF"));
    dfe->addEnumText(TEXT("ON"));
    dfe->addEnumText(TEXT("AUTO"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpToneClimbComparisonType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("None"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpVarioTimeConstantCircling"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT(" 1.0s"));
    dfe->addEnumText(TEXT(" 1.3s"));
    dfe->addEnumText(TEXT(" 1.8s"));
    dfe->addEnumText(TEXT(" 2.7s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVarioTimeConstantCruise"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT(" 1.0s"));
    dfe->addEnumText(TEXT(" 1.3s"));
    dfe->addEnumText(TEXT(" 1.8s"));
    dfe->addEnumText(TEXT(" 2.7s"));
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

  FillAllAudioEnums();

}


bool dlgConfigurationVarioShowModal(void){

  changed = false;

  if (!is_simulator() && devVarioFindVega() == NULL) {
    MessageBoxX (
		 gettext(TEXT("No communication with Vega.")),
		 gettext(TEXT("Vega error")), MB_OK);
    return false;
  }

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgVario_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_VARIO_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgVario.xml"),
			XCSoarInterface::main_window,
			TEXT("IDR_XML_VARIO"));
  }

  if (!wf) return false;

  wf->SetKeyDownNotify(FormKeyDown);


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
  wConfig17    = ((WndFrame *)wf->FindByName(TEXT("frmLimits")));
  wConfig18    = ((WndFrame *)wf->FindByName(TEXT("frmSchemes")));
  wConfig19    = ((WndFrame *)wf->FindByName(TEXT("frmDisplay")));

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

  // populate enums

  FillEnums();

  XCSoarInterface::CreateProgressDialog(gettext(TEXT("Reading vario settings...")));
  // Need step size finer than default 10
  XCSoarInterface::SetProgressStepSize(2);

  UpdateParameters(true);

  XCSoarInterface::CloseProgressDialog();

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

