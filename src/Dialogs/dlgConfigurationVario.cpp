/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/Vega.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Tabbed.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Device/device.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Profile/Profile.hpp"
#include "DataField/Enum.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"
#include "Compiler.h"
#include "OS/Sleep.h"
#include "Util/Macros.hpp"

#include <assert.h>
#include <string.h>

static const TCHAR *const captions[] = {
  _T(" 1 Hardware"),
  _T(" 2 Calibration"),
  _T(" 3 Audio Modes"),
  _T(" 4 Deadband"),
  _T(" 5 Tones: Cruise Faster"),
  _T(" 6 Tones: Cruise Slower"),
  _T(" 7 Tones: Cruise in Lift"),
  _T(" 8 Tones: Circling, climbing fast"),
  _T(" 9 Tones: Circling, climbing slow"),
  _T("10 Tones: Circling, descending"),
  _T("11 Vario flight logger"),
  _T("12 Audio mixer"),
  _T("13 FLARM Alerts"),
  _T("14 FLARM Identification"),
  _T("15 FLARM Repeats"),
  _T("16 Alerts"),
  _T("17 Airframe Limits"),
  _T("18 Audio Schemes"),
  _T("19 Display"),
};

static const TCHAR *const beep_types[] = {
  _T("Silence"),
  _T("Short"),
  _T("Medium"),
  _T("Long"),
  _T("Continuous"),
  _T("Short double"),
  NULL
};

static const TCHAR *const pitch_schemes[] = {
  _T("Constant high"),
  _T("Constant medium"),
  _T("Constant low"),
  _T("Speed percent"),
  _T("Speed error"),
  _T("Vario gross"),
  _T("Vario net"),
  _T("Vario relative"),
  _T("Vario gross/relative"),
  NULL
};

static const TCHAR *const period_schemes[] = {
  _T("Constant high"),
  _T("Constant medium"),
  _T("Constant low"),
  _T("Speed percent"),
  _T("Speed error"),
  _T("Vario gross"),
  _T("Vario net"),
  _T("Vario relative"),
  _T("Vario gross/relative"),
  _T("Intermittent"),
  NULL
};

static const TCHAR *const scales[] = {
  _T("+Linear"),
  _T("+Low end"),
  _T("+High end"),
  _T("-Linear"),
  _T("-Low end"),
  _T("-High end"),
  NULL
};

static const TCHAR *const flarm_user_interfaces[] = {
  _T("LED+Buzzer"),
  _T("None"),
  _T("Buzzer"),
  _T("LED"),
  NULL
};

static const TCHAR *const flarm_aircraft_types[] = {
  _T("Undefined"),
  _T("Glider"),
  _T("Tow plane"),
  _T("Helicopter"),
  _T("Parachute"),
  _T("Drop plane"),
  _T("Fixed hangglider"),
  _T("Soft paraglider"),
  _T("Powered aircraft"),
  _T("Jet aircraft"),
  _T("UFO"),
  _T("Baloon"),
  _T("Blimp, Zeppelin"),
  _T("UAV (Drone)"),
  _T("Static"),
  NULL
};

static const TCHAR *const needle_gauge_types[] = {
  _T("None"),
  _T("LX"),
  _T("Analog"),
  NULL
};

