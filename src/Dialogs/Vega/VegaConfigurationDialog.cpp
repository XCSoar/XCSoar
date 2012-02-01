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

#include "VegaDialogs.hpp"
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
  "VelocityAirBrake",
  "VelocityFlap",
  "LedBrightness",

  "BaudrateA",

  NULL
};

/**
 * This array contains the values that were filled into the form, and
 * is later used to check if the user has changed anything.  Only
 * modified values will be sent to the Vega.
 */
static int vega_setting_values[ARRAY_SIZE(vega_setting_names) - 1];

static VegaDevice *device;
static bool changed, dirty;
static WndForm *wf = NULL;
static TabbedControl *tabbed;

static WndProperty &
GetSettingControl(const char *name)
{
#ifdef _UNICODE
  TCHAR tname[64];
  MultiByteToWideChar(CP_UTF8, 0, name, -1, tname, ARRAY_SIZE(tname));
#else
  const char *tname = name;
#endif

  TCHAR buffer[64] = _T("prp");
  _tcscat(buffer, tname);
  WndProperty *control = (WndProperty *)wf->FindByName(buffer);
  assert(control != NULL);
  return *control;
}

static bool
GetSettingValue(const char *name, int &value_r)
{
  const auto x = device->GetSetting(name);
  if (!x.first)
    return false;

  value_r = x.second;
  return true;
}

static void
InitControl(const char *name, int &value_r)
{
  int lvalue;
  if (!GetSettingValue(name, lvalue)) {
    // vario hasn't set the value in the registry yet,
    // so no sensible defaults
    return;
  }

  // at start, set from last known registry value, this
  // helps if variables haven't been modified.
  value_r = lvalue;

  WndProperty &wp = GetSettingControl(name);
  wp.GetDataField()->SetAsInteger(lvalue);
  wp.RefreshDisplay();
}

static bool
SetControl(const char *name, int value)
{
  WndProperty &wp = GetSettingControl(name);
  wp.GetDataField()->SetAsInteger(value);
  wp.RefreshDisplay();

  if (!device->SendSetting(name, value))
    return false;

  return true;
}

static bool
UpdateControl(const char *name, int &value_r)
{
  WndProperty &wp = GetSettingControl(name);
  const int ui_value = wp.GetDataField()->GetAsInteger();

  int device_value;
  if (ui_value != value_r) {
    /* value has been changed by the user */
    if (!device->SendSetting(name, ui_value))
      return false;

    changed = dirty = true;
    value_r = ui_value;

    return true;
  } else if (GetSettingValue(name, device_value) &&
             device_value != ui_value) {
    /* value has been changed by the Vega */
    value_r = device_value;

    wp.GetDataField()->SetAsInteger(device_value);
    wp.RefreshDisplay();

    return false;
  } else
    return false;
}

gcc_pure
static bool
SettingExists(const char *name)
{
  return device->GetSetting(name).first;
}

/**
 * Wait for a setting to be received from the Vega.
 */
static bool
WaitForSetting(const char *name, unsigned timeout_ms)
{
  for (unsigned i = 0; i < timeout_ms / 100; ++i) {
    if (SettingExists(name))
      return true;
    Sleep(100);
  }

  return false;
}

static bool
RequestAll(const char *const*names)
{
  assert(names != NULL);
  assert(*names != NULL);

  /* long timeout for first response */
  unsigned timeout_ms = 3000;

  /* the first response that we're still waiting for */
  const char *const*start = names;

  for (const char *const*i = names; *i != NULL; ++i) {
    /* send up to 4 requests at a time */
    if (i - start >= 4) {
      /* queue is long enough: wait for one response */
      WaitForSetting(*start, timeout_ms);

      /* reduce timeout for follow-up responses */
      timeout_ms = 1000;

      ++start;
    }

    if (!SettingExists(*i) && !device->RequestSetting(*i))
      return false;
  }

  /* wait for the remaining responses */
  for (const char *const*i = start; *i != NULL; ++i)
    WaitForSetting(*i, 500);

  return true;
}

static void
InitControls(const char *const*names, int *values)
{
  RequestAll(names);

  for (; *names != NULL; ++names, ++values)
    InitControl(*names, *values);
}

