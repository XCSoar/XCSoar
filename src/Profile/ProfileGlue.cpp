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

#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Appearance.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "GlideRatio.hpp"
#include "Screen/Fonts.hpp"
#include "Dialogs/XML.hpp"
#include "WayPointFile.hpp"
#include "UtilsFont.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Interface.hpp"
#include "Sizes.h"

#include <assert.h>
#include <stdio.h>

// This function checks to see if Final Glide mode infoboxes have been
// initialised.  If all are zero, then the current configuration was
// using XCSoarV3 infoboxes, so copy settings from cruise mode.
static void
CheckInfoTypes()
{
  if (InfoBoxManager::IsEmpty(InfoBoxManager::MODE_CRUISE))
    return;

  bool iszero_fg = InfoBoxManager::IsEmpty(InfoBoxManager::MODE_FINAL_GLIDE);
  bool iszero_aux = InfoBoxManager::IsEmpty(InfoBoxManager::MODE_AUXILIARY);
  if (!iszero_fg && !iszero_aux)
    return;

  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i) {
    if (iszero_fg)
      InfoBoxManager::SetType(i, InfoBoxManager::GetType(i, InfoBoxManager::MODE_CRUISE),
                              InfoBoxManager::MODE_FINAL_GLIDE);
    if (iszero_aux)
      InfoBoxManager::SetType(i, InfoBoxManager::GetType(i, InfoBoxManager::MODE_CRUISE),
                              InfoBoxManager::MODE_AUXILIARY);
  }
}

void
Profile::Use()
{
  unsigned Speed = 0;
  unsigned Distance = 0;
  unsigned TaskSpeed = 0;
  unsigned Lift = 0;
  unsigned Altitude = 0;
  unsigned Temperature = 0;
  unsigned Temp = 0;
  int i;

  LogStartUp(_T("Read registry settings"));

  OrderedTaskBehaviour &osettings_task = 
    XCSoarInterface::SetSettingsComputer().ordered_defaults;

  TaskBehaviour &settings_task = 
    XCSoarInterface::SetSettingsComputer();

  Get(szProfileFinishMinHeight,
		  osettings_task.finish_min_height);
  Get(szProfileStartHeightRef,
		  osettings_task.start_max_height_ref);
  Get(szProfileStartMaxHeight,
		  osettings_task.start_max_height);
  Get(szProfileStartMaxSpeed,
		  osettings_task.start_max_speed);

  Get(szProfileStartMaxHeightMargin,
		  settings_task.start_max_height_margin);
  Get(szProfileStartMaxSpeedMargin,
		  settings_task.start_max_speed_margin);

#ifdef OLD_TASK // airspace priority
  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    Get(szProfileAirspacePriority[i], AirspacePriority[i]);
  }