static const char *const vega_setting_names[] = {
  "HasPressureTE",
  "HasPressurePitot",
  "HasPressureStatic",
  "HasPressureStall",
  "HasAccelerometer",
  "HasTemperature",
  "FlarmConnected",

  "TotalEnergyMixingRatio",
  "CalibrationAirSpeed",
  "CalibrationTEStatic",
  "CalibrationTEDynamic",
  "CalibrationTEProbe",

  //  "PDAPower",
  "ToneAveragerVarioTimeScale",
  "ToneAveragerCruiseTimeScale",
  "ToneMeanVolumeCircling",
  "ToneMeanVolumeCruise",
  "ToneBaseFrequencyOffset",
  "TonePitchScale",

  "ToneDeadbandCirclingType",
  "ToneDeadbandCirclingHigh",
  "ToneDeadbandCirclingLow",
  "ToneDeadbandCruiseType",
  "ToneDeadbandCruiseHigh",
  "ToneDeadbandCruiseLow",

  "ToneClimbComparisonType",
  "ToneCruiseLiftDetectionType",

  "ToneCruiseFasterBeepType",
  "ToneCruiseFasterPitchScheme",
  "ToneCruiseFasterPitchScale",
  "ToneCruiseFasterPeriodScheme",
  "ToneCruiseFasterPeriodScale",

  "ToneCruiseSlowerBeepType",
  "ToneCruiseSlowerPitchScheme",
  "ToneCruiseSlowerPitchScale",
  "ToneCruiseSlowerPeriodScheme",
  "ToneCruiseSlowerPeriodScale",

  "ToneCruiseLiftBeepType",
  "ToneCruiseLiftPitchScheme",
  "ToneCruiseLiftPitchScale",
  "ToneCruiseLiftPeriodScheme",
  "ToneCruiseLiftPeriodScale",

  "ToneCirclingClimbingHiBeepType",
  "ToneCirclingClimbingHiPitchScheme",
  "ToneCirclingClimbingHiPitchScale",
  "ToneCirclingClimbingHiPeriodScheme",
  "ToneCirclingClimbingHiPeriodScale",

  "ToneCirclingClimbingLowBeepType",
  "ToneCirclingClimbingLowPitchScheme",
  "ToneCirclingClimbingLowPitchScale",
  "ToneCirclingClimbingLowPeriodScheme",
  "ToneCirclingClimbingLowPeriodScale",

  "ToneCirclingDescendingBeepType",
  "ToneCirclingDescendingPitchScheme",
  "ToneCirclingDescendingPitchScale",
  "ToneCirclingDescendingPeriodScheme",
  "ToneCirclingDescendingPeriodScale",

  "VarioTimeConstantCircling",
  "VarioTimeConstantCruise",

  "UTCOffset",
  "IGCLoging",
  "IGCLoggerInterval",
  "MuteVarioOnPlay",
  "MuteVarioOnCom",
  "VarioRelativeMuteVol",
  "VoiceRelativeMuteVol",
  "MuteComSpkThreshold",
  "MuteComPhnThreshold",
  "MinUrgentVolume",
  "FlarmMaxObjectsReported",
  "FlarmMaxObjectsReportedOnCircling",
  "FlarmUserInterface",
  "KeepOnStraightFlightMode",
  "DontReportTraficModeChanges",
  "DontReportGliderType",
  "FlarmPrivacyFlag",
  "FlarmAircraftType",

  "FlarmInfoRepeatTime",
  "FlarmCautionRepeatTime",
  "FlarmWarningRepeatTime",
  "GearOnDelay",
  "GearOffDelay",
  "GearRepeatTime",
  "PlyMaxComDelay",
  "BatLowDelay",
  "BatEmptyDelay",
  "BatRepeatTime",

  "NeedleGaugeType",

  "VelocityNeverExceed",
  "VelocitySafeTerrain",
  "TerrainSafetyHeight",
  "VelocityManoeuvering",
  "VelocityAirbrake",
  "VelocityFlap",
  "LedBrightness",

  "BaudRateA",

  NULL
};

static VegaDevice *device;
static bool changed = false, dirty = false;
static WndForm *wf = NULL;
static TabbedControl *tabbed;

static WndProperty *
GetSettingControl(const char *name)
{
#ifdef _UNICODE
  TCHAR tname[64];
  if (MultiByteToWideChar(CP_UTF8, 0, name, -1, tname, ARRAY_SIZE(tname)) <= 0)
    return false;
#else
  const char *tname = name;
#endif

  TCHAR buffer[64] = _T("prp");
  _tcscat(buffer, tname);
  return (WndProperty *)wf->FindByName(buffer);
}

