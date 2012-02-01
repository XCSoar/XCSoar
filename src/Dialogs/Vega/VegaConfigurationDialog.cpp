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
#include "Schemes.hpp"
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

static const char *const audio_pages[] = {
  "CruiseFaster",
  "CruiseSlower",
  "CruiseLift",
  "CirclingClimbingHi",
  "CirclingClimbingLow",
  "CirclingDescending",
  NULL
};

static const TCHAR *const audio_pages_t[] = {
  _T("CruiseFaster"),
  _T("CruiseSlower"),
  _T("CruiseLift"),
  _T("CirclingClimbingHi"),
  _T("CirclingClimbingLow"),
  _T("CirclingDescending"),
  NULL
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
  "EnablePDASupply",

  "TotalEnergyMixingRatio",
  "CalibrationAirSpeed",
  "CalibrationTEStatic",
  "CalibrationTEDynamic",

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

  for (unsigned i = 0; audio_pages[i] != NULL; ++i) {
    const VEGA_SCHEME::Audio &data = VegaSchemes[schemetype].audio[i];
    const char *page = audio_pages[i];
    char name[64];

    sprintf(name, "Tone%sBeepType", page);
    SetControl(name, data.beep_type);
    sprintf(name, "Tone%sPitchScheme", page);
    SetControl(name, data.pitch_scheme);
    sprintf(name, "Tone%sPitchScale", page);
    SetControl(name, data.pitch_scale);
    sprintf(name, "Tone%sPeriodScheme", page);
    SetControl(name, data.period_scheme);
    sprintf(name, "Tone%sPeriodScale", page);
    SetControl(name, data.period_scheme);
  }

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
  for (auto i = audio_pages_t; *i != NULL; ++i)
    FillAudioEnums(*i);
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