#endif

  if (Get(szProfileLatLonUnits, Temp))
    Units::SetCoordinateFormat((CoordinateFormats_t)Temp);

  Get(szProfileSpeedUnitsValue, Speed);
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

  Get(szProfileTaskSpeedUnitsValue, TaskSpeed);
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

  Get(szProfileDistanceUnitsValue,Distance);
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

  Get(szProfileAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    Units::SetUserAltitudeUnit(unFeet);
    break;
  case 1:
  default:
    Units::SetUserAltitudeUnit(unMeter);
    break;
  }

  Get(szProfileTemperatureUnitsValue, Temperature);
  switch (Temperature) {
  default:
  case 0:
    Units::SetUserTemperatureUnit(unGradCelcius);
    break;
  case 1:
    Units::SetUserTemperatureUnit(unGradFahrenheit);
    break;
  }

  Get(szProfileLiftUnitsValue, Lift);
  switch (Lift) {
  case 0:
    Units::SetUserVerticalSpeedUnit(unKnots);
    break;
  case 1:
  default:
    Units::SetUserVerticalSpeedUnit(unMeterPerSecond);
    break;
  }

  for (i = 0; i < MAXINFOWINDOWS; i++) {
    if (Get(szProfileDisplayType[i], Temp))
      InfoBoxManager::SetTypes(i, Temp);
  }

  // check against V3 infotypes
  CheckInfoTypes();

  SETTINGS_MAP &settings_map = XCSoarInterface::SetSettingsMap();

  Temp = settings_map.DisplayOrientation;
  Get(szProfileDisplayUpValue, Temp);
  switch (Temp) {
  case TRACKUP:
    settings_map.DisplayOrientation = TRACKUP;
    break;
  case NORTHUP:
    settings_map.DisplayOrientation = NORTHUP;
    break;
  case NORTHCIRCLE:
    settings_map.DisplayOrientation = NORTHCIRCLE;
    break;
  case TRACKCIRCLE:
    settings_map.DisplayOrientation = TRACKCIRCLE;
    break;
  case NORTHTRACK:
    settings_map.DisplayOrientation = NORTHTRACK;
    break;
  }

  Temp = settings_map.DisplayTextType;
  Get(szProfileDisplayText, Temp);
  switch (Temp) {
  case 0:
    settings_map.DisplayTextType = DISPLAYNAME;
    break;
  case 1:
    settings_map.DisplayTextType = DISPLAYNUMBER;
    break;
  case 2:
    settings_map.DisplayTextType = DISPLAYFIRSTFIVE;
    break;
  case 3:
    settings_map.DisplayTextType = DISPLAYNONE;
    break;
  case 4:
    settings_map.DisplayTextType = DISPLAYFIRSTTHREE;
    break;
  case 5:
    settings_map.DisplayTextType = DISPLAYNAMEIFINTASK;
    break;
  case 6:
    settings_map.DisplayTextType = DISPLAYUNTILSPACE;
    break;
  }

  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();

  Temp = settings_computer.AltitudeMode;
  Get(szProfileAltMode, Temp);
  settings_computer.AltitudeMode = (AirspaceDisplayMode_t)Temp;

  Get(szProfileClipAlt,
      settings_computer.ClipAltitude);
  Get(szProfileAltMargin,
      settings_computer.AltWarningMargin);

  Get(szProfileSafetyAltitudeArrival,
      settings_computer.safety_height_arrival);
  Get(szProfileSafetyAltitudeTerrain,
      settings_computer.safety_height_terrain);
  Get(szProfileSafteySpeed,
      settings_computer.SafetySpeed);
  Get(szProfilePolarID,
      settings_computer.POLARID);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    if (Get(szProfileAirspaceMode[i], Temp)) {
      settings_computer.DisplayAirspaces[i] = (Temp & 0x1) != 0;
      settings_computer.airspace_warnings.class_warnings[i] = (Temp & 0x2) != 0;
    }

    Get(szProfileBrush[i],
        settings_map.iAirspaceBrush[i]);
    Get(szProfileColour[i],
        settings_map.iAirspaceColour[i]);
    if (settings_map.iAirspaceColour[i] >= NUMAIRSPACECOLORS)
      settings_map.iAirspaceColour[i] = 0;
    if (settings_map.iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES)
      settings_map.iAirspaceBrush[i] = 0;
  }

  Get(szProfileAirspaceBlackOutline,
      settings_map.bAirspaceBlackOutline);
  Get(szProfileSnailTrail,
      settings_map.TrailActive);

  Get(szProfileTrailDrift,
      settings_map.EnableTrailDrift);

  Get(szProfileDetourCostMarker,
      settings_map.EnableDetourCostMarker);

  Get(szProfileDrawTopology,
      settings_map.EnableTopology);

  Get(szProfileDrawTerrain,
      settings_map.EnableTerrain);

  Get(szProfileFinalGlideTerrain,
      settings_computer.FinalGlideTerrain);

  Get(szProfileAutoWind,
      settings_computer.AutoWindMode);

  Get(szProfileCircleZoom,
      settings_map.CircleZoom);

  Get(szProfileHomeWaypoint,
      settings_computer.HomeWaypoint);

  if (Get(szProfileAlternate1, Temp)) {
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    settings_computer.Alternate1 = Temp;
    settings_computer.EnableAlternate1 = true;
  } else {
    settings_computer.Alternate1 = -1;
    settings_computer.EnableAlternate1 = false;
  }

  if (Get(szProfileAlternate2, Temp)) {
    settings_computer.Alternate2 = Temp;
    settings_computer.EnableAlternate2 = true;
  } else {
    settings_computer.Alternate2 = -1;
    settings_computer.EnableAlternate2 = false;
  }

  Get(szProfileSnailWidthScale,
      settings_map.SnailWidthScale);

  Get(szProfileTeamcodeRefWaypoint,
      settings_computer.TeamCodeRefWaypoint);

  Get(szProfileAirspaceWarning,
      settings_computer.EnableAirspaceWarnings);

  Get(szProfileWarningTime,
      settings_computer.airspace_warnings.WarningTime);

  Get(szProfileAcknowledgementTime,
      settings_computer.airspace_warnings.AcknowledgementTime);

  Get(szProfileSoundVolume,
      settings_computer.SoundVolume);

  Get(szProfileSoundDeadband,
      settings_computer.SoundDeadband);

  Get(szProfileSoundAudioVario,
      settings_computer.EnableSoundVario);

  Get(szProfileSoundTask,
      settings_computer.EnableSoundTask);

  Get(szProfileSoundModes,
      settings_computer.EnableSoundModes);

  settings_map.EnableCDICruise = 0;
  settings_map.EnableCDICircling = 0;

