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
Profile::Use()
{
  unsigned Speed = 0;
  unsigned Distance = 0;
  unsigned TaskSpeed = 0;
  unsigned Lift = 0;
  unsigned Altitude = 0;
  unsigned Temp = 0;
  int i;

  LogStartUp(TEXT("Read registry settings"));

  if (is_altair())
    DefaultRegistrySettingsAltair();

#ifdef OLD_TASK
  SETTINGS_TASK settings_task = task.getSettings();
  Profile::Get(szProfileFinishMinHeight,
		  settings_task.FinishMinHeight);
  Profile::Get(szProfileStartHeightRef,
		  settings_task.StartHeightRef);
  Profile::Get(szProfileStartMaxHeight,
		  settings_task.StartMaxHeight);
  Profile::Get(szProfileStartMaxHeightMargin,
		  settings_task.StartMaxHeightMargin);
  Profile::Get(szProfileStartMaxSpeed,
		  settings_task.StartMaxSpeed);
  Profile::Get(szProfileStartMaxSpeedMargin,
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

  Profile::Get(szProfileSectorRadius,
      settings_task.SectorRadius);

  Profile::Get(szProfileStartRadius,
      settings_task.StartRadius);
  Profile::Get(szProfileFinishRadius,
      settings_task.FinishRadius);

  Temp = settings_task.AutoAdvance;
  GetFromRegistryD(szProfileAutoAdvance, Temp);
  settings_task.AutoAdvance = (AutoAdvanceMode_t)Temp;

  Profile::Get(szProfileFAIFinishHeight,
		  settings_task.EnableFAIFinishHeight);
  task.setSettings(settings_task);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    Profile::Get(szProfileAirspacePriority[i], AirspacePriority[i]);
  }
#endif

  Temp = 0;
  Profile::Get(szProfileLatLonUnits, Temp);
  Units::SetCoordinateFormat((CoordinateFormats_t)Temp);

  Profile::Get(szProfileSpeedUnitsValue, Speed);
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

  Profile::Get(szProfileTaskSpeedUnitsValue, TaskSpeed);
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

  Profile::Get(szProfileDistanceUnitsValue,Distance);
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

  Profile::Get(szProfileAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    Units::SetUserAltitudeUnit(unFeet);
    break;
  case 1:
  default:
    Units::SetUserAltitudeUnit(unMeter);
    break;
  }

  Profile::Get(szProfileLiftUnitsValue, Lift);
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
    Profile::Get(szProfileDisplayType[i], Temp);
    InfoBoxManager::setTypeAll(i, Temp);
  }

  // check against V3 infotypes
  CheckInfoTypes();

  Temp = SetSettingsMap().DisplayOrientation;
  Profile::Get(szProfileDisplayUpValue, Temp);
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
  Profile::Get(szProfileDisplayText, Temp);
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
  Profile::Get(szProfileAltMode, Temp);
  SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)Temp;

  Profile::Get(szProfileClipAlt,
      SetSettingsComputer().ClipAltitude);
  Profile::Get(szProfileAltMargin,
      SetSettingsComputer().AltWarningMargin);

  Profile::Get(szProfileSafetyAltitudeArrival,
		  SetSettingsComputer().SafetyAltitudeArrival);
  Profile::Get(szProfileSafetyAltitudeBreakOff,
		  SetSettingsComputer().SafetyAltitudeBreakoff);
  Profile::Get(szProfileSafetyAltitudeTerrain,
		  SetSettingsComputer().SafetyAltitudeTerrain);
  Profile::Get(szProfileSafteySpeed,
		  SetSettingsComputer().SafetySpeed);
  Profile::Get(szProfilePolarID, 
                  SetSettingsComputer().POLARID);

  GetRegistryString(szProfileRegKey, strRegKey, 65);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    SetSettingsComputer().iAirspaceMode[i] = GetRegistryAirspaceMode(i);

    Profile::Get(szProfileBrush[i], SetSettingsMap().iAirspaceBrush[i]);
    Profile::Get(szProfileColour[i], SetSettingsMap().iAirspaceColour[i]);
    if (SettingsMap().iAirspaceColour[i] >= NUMAIRSPACECOLORS) {
      SetSettingsMap().iAirspaceColour[i] = 0;
    }
    if (SettingsMap().iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES) {
      SetSettingsMap().iAirspaceBrush[i] = 0;
    }
  }

  Profile::Get(szProfileAirspaceBlackOutline,
		  SetSettingsMap().bAirspaceBlackOutline);
  Profile::Get(szProfileSnailTrail,
		  SetSettingsMap().TrailActive);

  Profile::Get(szProfileTrailDrift,
		  SetSettingsMap().EnableTrailDrift);

  Profile::Get(szProfileThermalLocator,
		  SetSettingsComputer().EnableThermalLocator);

  Profile::Get(szProfileAnimation, EnableAnimation);

  Profile::Get(szProfileDrawTopology,
		  SetSettingsMap().EnableTopology);

  Profile::Get(szProfileDrawTerrain,
		  SetSettingsMap().EnableTerrain);

  Profile::Get(szProfileFinalGlideTerrain,
		  SetSettingsComputer().FinalGlideTerrain);

  Profile::Get(szProfileAutoWind,
		  SetSettingsComputer().AutoWindMode);

  Profile::Get(szProfileCircleZoom,
		  SetSettingsMap().CircleZoom);

  Profile::Get(szProfileHomeWaypoint,
      SetSettingsComputer().HomeWaypoint);

  Temp = SettingsComputer().Alternate1;
  if (Profile::Get(szProfileAlternate1, Temp) == ERROR_SUCCESS) {
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    SetSettingsComputer().Alternate1 = Temp;
    SetSettingsComputer().EnableAlternate1 = true;
  } else {
    SetSettingsComputer().Alternate1 = -1;
    SetSettingsComputer().EnableAlternate1 = false;
  }

  Temp = SettingsComputer().Alternate2;
  if (Profile::Get(szProfileAlternate2, Temp) == ERROR_SUCCESS) {
    SetSettingsComputer().Alternate2 = Temp;
    SetSettingsComputer().EnableAlternate2 = true;
  } else {
    SetSettingsComputer().Alternate2 = -1;
    SetSettingsComputer().EnableAlternate2 = false;
  }

  Profile::Get(szProfileSnailWidthScale,
		  SetSettingsMap().SnailWidthScale);

  Profile::Get(szProfileTeamcodeRefWaypoint,
		  SetSettingsComputer().TeamCodeRefWaypoint);

  Profile::Get(szProfileAirspaceWarning,
		  SetSettingsComputer().EnableAirspaceWarnings);

  Profile::Get(szProfileWarningTime,
		  SetSettingsComputer().WarningTime);

  Profile::Get(szProfileAcknowledgementTime,
		  SetSettingsComputer().AcknowledgementTime);

  Profile::Get(szProfileSoundVolume,
		  SetSettingsComputer().SoundVolume);

  Profile::Get(szProfileSoundDeadband,
		  SetSettingsComputer().SoundDeadband);

  Profile::Get(szProfileSoundAudioVario,
		  SetSettingsComputer().EnableSoundVario);

  Profile::Get(szProfileSoundTask,
		  SetSettingsComputer().EnableSoundTask);

  Profile::Get(szProfileSoundModes,
		  SetSettingsComputer().EnableSoundModes);

  SetSettingsMap().EnableCDICruise = 0;
  SetSettingsMap().EnableCDICircling = 0;

