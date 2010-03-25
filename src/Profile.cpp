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
#include "UtilsText.hpp"
#include "ProfileKeys.hpp"
#include "Units.hpp"

#include <assert.h>

extern int WaypointsOutOfRange;

static void
DefaultRegistrySettingsAltair()
{
  // these are redundant b/c they're also added to "InitialiseFontsHardCoded"
  Profile::SetStringIfAbsent(_T("InfoWindowFont"),
      _T("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  Profile::SetStringIfAbsent(_T("TitleWindowFont"),
      _T("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"));
  Profile::SetStringIfAbsent(_T("CDIWindowFont"),
      _T("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"));
  Profile::SetStringIfAbsent(_T("MapLabelFont"),
      _T("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"));
  Profile::SetStringIfAbsent(_T("StatisticsFont"),
      _T("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  Profile::SetStringIfAbsent(_T("MapWindowFont"),
      _T("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  Profile::SetStringIfAbsent(_T("MapWindowBoldFont"),
      _T("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  Profile::SetStringIfAbsent(_T("BugsBallastFont"),
      _T("24,0,0,0,750,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  Profile::SetStringIfAbsent(_T("AirspacePressFont"),
      _T("24,0,0,0,750,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  Profile::SetStringIfAbsent(_T("AirspaceColourDlgFont"),
      _T("14,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"));
  Profile::SetStringIfAbsent(_T("TeamCodeFont"),
      _T("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"));
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

  LogStartUp(_T("Read registry settings"));

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

  Temp = XCSoarInterface::SetSettingsMap().DisplayOrientation;
  Profile::Get(szProfileDisplayUpValue, Temp);
  switch (Temp) {
  case TRACKUP:
    XCSoarInterface::SetSettingsMap().DisplayOrientation = TRACKUP;
    break;
  case NORTHUP:
    XCSoarInterface::SetSettingsMap().DisplayOrientation = NORTHUP;
    break;
  case NORTHCIRCLE:
    XCSoarInterface::SetSettingsMap().DisplayOrientation = NORTHCIRCLE;
    break;
  case TRACKCIRCLE:
    XCSoarInterface::SetSettingsMap().DisplayOrientation = TRACKCIRCLE;
    break;
  case NORTHTRACK:
    XCSoarInterface::SetSettingsMap().DisplayOrientation = NORTHTRACK;
    break;
  }

  Temp = XCSoarInterface::SetSettingsMap().DisplayTextType;
  Profile::Get(szProfileDisplayText, Temp);
  switch (Temp) {
  case 0:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYNAME;
    break;
  case 1:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYNUMBER;
    break;
  case 2:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYFIRSTFIVE;
    break;
  case 3:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYNONE;
    break;
  case 4:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYFIRSTTHREE;
    break;
  case 5:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYNAMEIFINTASK;
    break;
  }

  Temp = XCSoarInterface::SetSettingsComputer().AltitudeMode;
  Profile::Get(szProfileAltMode, Temp);
  XCSoarInterface::SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)Temp;

  Profile::Get(szProfileClipAlt,
      XCSoarInterface::SetSettingsComputer().ClipAltitude);
  Profile::Get(szProfileAltMargin,
      XCSoarInterface::SetSettingsComputer().AltWarningMargin);

  Profile::Get(szProfileSafetyAltitudeArrival,
      XCSoarInterface::SetSettingsComputer().SafetyAltitudeArrival);
  Profile::Get(szProfileSafetyAltitudeBreakOff,
      XCSoarInterface::SetSettingsComputer().SafetyAltitudeBreakoff);
  Profile::Get(szProfileSafetyAltitudeTerrain,
      XCSoarInterface::SetSettingsComputer().SafetyAltitudeTerrain);
  Profile::Get(szProfileSafteySpeed,
      XCSoarInterface::SetSettingsComputer().SafetySpeed);
  Profile::Get(szProfilePolarID, 
      XCSoarInterface::SetSettingsComputer().POLARID);

  Profile::GetString(szProfileRegKey, strRegKey, 65);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    XCSoarInterface::SetSettingsComputer().iAirspaceMode[i] =
        GetRegistryAirspaceMode(i);

    Profile::Get(szProfileBrush[i],
        XCSoarInterface::SetSettingsMap().iAirspaceBrush[i]);
    Profile::Get(szProfileColour[i],
        XCSoarInterface::SetSettingsMap().iAirspaceColour[i]);
    if (XCSoarInterface::SettingsMap().iAirspaceColour[i] >= NUMAIRSPACECOLORS) {
      XCSoarInterface::SetSettingsMap().iAirspaceColour[i] = 0;
    }
    if (XCSoarInterface::SettingsMap().iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES) {
      XCSoarInterface::SetSettingsMap().iAirspaceBrush[i] = 0;
    }
  }

  Profile::Get(szProfileAirspaceBlackOutline,
      XCSoarInterface::SetSettingsMap().bAirspaceBlackOutline);
  Profile::Get(szProfileSnailTrail,
      XCSoarInterface::SetSettingsMap().TrailActive);

  Profile::Get(szProfileTrailDrift,
      XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  Profile::Get(szProfileThermalLocator,
      XCSoarInterface::SetSettingsComputer().EnableThermalLocator);

  Profile::Get(szProfileAnimation, XCSoarInterface::EnableAnimation);

  Profile::Get(szProfileDrawTopology,
      XCSoarInterface::SetSettingsMap().EnableTopology);

  Profile::Get(szProfileDrawTerrain,
      XCSoarInterface::SetSettingsMap().EnableTerrain);

  Profile::Get(szProfileFinalGlideTerrain,
      XCSoarInterface::SetSettingsComputer().FinalGlideTerrain);

  Profile::Get(szProfileAutoWind,
      XCSoarInterface::SetSettingsComputer().AutoWindMode);

  Profile::Get(szProfileCircleZoom,
      XCSoarInterface::SetSettingsMap().CircleZoom);

  Profile::Get(szProfileHomeWaypoint,
      XCSoarInterface::SetSettingsComputer().HomeWaypoint);

  Temp = XCSoarInterface::SettingsComputer().Alternate1;
  if (Profile::Get(szProfileAlternate1, Temp) == ERROR_SUCCESS) {
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    XCSoarInterface::SetSettingsComputer().Alternate1 = Temp;
    XCSoarInterface::SetSettingsComputer().EnableAlternate1 = true;
  } else {
    XCSoarInterface::SetSettingsComputer().Alternate1 = -1;
    XCSoarInterface::SetSettingsComputer().EnableAlternate1 = false;
  }

  Temp = XCSoarInterface::SettingsComputer().Alternate2;
  if (Profile::Get(szProfileAlternate2, Temp) == ERROR_SUCCESS) {
    XCSoarInterface::SetSettingsComputer().Alternate2 = Temp;
    XCSoarInterface::SetSettingsComputer().EnableAlternate2 = true;
  } else {
    XCSoarInterface::SetSettingsComputer().Alternate2 = -1;
    XCSoarInterface::SetSettingsComputer().EnableAlternate2 = false;
  }

  Profile::Get(szProfileSnailWidthScale,
      XCSoarInterface::SetSettingsMap().SnailWidthScale);

  Profile::Get(szProfileTeamcodeRefWaypoint,
      XCSoarInterface::SetSettingsComputer().TeamCodeRefWaypoint);

  Profile::Get(szProfileAirspaceWarning,
      XCSoarInterface::SetSettingsComputer().EnableAirspaceWarnings);

  Profile::Get(szProfileWarningTime,
      XCSoarInterface::SetSettingsComputer().WarningTime);

  Profile::Get(szProfileAcknowledgementTime,
      XCSoarInterface::SetSettingsComputer().AcknowledgementTime);

  Profile::Get(szProfileSoundVolume,
      XCSoarInterface::SetSettingsComputer().SoundVolume);

  Profile::Get(szProfileSoundDeadband,
      XCSoarInterface::SetSettingsComputer().SoundDeadband);

  Profile::Get(szProfileSoundAudioVario,
      XCSoarInterface::SetSettingsComputer().EnableSoundVario);

  Profile::Get(szProfileSoundTask,
      XCSoarInterface::SetSettingsComputer().EnableSoundTask);

  Profile::Get(szProfileSoundModes,
      XCSoarInterface::SetSettingsComputer().EnableSoundModes);

  XCSoarInterface::SetSettingsMap().EnableCDICruise = 0;
  XCSoarInterface::SetSettingsMap().EnableCDICircling = 0;

#ifdef HAVE_BLANK
  Profile::Get(szProfileAutoBlank,
      XCSoarInterface::SetSettingsMap().EnableAutoBlank);
#endif

  Profile::Get(szProfileAutoBacklight,
      XCSoarInterface::EnableAutoBacklight);
  Profile::Get(szProfileAutoSoundVolume,
      XCSoarInterface::EnableAutoSoundVolume);
  Profile::Get(szProfileExtendedVisualGlide,
      XCSoarInterface::SetSettingsMap().ExtendedVisualGlide);

#ifdef PNA
  Temp = 1;
#else
  Temp = 0;
#endif
  Profile::Get(szProfileVirtualKeys,Temp);
  XCSoarInterface::VirtualKeys = Temp;

  Temp = (AverEffTime_t)ae2minutes;
  Profile::Get(szProfileAverEffTime,Temp);
  XCSoarInterface::SetSettingsComputer().AverEffTime = Temp;

#if defined(GNAV) || defined(PCGNAV)
  Temp = 0;
#else
  Temp = 250;
#endif
  Profile::Get(szProfileDebounceTimeout, Temp);
  XCSoarInterface::debounceTimeout = Temp;

  /* JMW broken
  Temp = 100;
  Profile::Get(szProfileAccelerometerZero, Temp);
  AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    Profile::Set(szProfileAccelerometerZero, Temp);
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
    LogStartUp(_T("Loading HP31X settings"));
  } else if (GlobalModelType == MODELTYPE_PNA_PN6000 ) {
    LogStartUp(_T("Loading PN6000 settings"));
    // key transcoding for this one
  } else if (GlobalModelType == MODELTYPE_PNA_MIO ) {
    LogStartUp(_T("Loading MIO settings"));
    // currently no special settings from MIO but need to handle hw keys
  } else if (GlobalModelType == MODELTYPE_PNA_NOKIA_500 ) {
    LogStartUp(_T("Loading Nokia500 settings"));
    // key transcoding is made
  } else if (GlobalModelType == MODELTYPE_PNA_MEDION_P5 ) {
    LogStartUp(_T("Loading Medion settings"));
  } else if (GlobalModelType == MODELTYPE_PNA_PNA ) {
    LogStartUp(_T("Loading default PNA settings"));
  } else {
    LogStartUp(_T("No special regsets for this PDA"));
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
    unsigned t = XCSoarInterface::SettingsComputer().olc_rules;
    Profile::Get(szProfileOLCRules, t);
    XCSoarInterface::SetSettingsComputer().olc_rules = (OLCRules)t;
  }
  Profile::Get(szProfileHandicap,
      XCSoarInterface::SetSettingsComputer().olc_handicap);
  Profile::Get(szProfileEnableExternalTriggerCruise,
      XCSoarInterface::SetSettingsComputer().EnableExternalTriggerCruise);

  Profile::Get(szProfileUTCOffset,
      XCSoarInterface::SetSettingsComputer().UTCOffset);
  if (XCSoarInterface::SettingsComputer().UTCOffset > 12 * 3600)
    XCSoarInterface::SetSettingsComputer().UTCOffset -= 24 * 3600;

  Profile::Get(szProfileBlockSTF,
      XCSoarInterface::SetSettingsComputer().EnableBlockSTF);
  Profile::Get(szProfileAutoZoom,
      XCSoarInterface::SetSettingsMap().AutoZoom);
  Profile::Get(szProfileMenuTimeout,
      XCSoarInterface::MenuTimeoutMax);
  Profile::Get(szProfileLockSettingsInFlight,
      XCSoarInterface::LockSettingsInFlight);
  Profile::Get(szProfileLoggerShort,
      XCSoarInterface::SetSettingsComputer().LoggerShortName);
  Profile::Get(szProfileEnableFLARMMap,
      XCSoarInterface::SetSettingsMap().EnableFLARMMap);
  Profile::Get(szProfileEnableFLARMGauge,
      XCSoarInterface::SetSettingsMap().EnableFLARMGauge);
  Profile::Get(szProfileTerrainContrast,
      XCSoarInterface::SetSettingsMap().TerrainContrast);
  Profile::Get(szProfileTerrainBrightness,
      XCSoarInterface::SetSettingsMap().TerrainBrightness);
  Profile::Get(szProfileTerrainRamp,
      XCSoarInterface::SetSettingsMap().TerrainRamp);

  Profile::Get(szProfileGliderScreenPosition,
      XCSoarInterface::SetSettingsMap().GliderScreenPosition);
  Profile::Get(szProfileBallastSecsToEmpty,
      XCSoarInterface::SetSettingsComputer().BallastSecsToEmpty);
  Profile::Get(szProfileSetSystemTimeFromGPS,
      XCSoarInterface::SetSettingsMap().SetSystemTimeFromGPS);
  Profile::Get(szProfileUseCustomFonts,
      UseCustomFonts);
  Profile::Get(szProfileVoiceClimbRate,
      XCSoarInterface::SetSettingsComputer().EnableVoiceClimbRate);
  Profile::Get(szProfileVoiceTerrain,
      XCSoarInterface::SetSettingsComputer().EnableVoiceTerrain);
  Profile::Get(szProfileVoiceWaypointDistance,
      XCSoarInterface::SetSettingsComputer().EnableVoiceWaypointDistance);
  Profile::Get(szProfileVoiceTaskAltitudeDifference,
      XCSoarInterface::SetSettingsComputer().EnableVoiceTaskAltitudeDifference);
  Profile::Get(szProfileVoiceMacCready,
      XCSoarInterface::SetSettingsComputer().EnableVoiceMacCready);
  Profile::Get(szProfileVoiceNewWaypoint,
      XCSoarInterface::SetSettingsComputer().EnableVoiceNewWaypoint);
  Profile::Get(szProfileVoiceInSector,
      XCSoarInterface::SetSettingsComputer().EnableVoiceInSector);
  Profile::Get(szProfileVoiceAirspace,
      XCSoarInterface::SetSettingsComputer().EnableVoiceAirspace);
  Profile::Get(szProfileEnableNavBaroAltitude,
      XCSoarInterface::SetSettingsComputer().EnableNavBaroAltitude);
  Profile::Get(szProfileLoggerTimeStepCruise,
      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCruise);
  Profile::Get(szProfileLoggerTimeStepCircling,
      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCircling);
  Profile::Get(szProfileAbortSafetyUseCurrent,
      XCSoarInterface::SetSettingsComputer().safety_mc_use_current);

  Temp = iround(XCSoarInterface::SettingsComputer().safety_mc * 10);
  Profile::Get(szProfileSafetyMacCready, Temp);
  XCSoarInterface::SetSettingsComputer().safety_mc = Temp / 10.0;

  Profile::Get(szProfileUserLevel, XCSoarInterface::UserLevel);

  Temp = iround(XCSoarInterface::SettingsComputer().risk_gamma * 10);
  Profile::Get(szProfileRiskGamma, Temp);
  XCSoarInterface::SetSettingsComputer().risk_gamma = Temp / 10.0;

  Temp = (CompassAppearance_t)apCompassAltA;
  Profile::Get(szProfileWindArrowStyle, Temp);
  XCSoarInterface::SetSettingsMap().WindArrowStyle = Temp;

  Profile::Get(szProfileDisableAutoLogger,
      XCSoarInterface::SetSettingsComputer().DisableAutoLogger);
}

void
Profile::SetRegistryAirspaceMode(int i)
{
  int val = XCSoarInterface::SettingsComputer().iAirspaceMode[i];
  Profile::Set(szProfileAirspaceMode[i], val);
}

int
Profile::GetRegistryAirspaceMode(int i)
{
  int Temp = 3; // display + warnings
  Profile::Get(szProfileAirspaceMode[i], Temp);
  return Temp;
}

void
Profile::SetSoundSettings()
{
  Profile::Set(szProfileSoundVolume,
               XCSoarInterface::SettingsComputer().SoundVolume);
  Profile::Set(szProfileSoundDeadband,
               XCSoarInterface::SettingsComputer().SoundDeadband);
  Profile::Set(szProfileSoundAudioVario,
               XCSoarInterface::SettingsComputer().EnableSoundVario);
  Profile::Set(szProfileSoundTask,
               XCSoarInterface::SettingsComputer().EnableSoundTask);
  Profile::Set(szProfileSoundModes,
               XCSoarInterface::SettingsComputer().EnableSoundModes);
}

void
Profile::SetWind()
{
  int Temp;
  Temp = iround(XCSoarInterface::Basic().wind.norm);
  Profile::Set(szProfileWindSpeed, Temp);
  Temp = iround(XCSoarInterface::Basic().wind.bearing);
  Profile::Set(szProfileWindBearing, Temp);
  //TODO  SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing);
}

void
Profile::GetWind()
{
  LogStartUp(_T("Load wind from registry"));

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
Profile::Load()
{
  LogStartUp(_T("Loading profiles"));
  // load registry backup if it exists
  LoadFile(failsafeProfileFile);
  LoadFile(startProfileFile);
}

void
Profile::LoadFile(const TCHAR *szFile)
{
  if (string_is_empty(szFile))
    return;

  LogStartUp(TEXT("Loading profile from %s"), szFile);
  LoadRegistryFromFile(szFile);
}

void
Profile::Save()
{
  LogStartUp(_T("Saving profiles"));
  // save registry backup first (try a few places)
  SaveFile(startProfileFile);
  SaveFile(defaultProfileFile);
}

void
Profile::SaveFile(const TCHAR *szFile)
{
  if (string_is_empty(szFile))
    return;

  LogStartUp(TEXT("Saving profile to %s"), szFile);
  SaveRegistryToFile(szFile);
}


void
Profile::SetFiles(const TCHAR* override)
{
  // Set the default profile file
  if (is_altair())
    LocalPath(defaultProfileFile, _T("config/")_T(XCSPROFILE));
  else
    LocalPath(defaultProfileFile, _T(XCSPROFILE));

  // Set the failsafe profile file
  LocalPath(failsafeProfileFile, _T(XCSPROFILE));

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
Profile::GetString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
  return GetRegistryString(szRegValue, pPos, dwSize);
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

bool
Profile::SetString(const TCHAR *szRegValue, const TCHAR *Pos)
{
  return SetRegistryString(szRegValue, Pos);
}

void
Profile::SetStringIfAbsent(const TCHAR *szRegValue, const TCHAR *Pos)
{
  TCHAR temp[MAX_PATH];
  if (!GetString(szRegValue, temp, MAX_PATH))
    SetString(szRegValue, Pos);
}

int
Profile::GetScaleList(fixed *List, size_t Size)
{
  static const TCHAR Name[] = TEXT("ScaleList");
  TCHAR Buffer[128];
  TCHAR *pWClast, *pToken;
  int Idx = 0;
  double vlast = 0;
  double val;

  assert(List != NULL);
  assert(Size > 0);

  SetRegistryString(Name, TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));

  if (!Profile::GetString(Name, Buffer, sizeof(Buffer) / sizeof(TCHAR)))
    return 0;

  pToken = _tcstok_r(Buffer, TEXT(","), &pWClast);

  while (Idx < (int)Size && pToken != NULL) {
    val = _tcstod(pToken, NULL);
    if (Idx > 0) {
      List[Idx] = (val + vlast) / 2;
      Idx++;
    }
    List[Idx] = val;
    Idx++;
    vlast = val;
    pToken = _tcstok_r(NULL, TEXT(","), &pWClast);
  }

  return Idx;
}