static bool
GetSettingValue(const char *name, int &value_r)
{
#ifdef _UNICODE
  TCHAR tname[64];
  if (MultiByteToWideChar(CP_UTF8, 0, name, -1, tname, ARRAY_SIZE(tname)) <= 0)
    return false;
#else
  const char *tname = name;
#endif

  TCHAR key[64] = _T("Vega");
  _tcscat(key, tname);
  return Profile::Get(key, value_r);
}

static void
SetSettingValue(const char *name, int value)
{
#ifdef _UNICODE
  TCHAR tname[64];
  if (MultiByteToWideChar(CP_UTF8, 0, name, -1, tname, ARRAY_SIZE(tname)) <= 0)
    return;
#else
  const char *tname = name;
#endif

  TCHAR key[64] = _T("Vega");
  _tcscat(key, tname);
  Profile::Set(key, value);
}

gcc_pure
static unsigned
GetSettingUpdated(const char *name)
{
#ifdef _UNICODE
  TCHAR tname[64];
  if (MultiByteToWideChar(CP_UTF8, 0, name, -1, tname, ARRAY_SIZE(tname)) <= 0)
    return false;
#else
  const char *tname = name;
#endif

  TCHAR key[64] = _T("Vega");
  _tcscat(key, tname);
  _tcscat(key, _T("Updated"));
  unsigned updated = 0;
  Profile::Get(key, updated);
  return updated;
}

static void
SetSettingUpdated(const char *name, unsigned updated)
{
#ifdef _UNICODE
  TCHAR tname[64];
  if (MultiByteToWideChar(CP_UTF8, 0, name, -1, tname, ARRAY_SIZE(tname)) <= 0)
    return;
#else
  const char *tname = name;
#endif

  TCHAR key[64] = _T("Vega");
  _tcscat(key, tname);
  _tcscat(key, _T("Updated"));
  Profile::Set(key, updated);
}

static bool
VegaConfigurationUpdated(const char *name, bool first, bool setvalue = false,
                         int ext_setvalue = 0)
{
  WndProperty *wp = GetSettingControl(name);

  if (first) {
    SetSettingUpdated(name, 0);
    // we are not ready, haven't received value from vario
    // (do request here)
    if (!device->RequestSetting(name))
      return false;

    if (!is_simulator())
      Sleep(250);
  }

  if (setvalue) {
    if (wp) {
      wp->GetDataField()->SetAsInteger((int)ext_setvalue);
      wp->RefreshDisplay();
    }

    if (!device->SendSetting(name, ext_setvalue))
      return false;

    if (!is_simulator())
      Sleep(250);

    return true;
  }

  int lvalue;
  if (!GetSettingValue(name, lvalue)) {
    // vario hasn't set the value in the registry yet,
    // so no sensible defaults
    return false;
  }

  // hack, fix the -1 (plug and play settings)
  if (strcmp(name, "HasTemperature") == 0) {
    if (lvalue >= 255)
      lvalue = 2;
  }

  if (first) {
    // at start, set from last known registry value, this
    // helps if variables haven't been modified.
    SetSettingUpdated(name, 2);

    if (wp) {
      wp->GetDataField()->SetAsInteger(lvalue);
      wp->RefreshDisplay();
    }

    return false;
  }

  const unsigned updated = GetSettingUpdated(name);
  if (updated == 1) {
    // value is updated externally, so set the property and can proceed
    // to editing values
    SetSettingUpdated(name, 2);

    if (wp) {
      wp->GetDataField()->SetAsInteger(lvalue);
      wp->RefreshDisplay();
    }
  } else if (updated == 2) {
    if (wp) {
      int newval = wp->GetDataField()->GetAsInteger();
      if (newval != lvalue) {
        // value has changed
        SetSettingUpdated(name, 2);
        SetSettingValue(name, newval);

        changed = dirty = true;

        // maybe represent all as text?
        // note that this code currently won't work for ints

        // hack, fix the -1 (plug and play settings)
        if (strcmp(name, "HasTemperature") == 0) {
          if (newval == 2)
            newval = 255;
        }

        if (!device->SendSetting(name, newval))
          return false;

        if (!is_simulator())
          Sleep(250);

        return true;
      }
    }
  }

  return false;
}