#ifdef HAVE_BLANK
  Get(szProfileAutoBlank,
      settings_map.EnableAutoBlank);
#endif

  Get(szProfileAutoBacklight,
      XCSoarInterface::EnableAutoBacklight);

  Get(szProfileGestures,
      settings_computer.EnableGestures);

  if (Get(szProfileAverEffTime, Temp))
    settings_computer.AverEffTime = Temp;

  if (Get(szProfileDebounceTimeout, Temp))
    XCSoarInterface::debounceTimeout = Temp;

  /* JMW broken
  if (Get(szProfileAccelerometerZero, Temp))
    AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    Set(szProfileAccelerometerZero, Temp);
  }
  */

  // new appearance variables

  //Temp = Appearance.IndFinalGlide;
  if (Get(szProfileAppIndFinalGlide, Temp))
    Appearance.IndFinalGlide = (IndFinalGlide_t)Temp;

  if (Get(szProfileAppIndLandable, Temp))
    Appearance.IndLandable = (IndLandable_t)Temp;

  Get(szProfileAppInverseInfoBox,
		  Appearance.InverseInfoBox);
  Get(szProfileAppGaugeVarioSpeedToFly,
		  Appearance.GaugeVarioSpeedToFly);
  Get(szProfileAppGaugeVarioAvgText,
		  Appearance.GaugeVarioAvgText);
  Get(szProfileAppGaugeVarioMc,
		  Appearance.GaugeVarioMc);
  Get(szProfileAppGaugeVarioBugs,
		  Appearance.GaugeVarioBugs);
  Get(szProfileAppGaugeVarioBallast,
		  Appearance.GaugeVarioBallast);
  Get(szProfileAppGaugeVarioGross,
		  Appearance.GaugeVarioGross);

  if (Get(szProfileAppCompassAppearance, Temp))
    Appearance.CompassAppearance = (CompassAppearance_t)Temp;

  if (Get(szProfileAppInfoBoxBorder, Temp))
    Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)Temp;

  if (Get(szProfileAppStatusMessageAlignment, Temp))
    Appearance.StateMessageAlign = (StateMessageAlign_t)Temp;

  if (Get(szProfileAppTextInputStyle, Temp))
    Appearance.TextInputStyle = (TextInputStyle_t)Temp;

  if (Get(szProfileAppDialogStyle, Temp))
    DialogStyleSetting = (DialogStyle)Temp;

  Get(szProfileAppDefaultMapWidth,
		  Appearance.DefaultMapWidth);
  Get(szProfileAppInfoBoxColors,
		  Appearance.InfoBoxColors);
  Get(szProfileAppAveNeedle,
		  Appearance.GaugeVarioAveNeedle);

  // StateMessageAlign : center, topleft
  // DefaultMapWidth: 206?
  // CompassAppearance (north arrow)
  //
  // DontShowLoggerIndicator
  // FlightModeIcon
  // DontShowAutoMacCready
  // MapScale
  // MapScale2
  // BestCruiseTrack
  // Aircraft
  // IndFinalGlide
  // IndLandable


  if (Get(szProfileAutoMcMode, Temp))
    settings_computer.auto_mc_mode =
      (TaskBehaviour::AutoMCMode_t)Temp;

  if (Get(szProfileOLCRules, Temp))
      settings_computer.olc_rules = (OLCRules)Temp;

  Get(szProfileHandicap,
      settings_computer.olc_handicap);
  Get(szProfileEnableExternalTriggerCruise,
      settings_computer.EnableExternalTriggerCruise);

  Get(szProfileUTCOffset,
      settings_computer.UTCOffset);
  if (settings_computer.UTCOffset > 12 * 3600)
    settings_computer.UTCOffset -= 24 * 3600;

  Get(szProfileBlockSTF,
      settings_computer.EnableBlockSTF);
  Get(szProfileAutoZoom,
      settings_map.AutoZoom);
  Get(szProfileMenuTimeout,
      XCSoarInterface::MenuTimeoutMax);
  Get(szProfileLockSettingsInFlight,
      XCSoarInterface::LockSettingsInFlight);
  Get(szProfileLoggerShort,
      settings_computer.LoggerShortName);
  Get(szProfileEnableFLARMMap,
      settings_map.EnableFLARMMap);
  Get(szProfileEnableFLARMGauge,
      settings_map.EnableFLARMGauge);
  Get(szProfileEnableTAGauge,
      settings_map.EnableTAGauge);
  Get(szProfileTerrainContrast,
      settings_map.TerrainContrast);
  Get(szProfileTerrainBrightness,
      settings_map.TerrainBrightness);
  Get(szProfileTerrainRamp,
      settings_map.TerrainRamp);

  Get(szProfileGliderScreenPosition,
      settings_map.GliderScreenPosition);
  Get(szProfileBallastSecsToEmpty,
      settings_computer.BallastSecsToEmpty);
  Get(szProfileSetSystemTimeFromGPS,
      settings_map.SetSystemTimeFromGPS);
  Get(szProfileUseCustomFonts,
      Appearance.UseCustomFonts);
  Get(szProfileVoiceClimbRate,
      settings_computer.EnableVoiceClimbRate);
  Get(szProfileVoiceTerrain,
      settings_computer.EnableVoiceTerrain);
  Get(szProfileVoiceWaypointDistance,
      settings_computer.EnableVoiceWaypointDistance);
  Get(szProfileVoiceTaskAltitudeDifference,
      settings_computer.EnableVoiceTaskAltitudeDifference);
  Get(szProfileVoiceMacCready,
      settings_computer.EnableVoiceMacCready);
  Get(szProfileVoiceNewWaypoint,
      settings_computer.EnableVoiceNewWaypoint);
  Get(szProfileVoiceInSector,
      settings_computer.EnableVoiceInSector);
  Get(szProfileVoiceAirspace,
      settings_computer.EnableVoiceAirspace);
  Get(szProfileEnableNavBaroAltitude,
      settings_computer.EnableNavBaroAltitude);
  Get(szProfileLoggerTimeStepCruise,
      settings_computer.LoggerTimeStepCruise);
  Get(szProfileLoggerTimeStepCircling,
      settings_computer.LoggerTimeStepCircling);
  Get(szProfileAbortSafetyUseCurrent,
      settings_computer.safety_mc_use_current);

  if (Get(szProfileSafetyMacCready, Temp))
    settings_computer.safety_mc = fixed(Temp) / 10;

  Get(szProfileUserLevel, XCSoarInterface::UserLevel);

  if (Get(szProfileRiskGamma, Temp))
    settings_computer.risk_gamma = fixed(Temp) / 10;

  if (Get(szProfileWindArrowStyle, Temp))
    settings_map.WindArrowStyle = Temp;

  Get(szProfileDisableAutoLogger,
      settings_computer.DisableAutoLogger);
}