static void
UpdateControls(const char *const*names, int *values)
{
  for (; *names != NULL; ++names, ++values)
    UpdateControl(*names, *values);
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

  SetControl("ToneClimbComparisonType",
             VegaSchemes[schemetype].ToneClimbComparisonType);
  SetControl("ToneCruiseLiftDetectionType",
             VegaSchemes[schemetype].ToneLiftComparisonType);

  SetControl("ToneCruiseFasterBeepType",
             VegaSchemes[schemetype].ToneCruiseFasterBeepType);
  SetControl("ToneCruiseFasterPitchScheme",
             VegaSchemes[schemetype].ToneCruiseFasterPitchScheme);
  SetControl("ToneCruiseFasterPitchScale",
             VegaSchemes[schemetype].ToneCruiseFasterPitchScale);
  SetControl("ToneCruiseFasterPeriodScheme",
             VegaSchemes[schemetype].ToneCruiseFasterPeriodScheme);
  SetControl("ToneCruiseFasterPeriodScale",
             VegaSchemes[schemetype].ToneCruiseFasterPeriodScale);

  SetControl("ToneCruiseSlowerBeepType",
             VegaSchemes[schemetype].ToneCruiseSlowerBeepType);
  SetControl("ToneCruiseSlowerPitchScheme",
             VegaSchemes[schemetype].ToneCruiseSlowerPitchScheme);
  SetControl("ToneCruiseSlowerPitchScale",
             VegaSchemes[schemetype].ToneCruiseSlowerPitchScale);
  SetControl("ToneCruiseSlowerPeriodScheme",
             VegaSchemes[schemetype].ToneCruiseSlowerPeriodScheme);
  SetControl("ToneCruiseSlowerPeriodScale",
             VegaSchemes[schemetype].ToneCruiseSlowerPeriodScale);

  SetControl("ToneCruiseLiftBeepType",
             VegaSchemes[schemetype].ToneCruiseLiftBeepType);
  SetControl("ToneCruiseLiftPitchScheme",
             VegaSchemes[schemetype].ToneCruiseLiftPitchScheme);
  SetControl("ToneCruiseLiftPitchScale",
             VegaSchemes[schemetype].ToneCruiseLiftPitchScale);
  SetControl("ToneCruiseLiftPeriodScheme",
             VegaSchemes[schemetype].ToneCruiseLiftPeriodScheme);
  SetControl("ToneCruiseLiftPeriodScale",
             VegaSchemes[schemetype].ToneCruiseLiftPeriodScale);

  SetControl("ToneCirclingClimbingHiBeepType",
             VegaSchemes[schemetype].ToneCirclingClimbingHiBeepType);
  SetControl("ToneCirclingClimbingHiPitchScheme",
             VegaSchemes[schemetype].ToneCirclingClimbingHiPitchScheme);
  SetControl("ToneCirclingClimbingHiPitchScale",
             VegaSchemes[schemetype].ToneCirclingClimbingHiPitchScale);
  SetControl("ToneCirclingClimbingHiPeriodScheme",
             VegaSchemes[schemetype].ToneCirclingClimbingHiPeriodScheme);
  SetControl("ToneCirclingClimbingHiPeriodScale",
             VegaSchemes[schemetype].ToneCirclingClimbingHiPeriodScale);

  SetControl("ToneCirclingClimbingLowBeepType",
             VegaSchemes[schemetype].ToneCirclingClimbingLowBeepType);
  SetControl("ToneCirclingClimbingLowPitchScheme",
             VegaSchemes[schemetype].ToneCirclingClimbingLowPitchScheme);
  SetControl("ToneCirclingClimbingLowPitchScale",
             VegaSchemes[schemetype].ToneCirclingClimbingLowPitchScale);
  SetControl("ToneCirclingClimbingLowPeriodScheme",
             VegaSchemes[schemetype].ToneCirclingClimbingLowPeriodScheme);
  SetControl("ToneCirclingClimbingLowPeriodScale",
             VegaSchemes[schemetype].ToneCirclingClimbingLowPeriodScale);

  SetControl("ToneCirclingDescendingBeepType",
             VegaSchemes[schemetype].ToneCirclingDescendingBeepType);
  SetControl("ToneCirclingDescendingPitchScheme",
             VegaSchemes[schemetype].ToneCirclingDescendingPitchScheme);
  SetControl("ToneCirclingDescendingPitchScale",
             VegaSchemes[schemetype].ToneCirclingDescendingPitchScale);
  SetControl("ToneCirclingDescendingPeriodScheme",
             VegaSchemes[schemetype].ToneCirclingDescendingPeriodScheme);
  SetControl("ToneCirclingDescendingPeriodScale",
             VegaSchemes[schemetype].ToneCirclingDescendingPeriodScale);

  MessageBoxX(_("Audio scheme updated."),
              _("Vega Audio"), MB_OK);
}

static void
UpdateParameters()
{
  UpdateControls(vega_setting_names, vega_setting_values);
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
  UpdateParameters();
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
  UpdateParameters();
  // make sure changes are sent to device
  wf->SetModalResult(mrOK);
}

static void
OnSaveClicked(gcc_unused WndButton &Sender)
{
  UpdateParameters();
  // make sure changes are sent to device
  if (dirty && device->SendSetting("StoreToEeprom", 2))
    dirty = false;
}

static void
OnDemoClicked(gcc_unused WndButton &Sender)
{
  // retrieve changes from form
  UpdateParameters();
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

  wp = (WndProperty*)wf->FindByName(_T("prpBaudrateA"));
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
    dfe->AddChoice(255, _T("AUTO"));
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
  changed = dirty = false;

  if (devVarioFindVega() == NULL) {
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

  InitControls(vega_setting_names, vega_setting_values);

  wf->ShowModal();

  UpdateParameters();

  delete wf;
  wf = NULL;

  return changed;
}