static void
UpdateControls(const char *const*names, bool first)
{
  for (; *names != NULL; ++names)
    VegaConfigurationUpdated(*names, first);
}

struct VEGA_SCHEME
{
  int ToneClimbComparisonType;
  int ToneLiftComparisonType;

  int ToneCruiseFasterBeepType;
  int ToneCruiseFasterPitchScheme;
  int ToneCruiseFasterPitchScale;
  int ToneCruiseFasterPeriodScheme;
  int ToneCruiseFasterPeriodScale;

  int ToneCruiseSlowerBeepType;
  int ToneCruiseSlowerPitchScheme;
  int ToneCruiseSlowerPitchScale;
  int ToneCruiseSlowerPeriodScheme;
  int ToneCruiseSlowerPeriodScale;

  int ToneCruiseLiftBeepType;
  int ToneCruiseLiftPitchScheme;
  int ToneCruiseLiftPitchScale;
  int ToneCruiseLiftPeriodScheme;
  int ToneCruiseLiftPeriodScale;

  int ToneCirclingClimbingHiBeepType;
  int ToneCirclingClimbingHiPitchScheme;
  int ToneCirclingClimbingHiPitchScale;
  int ToneCirclingClimbingHiPeriodScheme;
  int ToneCirclingClimbingHiPeriodScale;

  int ToneCirclingClimbingLowBeepType;
  int ToneCirclingClimbingLowPitchScheme;
  int ToneCirclingClimbingLowPitchScale;
  int ToneCirclingClimbingLowPeriodScheme;
  int ToneCirclingClimbingLowPeriodScale;

  int ToneCirclingDescendingBeepType;
  int ToneCirclingDescendingPitchScheme;
  int ToneCirclingDescendingPitchScale;
  int ToneCirclingDescendingPeriodScheme;
  int ToneCirclingDescendingPeriodScale;

};

// Value used for comparison in climb tone
#define  X_NONE 0
#define  X_MACCREADY 1
#define  X_AVERAGE 2