void
Profile::SetSoundSettings()
{
  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();

  Set(szProfileSoundVolume,
      settings_computer.SoundVolume);
  Set(szProfileSoundDeadband,
      settings_computer.SoundDeadband);
  Set(szProfileSoundAudioVario,
      settings_computer.EnableSoundVario);
  Set(szProfileSoundTask,
      settings_computer.EnableSoundTask);
  Set(szProfileSoundModes,
      settings_computer.EnableSoundModes);
}

int
Profile::GetScaleList(fixed *List, size_t Size)
{
  static const TCHAR Name[] = CONF("ScaleList");
  TCHAR Buffer[128];
  int Idx = 0;
  double vlast = 0;
  double val;

  assert(List != NULL);
  assert(Size > 0);

  Set(Name, _T("0.5,1,2,5,10,20,50,100,150,200,500,1000"));

  if (!Get(Name, Buffer, sizeof(Buffer) / sizeof(TCHAR)))
    return 0;

  const TCHAR *p = Buffer;
  while (Idx < (int)Size) {
    TCHAR *endptr;
    val = _tcstod(p, &endptr);
    if (Idx > 0) {
      List[Idx] = fixed(val + vlast) / 2;
      Idx++;
    }
    List[Idx] = fixed(val);
    Idx++;
    vlast = val;

    if (endptr == p)
      return 0;
    else if (*endptr == _T('\0'))
      break;
    else if (*endptr != _T(','))
      return 0;

    p = endptr + 1;
  }

  return Idx;
}

