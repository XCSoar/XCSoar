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

#include "Profile.hpp"
#include "Registry.hpp"
#include "LogFile.hpp"
#include "SettingsTask.hpp"
#include "Appearance.hpp"
#include "InfoBoxManager.hpp"
#include "GlideRatio.hpp"
#include "Screen/Fonts.hpp"
#include "Asset.hpp"
#include "Dialogs/XML.hpp"
#include "WayPointFile.hpp"
#include "LocalPath.hpp"
#include "StringUtil.hpp"
#include "Units.hpp"

#define CheckIndex(x, i) do {} while (false)

extern int WaypointsOutOfRange;

static void
DefaultRegistrySettingsAltair()
{
  // these are redundant b/c they're also added to "InitialiseFontsHardCoded"
  SetRegistryStringIfAbsent(TEXT("InfoWindowFont"),
      TEXT("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  SetRegistryStringIfAbsent(TEXT("TitleWindowFont"),
      TEXT("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"));
  SetRegistryStringIfAbsent(TEXT("CDIWindowFont"),
      TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"));
  SetRegistryStringIfAbsent(TEXT("MapLabelFont"),
      TEXT("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"));
  SetRegistryStringIfAbsent(TEXT("StatisticsFont"),
      TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  SetRegistryStringIfAbsent(TEXT("MapWindowFont"),
      TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  SetRegistryStringIfAbsent(TEXT("MapWindowBoldFont"),
      TEXT("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  SetRegistryStringIfAbsent(TEXT("BugsBallastFont"),
      TEXT("24,0,0,0,750,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  SetRegistryStringIfAbsent(TEXT("AirspacePressFont"),
      TEXT("24,0,0,0,750,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  SetRegistryStringIfAbsent(TEXT("AirspaceColourDlgFont"),
      TEXT("14,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"));
  SetRegistryStringIfAbsent(TEXT("TeamCodeFont"),
      TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"));
#if 0
  SetRegistryStringIfAbsent(TEXT("ScaleList"),
      TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));
#endif
}

// This function checks to see if Final Glide mode infoboxes have been
// initialised.  If all are zero, then the current configuration was
// using XCSoarV3 infoboxes, so copy settings from cruise mode.
static void
CheckInfoTypes()
{
  int i;
  bool iszero_fg = true;
  bool iszero_aux = true;

  for (i = 0; i < MAXINFOWINDOWS; ++i) {
    iszero_fg &= (InfoBoxManager::getType(i, 2) == 0);
    iszero_aux &= (InfoBoxManager::getType(i, 3) == 0);
  }

  if (iszero_fg || iszero_aux) {
    for (i = 0; i < MAXINFOWINDOWS; ++i) {
      if (iszero_fg)
        InfoBoxManager::setType(i, InfoBoxManager::getType(i, 1), 2);
      if (iszero_aux)
        InfoBoxManager::setType(i, InfoBoxManager::getType(i, 1), 3);

      StoreType(i, InfoBoxManager::getTypeAll(i));
    }
  }
}

void
Profile::ReadRegistrySettings()
{
  unsigned Speed = 0;
  unsigned Distance = 0;
  unsigned TaskSpeed = 0;
  unsigned Lift = 0;
  unsigned Altitude = 0;
  DWORD Temp = 0;
  int i;

  LogStartUp(TEXT("Read registry settings"));

  if (is_altair())
    DefaultRegistrySettingsAltair();

#ifdef OLD_TASK
  SETTINGS_TASK settings_task = task.getSettings();
  GetFromRegistry(szProfileFinishMinHeight,
		  settings_task.FinishMinHeight);
  GetFromRegistry(szProfileStartHeightRef,
		  settings_task.StartHeightRef);
  GetFromRegistry(szProfileStartMaxHeight,
		  settings_task.StartMaxHeight);
  GetFromRegistry(szProfileStartMaxHeightMargin,
		  settings_task.StartMaxHeightMargin);
  GetFromRegistry(szProfileStartMaxSpeed,
		  settings_task.StartMaxSpeed);
  GetFromRegistry(szProfileStartMaxSpeedMargin,
		  settings_task.StartMaxSpeedMargin);

  Temp = settings_task.SectorType;
  GetFromRegistryD(szProfileFAISector, Temp);
  settings_task.SectorType = (ASTSectorType_t)Temp;

  Temp = settings_task.StartType;
  GetFromRegistryD(szProfileStartLine, Temp);
  settings_task.StartType = (StartSectorType_t)Temp;

  Temp = settings_task.FinishType;
  GetFromRegistryD(szProfileFinishLine, Temp);
  settings_task.FinishType = (FinishSectorType_t)Temp;

  GetFromRegistry(szProfileSectorRadius,
      settings_task.SectorRadius);

  GetFromRegistry(szProfileStartRadius,
      settings_task.StartRadius);
  GetFromRegistry(szProfileFinishRadius,
      settings_task.FinishRadius);

  Temp = settings_task.AutoAdvance;
  GetFromRegistryD(szProfileAutoAdvance, Temp);
  settings_task.AutoAdvance = (AutoAdvanceMode_t)Temp;

  GetFromRegistry(szProfileFAIFinishHeight,
		  settings_task.EnableFAIFinishHeight);
  task.setSettings(settings_task);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    GetFromRegistry(szProfileAirspacePriority[i], AirspacePriority[i]);
  }
#endif

  Temp = 0;
  GetFromRegistryD(szProfileLatLonUnits, Temp);
  Units::SetCoordinateFormat((CoordinateFormats_t)Temp);

  GetFromRegistry(szProfileSpeedUnitsValue, Speed);
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

  GetFromRegistry(szProfileTaskSpeedUnitsValue, TaskSpeed);
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

  GetFromRegistry(szProfileDistanceUnitsValue,Distance);
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

  GetFromRegistry(szProfileAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    Units::SetUserAltitudeUnit(unFeet);
    break;
  case 1:
  default:
    Units::SetUserAltitudeUnit(unMeter);
    break;
  }

  GetFromRegistry(szProfileLiftUnitsValue, Lift);
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
    Temp = InfoBoxManager::getTypeAll(i);
    GetFromRegistryD(szProfileDisplayType[i], Temp);
    InfoBoxManager::setTypeAll(i, Temp);
  }

  // check against V3 infotypes
  CheckInfoTypes();

  Temp = SetSettingsMap().DisplayOrientation;
  GetFromRegistryD(szProfileDisplayUpValue, Temp);
  switch (Temp) {
  case TRACKUP:
    SetSettingsMap().DisplayOrientation = TRACKUP;
    break;
  case NORTHUP:
    SetSettingsMap().DisplayOrientation = NORTHUP;
    break;
  case NORTHCIRCLE:
    SetSettingsMap().DisplayOrientation = NORTHCIRCLE;
    break;
  case TRACKCIRCLE:
    SetSettingsMap().DisplayOrientation = TRACKCIRCLE;
    break;
  case NORTHTRACK:
    SetSettingsMap().DisplayOrientation = NORTHTRACK;
    break;
  }

  Temp = SetSettingsMap().DisplayTextType;
  GetFromRegistryD(szProfileDisplayText, Temp);
  switch (Temp) {
  case 0:
    SetSettingsMap().DisplayTextType = DISPLAYNAME;
    break;
  case 1:
    SetSettingsMap().DisplayTextType = DISPLAYNUMBER;
    break;
  case 2:
    SetSettingsMap().DisplayTextType = DISPLAYFIRSTFIVE;
    break;
  case 3:
    SetSettingsMap().DisplayTextType = DISPLAYNONE;
    break;
  case 4:
    SetSettingsMap().DisplayTextType = DISPLAYFIRSTTHREE;
    break;
  case 5:
    SetSettingsMap().DisplayTextType = DISPLAYNAMEIFINTASK;
    break;
  }

  Temp = SetSettingsComputer().AltitudeMode;
  GetFromRegistryD(szProfileAltMode, Temp);
  SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)Temp;

  GetFromRegistry(szProfileClipAlt,
      SetSettingsComputer().ClipAltitude);
  GetFromRegistry(szProfileAltMargin,
      SetSettingsComputer().AltWarningMargin);

  GetFromRegistry(szProfileSafetyAltitudeArrival,
		  SetSettingsComputer().SafetyAltitudeArrival);
  GetFromRegistry(szProfileSafetyAltitudeBreakOff,
		  SetSettingsComputer().SafetyAltitudeBreakoff);
  GetFromRegistry(szProfileSafetyAltitudeTerrain,
		  SetSettingsComputer().SafetyAltitudeTerrain);
  GetFromRegistry(szProfileSafteySpeed,
		  SetSettingsComputer().SafetySpeed);
  GetFromRegistry(szProfilePolarID, 
                  SetSettingsComputer().POLARID);

  GetRegistryString(szProfileRegKey, strRegKey, 65);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    SetSettingsComputer().iAirspaceMode[i] = GetRegistryAirspaceMode(i);

    GetFromRegistry(szProfileBrush[i], SetSettingsMap().iAirspaceBrush[i]);
    GetFromRegistry(szProfileColour[i], SetSettingsMap().iAirspaceColour[i]);
    if (SettingsMap().iAirspaceColour[i] >= NUMAIRSPACECOLORS) {
      SetSettingsMap().iAirspaceColour[i] = 0;
    }
    if (SettingsMap().iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES) {
      SetSettingsMap().iAirspaceBrush[i] = 0;
    }
  }

  GetFromRegistry(szProfileAirspaceBlackOutline,
		  SetSettingsMap().bAirspaceBlackOutline);
  GetFromRegistry(szProfileSnailTrail,
		  SetSettingsMap().TrailActive);

  GetFromRegistry(szProfileTrailDrift,
		  SetSettingsMap().EnableTrailDrift);

  GetFromRegistry(szProfileThermalLocator,
		  SetSettingsComputer().EnableThermalLocator);

  GetFromRegistry(szProfileAnimation, EnableAnimation);

  GetFromRegistry(szProfileDrawTopology,
		  SetSettingsMap().EnableTopology);

  GetFromRegistry(szProfileDrawTerrain,
		  SetSettingsMap().EnableTerrain);

  GetFromRegistry(szProfileFinalGlideTerrain,
		  SetSettingsComputer().FinalGlideTerrain);

  GetFromRegistry(szProfileAutoWind,
		  SetSettingsComputer().AutoWindMode);

  GetFromRegistry(szProfileCircleZoom,
		  SetSettingsMap().CircleZoom);

  GetFromRegistry(szProfileHomeWaypoint,
      SetSettingsComputer().HomeWaypoint);

  Temp = SettingsComputer().Alternate1;
  if (GetFromRegistryD(szProfileAlternate1, Temp) == ERROR_SUCCESS) {
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    SetSettingsComputer().Alternate1 = Temp;
    SetSettingsComputer().EnableAlternate1 = true;
  } else {
    SetSettingsComputer().Alternate1 = -1;
    SetSettingsComputer().EnableAlternate1 = false;
  }

  Temp = SettingsComputer().Alternate2;
  if (GetFromRegistryD(szProfileAlternate2, Temp) == ERROR_SUCCESS) {
    SetSettingsComputer().Alternate2 = Temp;
    SetSettingsComputer().EnableAlternate2 = true;
  } else {
    SetSettingsComputer().Alternate2 = -1;
    SetSettingsComputer().EnableAlternate2 = false;
  }

  GetFromRegistry(szProfileSnailWidthScale,
		  SetSettingsMap().SnailWidthScale);

  GetFromRegistry(szProfileTeamcodeRefWaypoint,
		  SetSettingsComputer().TeamCodeRefWaypoint);

  GetFromRegistry(szProfileAirspaceWarning,
		  SetSettingsComputer().EnableAirspaceWarnings);

  GetFromRegistry(szProfileWarningTime,
		  SetSettingsComputer().WarningTime);

  GetFromRegistry(szProfileAcknowledgementTime,
		  SetSettingsComputer().AcknowledgementTime);

  GetFromRegistry(szProfileSoundVolume,
		  SetSettingsComputer().SoundVolume);

  GetFromRegistry(szProfileSoundDeadband,
		  SetSettingsComputer().SoundDeadband);

  GetFromRegistry(szProfileSoundAudioVario,
		  SetSettingsComputer().EnableSoundVario);

  GetFromRegistry(szProfileSoundTask,
		  SetSettingsComputer().EnableSoundTask);

  GetFromRegistry(szProfileSoundModes,
		  SetSettingsComputer().EnableSoundModes);

  SetSettingsMap().EnableCDICruise = 0;
  SetSettingsMap().EnableCDICircling = 0;

#ifdef HAVE_BLANK
  GetFromRegistry(szProfileAutoBlank,
		  SetSettingsMap().EnableAutoBlank);
#endif

  GetFromRegistry(szProfileAutoBacklight,
		  EnableAutoBacklight);
  GetFromRegistry(szProfileAutoSoundVolume,
		  EnableAutoSoundVolume);
  GetFromRegistry(szProfileExtendedVisualGlide,
		  SetSettingsMap().ExtendedVisualGlide);

#ifdef PNA
  Temp = 1;
#else
  Temp = 0;
#endif
  GetFromRegistryD(szProfileVirtualKeys,Temp);
  VirtualKeys = Temp;

  Temp = (AverEffTime_t)ae2minutes;
  GetFromRegistryD(szProfileAverEffTime,Temp);
  SetSettingsComputer().AverEffTime = Temp;

#if defined(GNAV) || defined(PCGNAV)
  Temp = 0;
#else
  Temp = 250;
#endif
  GetFromRegistryD(szProfileDebounceTimeout, Temp);
  debounceTimeout = Temp;

  /* JMW broken
  Temp = 100;
  GetFromRegistry(szProfileAccelerometerZero, Temp);
  AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    SetToRegistry(szProfileAccelerometerZero, Temp);
  }
  */

  // new appearance variables

  //Temp = Appearance.IndFinalGlide;
  Temp = (IndFinalGlide_t)fgFinalGlideDefault;
  GetFromRegistryD(szProfileAppIndFinalGlide, Temp);
  Appearance.IndFinalGlide = (IndFinalGlide_t)Temp;

  Temp = Appearance.IndLandable;
  GetFromRegistryD(szProfileAppIndLandable, Temp);
  Appearance.IndLandable = (IndLandable_t)Temp;

  GetFromRegistry(szProfileAppInverseInfoBox,
		  Appearance.InverseInfoBox);
  GetFromRegistry(szProfileAppGaugeVarioSpeedToFly,
		  Appearance.GaugeVarioSpeedToFly);
  GetFromRegistry(szProfileAppGaugeVarioAvgText,
		  Appearance.GaugeVarioAvgText);
  GetFromRegistry(szProfileAppGaugeVarioMc,
		  Appearance.GaugeVarioMc);
  GetFromRegistry(szProfileAppGaugeVarioBugs,
		  Appearance.GaugeVarioBugs);
  GetFromRegistry(szProfileAppGaugeVarioBallast,
		  Appearance.GaugeVarioBallast);
  GetFromRegistry(szProfileAppGaugeVarioGross,
		  Appearance.GaugeVarioGross);

  Temp = Appearance.CompassAppearance;
  GetFromRegistryD(szProfileAppCompassAppearance, Temp);
  Appearance.CompassAppearance = (CompassAppearance_t)Temp;

  Temp = (InfoBoxBorderAppearance_t)apIbBox;
  GetFromRegistryD(szProfileAppInfoBoxBorder, Temp);
  Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)Temp;

  // VENTA2-ADDON Geometry change and PNA custom font settings
  // depending on infobox geometry and model type
  // I had to move here the font setting because I needed first to
  // know the screen geometry, in the registry!
#if defined(PNA) || defined(FIVV)
  Temp = Appearance.InfoBoxGeom;
  GetFromRegistryD(szProfileAppInfoBoxGeom, Temp);
  Appearance.InfoBoxGeom = (InfoBoxGeomAppearance_t)Temp;

  if (GlobalModelType == MODELTYPE_PNA_HP31X ) {
    // key transcoding for this one
    LogStartUp(TEXT("Loading HP31X settings"));
  } else if (GlobalModelType == MODELTYPE_PNA_PN6000 ) {
    LogStartUp(TEXT("Loading PN6000 settings"));
    // key transcoding for this one
  } else if (GlobalModelType == MODELTYPE_PNA_MIO ) {
    LogStartUp(TEXT("Loading MIO settings"));
    // currently no special settings from MIO but need to handle hw keys
  } else if (GlobalModelType == MODELTYPE_PNA_NOKIA_500 ) {
    LogStartUp(TEXT("Loading Nokia500 settings"));
    // key transcoding is made
  } else if (GlobalModelType == MODELTYPE_PNA_MEDION_P5 ) {
    LogStartUp(TEXT("Loading Medion settings"));
  } else if (GlobalModelType == MODELTYPE_PNA_PNA ) {
    LogStartUp(TEXT("Loading default PNA settings"));
  } else {
    LogStartUp(TEXT("No special regsets for this PDA"));
  }

  // VENTA-ADDON Model change
  Temp = Appearance.InfoBoxModel;
  GetFromRegistryD(szProfileAppInfoBoxModel, Temp);
  Appearance.InfoBoxModel = (InfoBoxModelAppearance_t)Temp;
#endif

  Temp = Appearance.StateMessageAlign;
  GetFromRegistryD(szProfileAppStatusMessageAlignment, Temp);
  Appearance.StateMessageAlign = (StateMessageAlign_t)Temp;

  Temp = Appearance.TextInputStyle;
  GetFromRegistryD(szProfileAppTextInputStyle, Temp);
  Appearance.TextInputStyle = (TextInputStyle_t)Temp;

  Temp = g_eDialogStyle;
  GetFromRegistryD(szProfileAppDialogStyle, Temp);
  g_eDialogStyle = (DialogStyle_t)Temp;

  GetFromRegistry(szProfileAppDefaultMapWidth,
		  Appearance.DefaultMapWidth);
  GetFromRegistry(szProfileAppInfoBoxColors,
		  Appearance.InfoBoxColors);
  GetFromRegistry(szProfileAppAveNeedle,
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

#ifdef OLD_TASK
  GetFromRegistry(szProfileAutoMcMode,
		  SetSettingsComputer().auto_mc_mode);
#endif
  GetFromRegistry(szProfileWaypointsOutOfRange,
                  WayPointFile::WaypointsOutOfRangeSetting);
  {
    unsigned t = SettingsComputer().olc_rules;
    GetFromRegistry(szProfileOLCRules, t);
    SetSettingsComputer().olc_rules = (OLCRules)t;
  }
  GetFromRegistry(szProfileHandicap,
		  SetSettingsComputer().olc_handicap);
  GetFromRegistry(szProfileEnableExternalTriggerCruise,
		  SetSettingsComputer().EnableExternalTriggerCruise);

  GetFromRegistry(szProfileUTCOffset,
		  SetSettingsComputer().UTCOffset);
  if (SettingsComputer().UTCOffset > 12 * 3600)
    SetSettingsComputer().UTCOffset -= 24 * 3600;

  GetFromRegistry(szProfileBlockSTF,
		  SetSettingsComputer().EnableBlockSTF);
  GetFromRegistry(szProfileAutoZoom,
		  SetSettingsMap().AutoZoom);
  GetFromRegistry(szProfileMenuTimeout,
		  MenuTimeoutMax);
  GetFromRegistry(szProfileLockSettingsInFlight,
		  LockSettingsInFlight);
  GetFromRegistry(szProfileLoggerShort,
		  SetSettingsComputer().LoggerShortName);
  GetFromRegistry(szProfileEnableFLARMMap,
		  SetSettingsMap().EnableFLARMMap);
  GetFromRegistry(szProfileEnableFLARMGauge,
		  SetSettingsMap().EnableFLARMGauge);
  GetFromRegistry(szProfileTerrainContrast,
		  SetSettingsMap().TerrainContrast);
  GetFromRegistry(szProfileTerrainBrightness,
		  SetSettingsMap().TerrainBrightness);
  GetFromRegistry(szProfileTerrainRamp,
		  SetSettingsMap().TerrainRamp);

  GetFromRegistry(szProfileGliderScreenPosition,
		  SetSettingsMap().GliderScreenPosition);
  GetFromRegistry(szProfileBallastSecsToEmpty,
		  SetSettingsComputer().BallastSecsToEmpty);
  GetFromRegistry(szProfileSetSystemTimeFromGPS,
		  SetSettingsMap().SetSystemTimeFromGPS);
  GetFromRegistry(szProfileUseCustomFonts,
		  UseCustomFonts);
  GetFromRegistry(szProfileVoiceClimbRate,
		  SetSettingsComputer().EnableVoiceClimbRate);
  GetFromRegistry(szProfileVoiceTerrain,
		  SetSettingsComputer().EnableVoiceTerrain);
  GetFromRegistry(szProfileVoiceWaypointDistance,
		  SetSettingsComputer().EnableVoiceWaypointDistance);
  GetFromRegistry(szProfileVoiceTaskAltitudeDifference,
		  SetSettingsComputer().EnableVoiceTaskAltitudeDifference);
  GetFromRegistry(szProfileVoiceMacCready,
		  SetSettingsComputer().EnableVoiceMacCready);
  GetFromRegistry(szProfileVoiceNewWaypoint,
		  SetSettingsComputer().EnableVoiceNewWaypoint);
  GetFromRegistry(szProfileVoiceInSector,
		  SetSettingsComputer().EnableVoiceInSector);
  GetFromRegistry(szProfileVoiceAirspace,
		  SetSettingsComputer().EnableVoiceAirspace);
  GetFromRegistry(szProfileEnableNavBaroAltitude,
		  SetSettingsComputer().EnableNavBaroAltitude);
  GetFromRegistry(szProfileLoggerTimeStepCruise,
		  SetSettingsComputer().LoggerTimeStepCruise);
  GetFromRegistry(szProfileLoggerTimeStepCircling,
		  SetSettingsComputer().LoggerTimeStepCircling);
  GetFromRegistry(szProfileAbortSafetyUseCurrent,
		  SetSettingsComputer().safety_mc_use_current);

  Temp = iround(SettingsComputer().safety_mc * 10);
  GetFromRegistryD(szProfileSafetyMacCready, Temp);
  SetSettingsComputer().safety_mc = Temp / 10.0;

  GetFromRegistry(szProfileUserLevel, UserLevel);

  Temp = iround(SettingsComputer().risk_gamma * 10);
  GetFromRegistryD(szProfileRiskGamma, Temp);
  SetSettingsComputer().risk_gamma = Temp / 10.0;

  Temp = (CompassAppearance_t)apCompassAltA;
  GetFromRegistryD(szProfileWindArrowStyle, Temp);
  SetSettingsMap().WindArrowStyle = Temp;

  GetFromRegistry(szProfileDisableAutoLogger,
		  SetSettingsComputer().DisableAutoLogger);
}

void
Profile::SetRegistryAirspaceMode(int i)
{
  CheckIndex(SetSettingsComputer().iAirspaceMode, i);
  CheckIndex(szProfileAirspaceMode, i);

  DWORD val = SettingsComputer().iAirspaceMode[i];
  SetToRegistry(szProfileAirspaceMode[i], val);
}

int
Profile::GetRegistryAirspaceMode(int i)
{
  DWORD Temp = 3; // display + warnings
  CheckIndex(szProfileAirspaceMode, i);
  GetFromRegistryD(szProfileAirspaceMode[i], Temp);
  return Temp;
}

void
Profile::SaveSoundSettings()
{
  SetToRegistry(szProfileSoundVolume, (DWORD)SettingsComputer().SoundVolume);
  SetToRegistry(szProfileSoundDeadband, (DWORD)SettingsComputer().SoundDeadband);
  SetToRegistry(szProfileSoundAudioVario, SettingsComputer().EnableSoundVario);
  SetToRegistry(szProfileSoundTask, SettingsComputer().EnableSoundTask);
  SetToRegistry(szProfileSoundModes, SettingsComputer().EnableSoundModes);
}

void
Profile::SaveWindToRegistry()
{
  DWORD Temp;
  Temp = iround(Basic().wind.norm);
  SetToRegistry(szProfileWindSpeed, Temp);
  Temp = iround(Basic().wind.bearing);
  SetToRegistry(szProfileWindBearing, Temp);
  //TODO  SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing);
}

void
Profile::LoadWindFromRegistry()
{
  LogStartUp(TEXT("Load wind from registry"));

  /* JMW incomplete
  DWORD Temp;
  Temp=0;
  GetFromRegistry(szProfileWindSpeed,&Temp);
  Calculated().WindSpeed = Temp;
  Temp=0;
  GetFromRegistry(szProfileWindBearing,&Temp);
  Calculated().WindBearing = Temp;
  */
}

TCHAR startProfileFile[MAX_PATH];
TCHAR defaultProfileFile[MAX_PATH];
TCHAR failsafeProfileFile[MAX_PATH];

void
Profile::Load(void)
{
  LogStartUp(TEXT("Load profile"));
  // load registry backup if it exists
  LoadRegistryFromFile(failsafeProfileFile);
  LoadRegistryFromFile(startProfileFile);
}

void
Profile::Save(void)
{
  LogStartUp(TEXT("Save profile"));
  // save registry backup first (try a few places)
  SaveRegistryToFile(startProfileFile);
  SaveRegistryToFile(defaultProfileFile);
}

void
Profile::SetFiles(const TCHAR* override)
{
  // Set the default profile file
  if (is_altair())
    LocalPath(defaultProfileFile, TEXT("config/xcsoar-registry.prf"));
  else
    LocalPath(defaultProfileFile, TEXT(XCSPROFILE));

  // Set the failsafe profile file
  LocalPath(failsafeProfileFile, TEXT(XCSPROFILE));

  // Set the profile file to load at startup
  // -> to the default file
  _tcscpy(startProfileFile, defaultProfileFile);

  // -> to the given filename (if exists)
  if (!string_is_empty(override))
    _tcsncpy(startProfileFile, override, MAX_PATH - 1);
}