// Condition for detecting lift in cruise mode
#define  Y_NONE 0
#define  Y_RELATIVE_ZERO 1
#define  Y_RELATIVE_MACCREADY_HALF 2
#define  Y_GROSS_ZERO 3
#define  Y_NET_MACCREADY_HALF 4
#define  Y_RELATIVE_MACCREADY 5
#define  Y_NET_MACCREADY 6

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
  {X_NONE, Y_RELATIVE_MACCREADY_HALF,
   BEEPTYPE_LONG, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_INTERMITTENT, SCALE_LINEAR,
   BEEPTYPE_SHORTDOUBLE, PITCH_SPEED_ERROR, SCALE_LINEAR, PERIOD_CONST_INTERMITTENT, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_RELATIVE, SCALE_LINEAR, PERIOD_VARIO_RELATIVE, SCALE_LINEAR,
   BEEPTYPE_SHORT, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_MEDIUM, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_VARIO_GROSS, SCALE_LINEAR,
   BEEPTYPE_CONTINUOUS, PITCH_VARIO_GROSS, SCALE_LINEAR, PERIOD_CONST_HI, SCALE_LINEAR},

  // Borgelt
  {X_AVERAGE, Y_RELATIVE_MACCREADY,
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

static void
SetParametersScheme(int schemetype)
{
  if(MessageBoxX(_("Set new audio scheme?  Old values will be lost."),
                 _("Vega Audio"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;


  VegaConfigurationUpdated("ToneClimbComparisonType", false, true,
                           VegaSchemes[schemetype].ToneClimbComparisonType);
  VegaConfigurationUpdated("ToneCruiseLiftDetectionType", false, true,
                           VegaSchemes[schemetype].ToneLiftComparisonType);

  VegaConfigurationUpdated("ToneCruiseFasterBeepType", false, true,
                           VegaSchemes[schemetype].ToneCruiseFasterBeepType);
  VegaConfigurationUpdated("ToneCruiseFasterPitchScheme", false, true,
                           VegaSchemes[schemetype].ToneCruiseFasterPitchScheme);
  VegaConfigurationUpdated("ToneCruiseFasterPitchScale", false, true,
                           VegaSchemes[schemetype].ToneCruiseFasterPitchScale);
  VegaConfigurationUpdated("ToneCruiseFasterPeriodScheme", false, true,
                           VegaSchemes[schemetype].ToneCruiseFasterPeriodScheme);
  VegaConfigurationUpdated("ToneCruiseFasterPeriodScale", false, true,
                           VegaSchemes[schemetype].ToneCruiseFasterPeriodScale);

  VegaConfigurationUpdated("ToneCruiseSlowerBeepType", false, true,
                           VegaSchemes[schemetype].ToneCruiseSlowerBeepType);
  VegaConfigurationUpdated("ToneCruiseSlowerPitchScheme", false, true,
                           VegaSchemes[schemetype].ToneCruiseSlowerPitchScheme);
  VegaConfigurationUpdated("ToneCruiseSlowerPitchScale", false, true,
                           VegaSchemes[schemetype].ToneCruiseSlowerPitchScale);
  VegaConfigurationUpdated("ToneCruiseSlowerPeriodScheme", false, true,
                           VegaSchemes[schemetype].ToneCruiseSlowerPeriodScheme);
  VegaConfigurationUpdated("ToneCruiseSlowerPeriodScale", false, true,
                           VegaSchemes[schemetype].ToneCruiseSlowerPeriodScale);

  VegaConfigurationUpdated("ToneCruiseLiftBeepType", false, true,
                           VegaSchemes[schemetype].ToneCruiseLiftBeepType);
  VegaConfigurationUpdated("ToneCruiseLiftPitchScheme", false, true,
                           VegaSchemes[schemetype].ToneCruiseLiftPitchScheme);
  VegaConfigurationUpdated("ToneCruiseLiftPitchScale", false, true,
                           VegaSchemes[schemetype].ToneCruiseLiftPitchScale);
  VegaConfigurationUpdated("ToneCruiseLiftPeriodScheme", false, true,
                           VegaSchemes[schemetype].ToneCruiseLiftPeriodScheme);
  VegaConfigurationUpdated("ToneCruiseLiftPeriodScale", false, true,
                           VegaSchemes[schemetype].ToneCruiseLiftPeriodScale);

  VegaConfigurationUpdated("ToneCirclingClimbingHiBeepType", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingHiBeepType);
  VegaConfigurationUpdated("ToneCirclingClimbingHiPitchScheme", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingHiPitchScheme);
  VegaConfigurationUpdated("ToneCirclingClimbingHiPitchScale", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingHiPitchScale);
  VegaConfigurationUpdated("ToneCirclingClimbingHiPeriodScheme", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingHiPeriodScheme);
  VegaConfigurationUpdated("ToneCirclingClimbingHiPeriodScale", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingHiPeriodScale);

  VegaConfigurationUpdated("ToneCirclingClimbingLowBeepType", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingLowBeepType);
  VegaConfigurationUpdated("ToneCirclingClimbingLowPitchScheme", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingLowPitchScheme);
  VegaConfigurationUpdated("ToneCirclingClimbingLowPitchScale", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingLowPitchScale);
  VegaConfigurationUpdated("ToneCirclingClimbingLowPeriodScheme", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingLowPeriodScheme);
  VegaConfigurationUpdated("ToneCirclingClimbingLowPeriodScale", false, true,
                           VegaSchemes[schemetype].ToneCirclingClimbingLowPeriodScale);

  VegaConfigurationUpdated("ToneCirclingDescendingBeepType", false, true,
                           VegaSchemes[schemetype].ToneCirclingDescendingBeepType);
  VegaConfigurationUpdated("ToneCirclingDescendingPitchScheme", false, true,
                           VegaSchemes[schemetype].ToneCirclingDescendingPitchScheme);
  VegaConfigurationUpdated("ToneCirclingDescendingPitchScale", false, true,
                           VegaSchemes[schemetype].ToneCirclingDescendingPitchScale);
  VegaConfigurationUpdated("ToneCirclingDescendingPeriodScheme", false, true,
                           VegaSchemes[schemetype].ToneCirclingDescendingPeriodScheme);
  VegaConfigurationUpdated("ToneCirclingDescendingPeriodScale", false, true,
                           VegaSchemes[schemetype].ToneCirclingDescendingPeriodScale);

  MessageBoxX(_("Audio scheme updated."),
              _("Vega Audio"), MB_OK);
}

static void
UpdateParameters(bool first)
{
  UpdateControls(vega_setting_names, first);
}

static void
UpdateCaption()
{
  wf->SetCaption(captions[tabbed->GetCurrentPage()]);
}

static void
PageSwitched()
{
  UpdateCaption();
  UpdateParameters(false);
}

static void
OnNextClicked(gcc_unused WndButton &Sender)
{
  tabbed->NextPage();
  PageSwitched();
}

static void
OnPrevClicked(gcc_unused WndButton &Sender)
{
  tabbed->PreviousPage();
  PageSwitched();
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  UpdateParameters(false);
  // make sure changes are sent to device
  wf->SetModalResult(mrOK);
}

static void
OnSaveClicked(gcc_unused WndButton &Sender)
{
  UpdateParameters(false);
  // make sure changes are sent to device
  if (dirty && device->SendSetting("StoreToEeprom", 2))
    dirty = false;

  if (!is_simulator())
    Sleep(500);
}

static void
OnDemoClicked(gcc_unused WndButton &Sender)
{
  // retrieve changes from form
  UpdateParameters(false);
  dlgVegaDemoShowModal();
}

static void
OnSchemeVegaClicked(gcc_unused WndButton &Sender)
{
  SetParametersScheme(0);
}

static void
OnSchemeBorgeltClicked(gcc_unused WndButton &Sender)
{
  SetParametersScheme(1);
}

static void
OnSchemeCambridgeClicked(gcc_unused WndButton &Sender)
{
  SetParametersScheme(2);
}

static void
OnSchemeZanderClicked(gcc_unused WndButton &Sender)
{
  SetParametersScheme(3);
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    tabbed->PreviousPage();
    PageSwitched();
    //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    tabbed->NextPage();
    PageSwitched();
    //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

static void
FillAudioEnums(const TCHAR* name)
{
  WndProperty *wp;
  TCHAR fullname[100];

  _stprintf(fullname, _T("prpTone%sBeepType"), name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(beep_types);
    wp->RefreshDisplay();
  }

  _stprintf(fullname, _T("prpTone%sPitchScheme"), name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(pitch_schemes);
    wp->RefreshDisplay();
  }

  _stprintf(fullname, _T("prpTone%sPeriodScheme"), name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(period_schemes);
    wp->RefreshDisplay();
  }

  _stprintf(fullname, _T("prpTone%sPitchScale"), name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(scales);
    wp->RefreshDisplay();
  }

  _stprintf(fullname, _T("prpTone%sPeriodScale"), name);
  wp = (WndProperty*)wf->FindByName(fullname);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(scales);
    wp->RefreshDisplay();
  }
}

static void
FillAllAudioEnums(void)
{
  FillAudioEnums(_T("CruiseFaster"));
  FillAudioEnums(_T("CruiseSlower"));
  FillAudioEnums(_T("CruiseLift"));
  FillAudioEnums(_T("CirclingClimbingHi"));
  FillAudioEnums(_T("CirclingClimbingLow"));
  FillAudioEnums(_T("CirclingDescending"));
}

static void
FillEnums(void)
{
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpBaudRateA"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Auto"));
    dfe->addEnumText(_T("4800"));
    dfe->addEnumText(_T("9600"));
    dfe->addEnumText(_T("19200"));
    dfe->addEnumText(_T("38400"));
    dfe->addEnumText(_T("57600"));
    dfe->addEnumText(_T("115200"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpHasTemperature"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Off"));
    dfe->addEnumText(_T("On"));
    dfe->addEnumText(_T("AUTO"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpToneClimbComparisonType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("None"));
    dfe->addEnumText(_T("Gross>MacCready"));
    dfe->addEnumText(_T("Gross>Average"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpToneCruiseLiftDetectionType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Disabled"));
    dfe->addEnumText(_T("Relative>0"));
    dfe->addEnumText(_T("Relative>MacCready/2"));
    dfe->addEnumText(_T("Gross>0"));
    dfe->addEnumText(_T("Net>MacCready/2"));
    dfe->addEnumText(_T("Relative>MacCready"));
    dfe->addEnumText(_T("Net>MacCready"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpVarioTimeConstantCircling"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T(" 1.0s"));
    dfe->addEnumText(_T(" 1.3s"));
    dfe->addEnumText(_T(" 1.8s"));
    dfe->addEnumText(_T(" 2.7s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpVarioTimeConstantCruise"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T(" 1.0s"));
    dfe->addEnumText(_T(" 1.3s"));
    dfe->addEnumText(_T(" 1.8s"));
    dfe->addEnumText(_T(" 2.7s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpToneAveragerVarioTimeScale"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T(" 0.0s"));
    dfe->addEnumText(_T(" 0.8s"));
    dfe->addEnumText(_T(" 1.7s"));
    dfe->addEnumText(_T(" 3.5s"));
    dfe->addEnumText(_T(" 7.5s"));
    dfe->addEnumText(_T("15.0s"));
    dfe->addEnumText(_T("30.0s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpToneAveragerCruiseTimeScale"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T(" 0.0s"));
    dfe->addEnumText(_T(" 0.8s"));
    dfe->addEnumText(_T(" 1.7s"));
    dfe->addEnumText(_T(" 3.5s"));
    dfe->addEnumText(_T(" 7.5s"));
    dfe->addEnumText(_T("15.0s"));
    dfe->addEnumText(_T("30.0s"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpToneDeadbandCirclingType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Step"));
    dfe->addEnumText(_T("Ramp"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpToneDeadbandCruiseType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Step"));
    dfe->addEnumText(_T("Ramp"));
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlarmUserInterface"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(flarm_user_interfaces);
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlarmAircraftType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(flarm_aircraft_types);
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpNeedleGaugeType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumTexts(needle_gauge_types);
    dfe->Set(0);
    wp->RefreshDisplay();
  }

  FillAllAudioEnums();
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnDemoClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnSchemeVegaClicked),
  DeclareCallBackEntry(OnSchemeBorgeltClicked),
  DeclareCallBackEntry(OnSchemeCambridgeClicked),
  DeclareCallBackEntry(OnSchemeZanderClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgConfigurationVarioShowModal(Device &_device)
{
  device = (VegaDevice *)&_device;
  changed = false;

  if (!is_simulator() && devVarioFindVega() == NULL) {
    MessageBoxX(_("No communication with Vega."), _("Vega error"), MB_OK);
    return false;
  }

  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape
                  ? _T("IDR_XML_VARIO_L") : _T("IDR_XML_VARIO_L"));
  if (!wf)
    return false;

  wf->SetKeyDownNotify(FormKeyDown);

  tabbed = ((TabbedControl *)wf->FindByName(_T("tabbed")));
  assert(tabbed != NULL);

  UpdateCaption();

  // populate enums

  FillEnums();

  UpdateParameters(true);

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