void
Profile::SetAirspaceMode(int i)
{
  const SETTINGS_AIRSPACE &settings_airspace =
    XCSoarInterface::SettingsComputer();

  int val = 0;
  if (settings_airspace.DisplayAirspaces[i])
    val |= 0x1;
  if (settings_airspace.airspace_warnings.class_warnings[i])
    val |= 0x2;

  Set(szProfileAirspaceMode[i], val);
}

void
Profile::SetAirspaceColor(int i, int c)
{
  Set(szProfileColour[i], c);
}

void
Profile::SetAirspaceBrush(int i, int c)
{
  Set(szProfileBrush[i], c);
}

void
Profile::SetInfoBoxes(int Index, int the_type)
{
  Set(szProfileDisplayType[Index], the_type);
}

static const TCHAR *
MakeDeviceSettingName(TCHAR *buffer, const TCHAR *prefix, unsigned n,
                      const TCHAR *suffix)
{
  _tcscpy(buffer, prefix);

  if (n > 0)
    _stprintf(buffer + _tcslen(buffer), _T("%u"), n + 1);

  _tcscat(buffer, suffix);

  return buffer;
}

static enum DeviceConfig::port_type
StringToPortType(const TCHAR *value)
{
  if (_tcscmp(value, _T("serial")) == 0)
    return DeviceConfig::SERIAL;

  if (_tcscmp(value, _T("auto")) == 0)
    return DeviceConfig::AUTO;

  return DeviceConfig::SERIAL;
}

static enum DeviceConfig::port_type
ReadPortType(unsigned n)
{
  TCHAR name[64], value[64];

  MakeDeviceSettingName(name, CONF("Port"), n, _T("Type"));
  if (!Profile::Get(name, value, sizeof(value) / sizeof(value[0])))
    return DeviceConfig::SERIAL;

  return StringToPortType(value);
}

void
Profile::GetDeviceConfig(unsigned n, DeviceConfig &config)
{
  TCHAR buffer[64];
  unsigned Temp = 0;

  config.port_type = ReadPortType(n);

  MakeDeviceSettingName(buffer, CONF("Port"), n, _T("Index"));
  if (Get(buffer, Temp))
    config.port_index = Temp;

  MakeDeviceSettingName(buffer, CONF("Speed"), n, _T("Index"));
  if (Get(buffer, Temp))
    config.speed_index = Temp;

  config.driver_name[0] = '\0';

  _tcscpy(buffer, CONF("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  Get(buffer, config.driver_name,
      sizeof(config.driver_name) / sizeof(config.driver_name[0]));
}

static const TCHAR *
PortTypeToString(enum DeviceConfig::port_type type)
{
  switch (type) {
  case DeviceConfig::SERIAL:
    return _T("serial");

  case DeviceConfig::AUTO:
    return _T("auto");
  }

  return NULL;
}

static bool
WritePortType(unsigned n, enum DeviceConfig::port_type type)
{
  const TCHAR *value = PortTypeToString(type);
  if (value == NULL)
    return false;

  TCHAR name[64];

  MakeDeviceSettingName(name, CONF("Port"), n, _T("Type"));
  return Profile::Set(name, value);
}

void
Profile::SetDeviceConfig(unsigned n, const DeviceConfig &config)
{
  TCHAR buffer[64];

  WritePortType(n, config.port_type);

  MakeDeviceSettingName(buffer, CONF("Port"), n, _T("Index"));
  Set(buffer, config.port_index);

  MakeDeviceSettingName(buffer, CONF("Speed"), n, _T("Index"));
  Set(buffer, config.speed_index);

  _tcscpy(buffer, CONF("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  Set(buffer, config.driver_name);
}

bool
Profile::GetFont(const TCHAR *key, LOGFONT* lplf)
{
  TCHAR Buffer[128];

  assert(key != NULL);
  assert(key[0] != '\0');
  assert(lplf != NULL);

  if (Get(key, Buffer, sizeof(Buffer) / sizeof(TCHAR)))
    return GetFontFromString(Buffer, lplf);

  return false;
}

void
Profile::SetFont(const TCHAR *key, LOGFONT &logfont)
{
  TCHAR Buffer[256];

  assert(key != NULL);
  assert(key[0] != '\0');

  _stprintf(Buffer, _T("%d,%d,0,0,%d,%d,0,0,0,0,0,%d,%d,%s"),
            logfont.lfHeight, logfont.lfWidth, logfont.lfWeight, logfont.lfItalic,
            logfont.lfQuality, logfont.lfPitchAndFamily, logfont.lfFaceName);
  Profile::Set(key, Buffer);
}