#ifdef HAVE_BLANK
  Profile::Get(szProfileAutoBlank,
		  SetSettingsMap().EnableAutoBlank);
#endif

  Profile::Get(szProfileAutoBacklight,
		  EnableAutoBacklight);
  Profile::Get(szProfileAutoSoundVolume,
		  EnableAutoSoundVolume);
  Profile::Get(szProfileExtendedVisualGlide,
		  SetSettingsMap().ExtendedVisualGlide);

#ifdef PNA
  Temp = 1;
#else
  Temp = 0;
#endif
  Profile::Get(szProfileVirtualKeys,Temp);
  VirtualKeys = Temp;

  Temp = (AverEffTime_t)ae2minutes;
  Profile::Get(szProfileAverEffTime,Temp);
  SetSettingsComputer().AverEffTime = Temp;

#if defined(GNAV) || defined(PCGNAV)
  Temp = 0;
#else
  Temp = 250;
#endif
  Profile::Get(szProfileDebounceTimeout, Temp);
  debounceTimeout = Temp;

  /* JMW broken
  Temp = 100;
  Profile::Get(szProfileAccelerometerZero, Temp);
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
  Profile::Get(szProfileAppIndFinalGlide, Temp);
  Appearance.IndFinalGlide = (IndFinalGlide_t)Temp;

  Temp = Appearance.IndLandable;
  Profile::Get(szProfileAppIndLandable, Temp);
  Appearance.IndLandable = (IndLandable_t)Temp;

  Profile::Get(szProfileAppInverseInfoBox,
		  Appearance.InverseInfoBox);
  Profile::Get(szProfileAppGaugeVarioSpeedToFly,
		  Appearance.GaugeVarioSpeedToFly);
  Profile::Get(szProfileAppGaugeVarioAvgText,
		  Appearance.GaugeVarioAvgText);
  Profile::Get(szProfileAppGaugeVarioMc,
		  Appearance.GaugeVarioMc);
  Profile::Get(szProfileAppGaugeVarioBugs,
		  Appearance.GaugeVarioBugs);
  Profile::Get(szProfileAppGaugeVarioBallast,
		  Appearance.GaugeVarioBallast);
  Profile::Get(szProfileAppGaugeVarioGross,
		  Appearance.GaugeVarioGross);

  Temp = Appearance.CompassAppearance;
  Profile::Get(szProfileAppCompassAppearance, Temp);
  Appearance.CompassAppearance = (CompassAppearance_t)Temp;

  Temp = (InfoBoxBorderAppearance_t)apIbBox;
  Profile::Get(szProfileAppInfoBoxBorder, Temp);
  Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)Temp;

  // VENTA2-ADDON Geometry change and PNA custom font settings
  // depending on infobox geometry and model type
  // I had to move here the font setting because I needed first to
  // know the screen geometry, in the registry!
#if defined(PNA) || defined(FIVV)
  Temp = Appearance.InfoBoxGeom;
  Profile::Get(szProfileAppInfoBoxGeom, Temp);
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
  Profile::Get(szProfileAppInfoBoxModel, Temp);
  Appearance.InfoBoxModel = (InfoBoxModelAppearance_t)Temp;
#endif

  Temp = Appearance.StateMessageAlign;
  Profile::Get(szProfileAppStatusMessageAlignment, Temp);
  Appearance.StateMessageAlign = (StateMessageAlign_t)Temp;

  Temp = Appearance.TextInputStyle;
  Profile::Get(szProfileAppTextInputStyle, Temp);
  Appearance.TextInputStyle = (TextInputStyle_t)Temp;

  Temp = g_eDialogStyle;
  Profile::Get(szProfileAppDialogStyle, Temp);
  g_eDialogStyle = (DialogStyle_t)Temp;

  Profile::Get(szProfileAppDefaultMapWidth,
		  Appearance.DefaultMapWidth);
  Profile::Get(szProfileAppInfoBoxColors,
		  Appearance.InfoBoxColors);
  Profile::Get(szProfileAppAveNeedle,
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
  Profile::Get(szProfileAutoMcMode,
		  SetSettingsComputer().auto_mc_mode);
#endif
  Profile::Get(szProfileWaypointsOutOfRange,
                  WayPointFile::WaypointsOutOfRangeSetting);
  {
    unsigned t = SettingsComputer().olc_rules;
    Profile::Get(szProfileOLCRules, t);
    SetSettingsComputer().olc_rules = (OLCRules)t;
  }
  Profile::Get(szProfileHandicap,
		  SetSettingsComputer().olc_handicap);
  Profile::Get(szProfileEnableExternalTriggerCruise,
		  SetSettingsComputer().EnableExternalTriggerCruise);

  Profile::Get(szProfileUTCOffset,
		  SetSettingsComputer().UTCOffset);
  if (SettingsComputer().UTCOffset > 12 * 3600)
    SetSettingsComputer().UTCOffset -= 24 * 3600;

  Profile::Get(szProfileBlockSTF,
		  SetSettingsComputer().EnableBlockSTF);
  Profile::Get(szProfileAutoZoom,
		  SetSettingsMap().AutoZoom);
  Profile::Get(szProfileMenuTimeout,
		  MenuTimeoutMax);
  Profile::Get(szProfileLockSettingsInFlight,
		  LockSettingsInFlight);
  Profile::Get(szProfileLoggerShort,
		  SetSettingsComputer().LoggerShortName);
  Profile::Get(szProfileEnableFLARMMap,
		  SetSettingsMap().EnableFLARMMap);
  Profile::Get(szProfileEnableFLARMGauge,
		  SetSettingsMap().EnableFLARMGauge);
  Profile::Get(szProfileTerrainContrast,
		  SetSettingsMap().TerrainContrast);
  Profile::Get(szProfileTerrainBrightness,
		  SetSettingsMap().TerrainBrightness);
  Profile::Get(szProfileTerrainRamp,
		  SetSettingsMap().TerrainRamp);

  Profile::Get(szProfileGliderScreenPosition,
		  SetSettingsMap().GliderScreenPosition);
  Profile::Get(szProfileBallastSecsToEmpty,
		  SetSettingsComputer().BallastSecsToEmpty);
  Profile::Get(szProfileSetSystemTimeFromGPS,
		  SetSettingsMap().SetSystemTimeFromGPS);
  Profile::Get(szProfileUseCustomFonts,
		  UseCustomFonts);
  Profile::Get(szProfileVoiceClimbRate,
		  SetSettingsComputer().EnableVoiceClimbRate);
  Profile::Get(szProfileVoiceTerrain,
		  SetSettingsComputer().EnableVoiceTerrain);
  Profile::Get(szProfileVoiceWaypointDistance,
		  SetSettingsComputer().EnableVoiceWaypointDistance);
  Profile::Get(szProfileVoiceTaskAltitudeDifference,
		  SetSettingsComputer().EnableVoiceTaskAltitudeDifference);
  Profile::Get(szProfileVoiceMacCready,
		  SetSettingsComputer().EnableVoiceMacCready);
  Profile::Get(szProfileVoiceNewWaypoint,
		  SetSettingsComputer().EnableVoiceNewWaypoint);
  Profile::Get(szProfileVoiceInSector,
		  SetSettingsComputer().EnableVoiceInSector);
  Profile::Get(szProfileVoiceAirspace,
		  SetSettingsComputer().EnableVoiceAirspace);
  Profile::Get(szProfileEnableNavBaroAltitude,
		  SetSettingsComputer().EnableNavBaroAltitude);
  Profile::Get(szProfileLoggerTimeStepCruise,
		  SetSettingsComputer().LoggerTimeStepCruise);
  Profile::Get(szProfileLoggerTimeStepCircling,
		  SetSettingsComputer().LoggerTimeStepCircling);
  Profile::Get(szProfileAbortSafetyUseCurrent,
		  SetSettingsComputer().safety_mc_use_current);

  Temp = iround(SettingsComputer().safety_mc * 10);
  Profile::Get(szProfileSafetyMacCready, Temp);
  SetSettingsComputer().safety_mc = Temp / 10.0;

  Profile::Get(szProfileUserLevel, UserLevel);

  Temp = iround(SettingsComputer().risk_gamma * 10);
  Profile::Get(szProfileRiskGamma, Temp);
  SetSettingsComputer().risk_gamma = Temp / 10.0;

  Temp = (CompassAppearance_t)apCompassAltA;
  Profile::Get(szProfileWindArrowStyle, Temp);
  SetSettingsMap().WindArrowStyle = Temp;

  Profile::Get(szProfileDisableAutoLogger,
		  SetSettingsComputer().DisableAutoLogger);
}

void
Profile::SetRegistryAirspaceMode(int i)
{
  int val = SettingsComputer().iAirspaceMode[i];
  SetToRegistry(szProfileAirspaceMode[i], val);
}

int
Profile::GetRegistryAirspaceMode(int i)
{
  int Temp = 3; // display + warnings
  Profile::Get(szProfileAirspaceMode[i], Temp);
  return Temp;
}

void
Profile::SaveSoundSettings()
{
  SetToRegistry(szProfileSoundVolume, SettingsComputer().SoundVolume);
  SetToRegistry(szProfileSoundDeadband, SettingsComputer().SoundDeadband);
  SetToRegistry(szProfileSoundAudioVario, SettingsComputer().EnableSoundVario);
  SetToRegistry(szProfileSoundTask, SettingsComputer().EnableSoundTask);
  SetToRegistry(szProfileSoundModes, SettingsComputer().EnableSoundModes);
}

void
Profile::SaveWindToRegistry()
{
  int Temp;
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
  Profile::Get(szProfileWindSpeed,&Temp);
  Calculated().WindSpeed = Temp;
  Temp=0;
  Profile::Get(szProfileWindBearing,&Temp);
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

bool
Profile::Get(const TCHAR *szRegValue, int &pPos)
{
  return GetFromRegistry(szRegValue, pPos);
}

bool
Profile::Get(const TCHAR *szRegValue, short &pPos)
{
  return GetFromRegistry(szRegValue, pPos);
}

bool
Profile::Get(const TCHAR *szRegValue, bool &pPos)
{
  return GetFromRegistry(szRegValue, pPos);
}

bool
Profile::Get(const TCHAR *szRegValue, unsigned &pPos)
{
  return GetFromRegistry(szRegValue, pPos);
}

bool
Profile::Get(const TCHAR *szRegValue, double &pPos)
{
  return GetFromRegistry(szRegValue, pPos);
}

bool
Profile::Set(const TCHAR *szRegValue, int pPos)
{
  return SetToRegistry(szRegValue, pPos);
}

bool
Profile::Set(const TCHAR *szRegValue, short pPos)
{
  return SetToRegistry(szRegValue, pPos);
}

bool
Profile::Set(const TCHAR *szRegValue, bool pPos)
{
  return SetToRegistry(szRegValue, pPos);
}

bool
Profile::Set(const TCHAR *szRegValue, unsigned pPos)
{
  return SetToRegistry(szRegValue, pPos);
}

bool
Profile::Set(const TCHAR *szRegValue, double pPos)
{
  return SetToRegistry(szRegValue, (DWORD)pPos);
}
