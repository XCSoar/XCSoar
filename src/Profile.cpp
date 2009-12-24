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
#include "Math/Units.h"
#include "Appearance.hpp"
#include "InfoBoxManager.h"
#include "GlideRatio.hpp"
#include "Screen/Fonts.hpp"
#include "Asset.hpp"

#define CheckIndex(x, i) do {} while (false) // XXX

extern int WaypointsOutOfRange;

static void
DefaultRegistrySettingsAltair()
{
  // RLD left GNAV Altair settings untouched.
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
    iszero_fg  &= (InfoBoxManager::getType(i, 2)==0);
    iszero_aux &= (InfoBoxManager::getType(i, 3)==0);
  }
  if (iszero_fg || iszero_aux) {
    for (i = 0; i < MAXINFOWINDOWS; ++i) {
      if (iszero_fg) {
        InfoBoxManager::setType(i, InfoBoxManager::getType(i, 1), 2);
      }
      if (iszero_aux) {
        InfoBoxManager::setType(i, InfoBoxManager::getType(i, 1), 3);
      }
      StoreType(i, InfoBoxManager::getTypeAll(i));
    }
  }
}

void
Profile::ReadRegistrySettings()
{
  DWORD Speed = 0;
  DWORD Distance = 0;
  DWORD TaskSpeed = 0;
  DWORD Lift = 0;
  DWORD Altitude = 0;
  DWORD Temp = 0;
  int i;

  StartupStore(TEXT("Read registry settings\n"));

  if (is_altair())
    DefaultRegistrySettingsAltair();

#ifdef OLD_TASK
  SETTINGS_TASK settings_task = task.getSettings();
  GetFromRegistry(szRegistryFinishMinHeight,
		  settings_task.FinishMinHeight);
  GetFromRegistry(szRegistryStartHeightRef,
		  settings_task.StartHeightRef);
  GetFromRegistry(szRegistryStartMaxHeight,
		  settings_task.StartMaxHeight);
  GetFromRegistry(szRegistryStartMaxHeightMargin,
		  settings_task.StartMaxHeightMargin);
  GetFromRegistry(szRegistryStartMaxSpeed,
		  settings_task.StartMaxSpeed);
  GetFromRegistry(szRegistryStartMaxSpeedMargin,
		  settings_task.StartMaxSpeedMargin);

  Temp = settings_task.SectorType;
  GetFromRegistryD(szRegistryFAISector, Temp);
  settings_task.SectorType = (ASTSectorType_t)Temp;

  Temp = settings_task.StartType;
  GetFromRegistryD(szRegistryStartLine, Temp);
  settings_task.StartType = (StartSectorType_t)Temp;

  Temp = settings_task.FinishType;
  GetFromRegistryD(szRegistryFinishLine, Temp);
  settings_task.FinishType = (FinishSectorType_t)Temp;

  GetFromRegistry(szRegistrySectorRadius,
      settings_task.SectorRadius);

  GetFromRegistry(szRegistryStartRadius,
      settings_task.StartRadius);
  GetFromRegistry(szRegistryFinishRadius,
      settings_task.FinishRadius);

  Temp = settings_task.AutoAdvance;
  GetFromRegistryD(szRegistryAutoAdvance, Temp);
  settings_task.AutoAdvance = (AutoAdvanceMode_t)Temp;

  GetFromRegistry(szRegistryFAIFinishHeight,
		  settings_task.EnableFAIFinishHeight);
  task.setSettings(settings_task);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    GetFromRegistry(szRegistryAirspacePriority[i], AirspacePriority[i]);
  }
#endif

  Temp = 0;
  GetFromRegistryD(szRegistryLatLonUnits, Temp);
  Units::CoordinateFormat = (CoordinateFormats_t)Temp;

  GetFromRegistryD(szRegistrySpeedUnitsValue, Speed);
  switch (Speed) {
  case 0:
    SPEEDMODIFY = TOMPH;
    break;
  case 1:
    SPEEDMODIFY = TOKNOTS;
    break;
  case 2:
    SPEEDMODIFY = TOKPH;
    break;
  }

  TaskSpeed = 2;
  GetFromRegistryD(szRegistryTaskSpeedUnitsValue, TaskSpeed);
  switch (TaskSpeed) {
  case 0:
    TASKSPEEDMODIFY = TOMPH;
    break;
  case 1:
    TASKSPEEDMODIFY = TOKNOTS;
    break;
  case 2:
    TASKSPEEDMODIFY = TOKPH;
    break;
  }

  GetFromRegistryD(szRegistryDistanceUnitsValue,Distance);
  switch (Distance) {
  case 0:
    DISTANCEMODIFY = TOMILES;
    break;
  case 1:
    DISTANCEMODIFY = TONAUTICALMILES;
    break;
  case 2:
    DISTANCEMODIFY = TOKILOMETER;
    break;
  }

  GetFromRegistryD(szRegistryAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    ALTITUDEMODIFY = TOFEET;
    break;
  case 1:
    ALTITUDEMODIFY = TOMETER;
    break;
  }

  GetFromRegistryD(szRegistryLiftUnitsValue, Lift);
  switch (Lift) {
  case 0:
    LIFTMODIFY = TOKNOTS;
    break;
  case 1:
    LIFTMODIFY = TOMETER;
    break;
  }

  Units::NotifyUnitChanged();

  for (i = 0; i < MAXINFOWINDOWS; i++) {
    Temp = InfoBoxManager::getTypeAll(i);
    GetFromRegistryD(szRegistryDisplayType[i], Temp);
    InfoBoxManager::setTypeAll(i, Temp);
  }

  // check against V3 infotypes
  CheckInfoTypes();

  Temp = SetSettingsMap().DisplayOrientation;
  GetFromRegistryD(szRegistryDisplayUpValue, Temp);
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
  GetFromRegistryD(szRegistryDisplayText, Temp);
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
  GetFromRegistryD(szRegistryAltMode, Temp);
  SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)Temp;

  GetFromRegistry(szRegistryClipAlt,
      SetSettingsComputer().ClipAltitude);
  GetFromRegistry(szRegistryAltMargin,
      SetSettingsComputer().AltWarningMargin);

  GetFromRegistry(szRegistrySafetyAltitudeArrival,
		  SetSettingsComputer().SafetyAltitudeArrival);
  GetFromRegistry(szRegistrySafetyAltitudeBreakOff,
		  SetSettingsComputer().SafetyAltitudeBreakoff);
  GetFromRegistry(szRegistrySafetyAltitudeTerrain,
		  SetSettingsComputer().SafetyAltitudeTerrain);
  GetFromRegistry(szRegistrySafteySpeed,
		  SetSettingsComputer().SafetySpeed);
  GetFromRegistry(szRegistryPolarID, 
                  SetSettingsComputer().POLARID);

  GetRegistryString(szRegistryRegKey, strRegKey, 65);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    SetSettingsComputer().iAirspaceMode[i] = GetRegistryAirspaceMode(i);

    GetFromRegistry(szRegistryBrush[i], SetSettingsMap().iAirspaceBrush[i]);
    GetFromRegistry(szRegistryColour[i], SetSettingsMap().iAirspaceColour[i]);
    if (SettingsMap().iAirspaceColour[i] >= NUMAIRSPACECOLORS) {
      SetSettingsMap().iAirspaceColour[i] = 0;
    }
    if (SettingsMap().iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES) {
      SetSettingsMap().iAirspaceBrush[i] = 0;
    }
  }

  GetFromRegistry(szRegistryAirspaceBlackOutline,
		  SetSettingsMap().bAirspaceBlackOutline);
  GetFromRegistry(szRegistrySnailTrail,
		  SetSettingsMap().TrailActive);

  GetFromRegistry(szRegistryTrailDrift,
		  SetSettingsMap().EnableTrailDrift);

  GetFromRegistry(szRegistryThermalLocator,
		  SetSettingsComputer().EnableThermalLocator);

  GetFromRegistry(szRegistryAnimation, EnableAnimation);

  GetFromRegistry(szRegistryDrawTopology,
		  SetSettingsMap().EnableTopology);

  GetFromRegistry(szRegistryDrawTerrain,
		  SetSettingsMap().EnableTerrain);

  GetFromRegistry(szRegistryFinalGlideTerrain,
		  SetSettingsComputer().FinalGlideTerrain);

  GetFromRegistry(szRegistryAutoWind,
		  SetSettingsComputer().AutoWindMode);

  GetFromRegistry(szRegistryCircleZoom,
		  SetSettingsMap().CircleZoom);

  GetFromRegistry(szRegistryHomeWaypoint,
      SetSettingsComputer().HomeWaypoint);

  // VENTA3
  Temp = SettingsComputer().Alternate1;
  if (GetFromRegistryD(szRegistryAlternate1, Temp) == ERROR_SUCCESS) {
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    SetSettingsComputer().Alternate1 = Temp;
    SetSettingsComputer().EnableAlternate1 = true;
  } else {
    SetSettingsComputer().Alternate1 = -1;
    SetSettingsComputer().EnableAlternate1 = false;
  }

  Temp = SettingsComputer().Alternate2;
  if (GetFromRegistryD(szRegistryAlternate2, Temp) == ERROR_SUCCESS) {
    SetSettingsComputer().Alternate2 = Temp;
    SetSettingsComputer().EnableAlternate2 = true;
  } else {
    SetSettingsComputer().Alternate2 = -1;
    SetSettingsComputer().EnableAlternate2 = false;
  }


  GetFromRegistry(szRegistrySnailWidthScale,
		  SetSettingsMap().SnailWidthScale);

  GetFromRegistry(szRegistryTeamcodeRefWaypoint,
		  SetSettingsComputer().TeamCodeRefWaypoint);

  GetFromRegistry(szRegistryAirspaceWarning,
		  SetSettingsComputer().EnableAirspaceWarnings);

  GetFromRegistry(szRegistryWarningTime,
		  SetSettingsComputer().WarningTime);

  GetFromRegistry(szRegistryAcknowledgementTime,
		  SetSettingsComputer().AcknowledgementTime);

  GetFromRegistry(szRegistrySoundVolume,
		  SetSettingsComputer().SoundVolume);

  GetFromRegistry(szRegistrySoundDeadband,
		  SetSettingsComputer().SoundDeadband);

  GetFromRegistry(szRegistrySoundAudioVario,
		  SetSettingsComputer().EnableSoundVario);

  GetFromRegistry(szRegistrySoundTask,
		  SetSettingsComputer().EnableSoundTask);

  GetFromRegistry(szRegistrySoundModes,
		  SetSettingsComputer().EnableSoundModes);

  /* no longer used
  Temp = 500;
  GetFromRegistry(szRegistryNettoSpeed,&Temp);
  NettoSpeed = Temp;
  */

  SetSettingsMap().EnableCDICruise = 0;
  SetSettingsMap().EnableCDICircling = 0;

  /* JMW temporarily disabled these because they are not updated for 4.7+
  Temp = 0;
  GetFromRegistry(szRegistryCDICruise,&Temp);
  EnableCDICruise = (Temp == 1);

  Temp = 0;
  GetFromRegistry(szRegistryCDICircling,&Temp);
  EnableCDICircling = (Temp == 1);
  */

#ifdef HAVE_BLANK
  GetFromRegistry(szRegistryAutoBlank,
		  SetSettingsMap().EnableAutoBlank);
#endif

  GetFromRegistry(szRegistryAutoBacklight,
		  EnableAutoBacklight);
  GetFromRegistry(szRegistryAutoSoundVolume,
		  EnableAutoSoundVolume);
  GetFromRegistry(szRegistryExtendedVisualGlide,
		  SetSettingsMap().ExtendedVisualGlide);

#ifdef PNA
  Temp = 1;
#else
  Temp = 0;
#endif
  GetFromRegistryD(szRegistryVirtualKeys,Temp);
  VirtualKeys = Temp;

  Temp = (AverEffTime_t)ae2minutes;
  GetFromRegistryD(szRegistryAverEffTime,Temp);
  SetSettingsComputer().AverEffTime = Temp;

  /*
  Temp = 0;
  GetFromRegistry(szRegistryVarioGauge,&Temp);
  EnableVarioGauge = (Temp == 1);
  */

#if defined(GNAV) || defined(PCGNAV)
  Temp = 0;
#else
  Temp = 250;
#endif
  GetFromRegistryD(szRegistryDebounceTimeout, Temp);
  debounceTimeout = Temp;

  /* JMW broken
  Temp = 100;
  GetFromRegistry(szRegistryAccelerometerZero, Temp);
  AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    SetToRegistry(szRegistryAccelerometerZero, Temp);
  }
  */

  // new appearance variables

  //Temp = Appearance.IndFinalGlide;
  Temp = (IndFinalGlide_t)fgFinalGlideDefault; // VNT9 default
  GetFromRegistryD(szRegistryAppIndFinalGlide, Temp);
  Appearance.IndFinalGlide = (IndFinalGlide_t)Temp;

  Temp = Appearance.IndLandable;
  GetFromRegistryD(szRegistryAppIndLandable, Temp);
  Appearance.IndLandable = (IndLandable_t)Temp;

  GetFromRegistry(szRegistryAppInverseInfoBox,
		  Appearance.InverseInfoBox);
  GetFromRegistry(szRegistryAppGaugeVarioSpeedToFly,
		  Appearance.GaugeVarioSpeedToFly);
  GetFromRegistry(szRegistryAppGaugeVarioAvgText,
		  Appearance.GaugeVarioAvgText);
  GetFromRegistry(szRegistryAppGaugeVarioMc,
		  Appearance.GaugeVarioMc);
  GetFromRegistry(szRegistryAppGaugeVarioBugs,
		  Appearance.GaugeVarioBugs);
  GetFromRegistry(szRegistryAppGaugeVarioBallast,
		  Appearance.GaugeVarioBallast);
  GetFromRegistry(szRegistryAppGaugeVarioGross,
		  Appearance.GaugeVarioGross);

  Temp = Appearance.CompassAppearance;
  GetFromRegistryD(szRegistryAppCompassAppearance, Temp);
  Appearance.CompassAppearance = (CompassAppearance_t)Temp;

  //Temp = Appearance.InfoBoxBorder;
  Temp = (InfoBoxBorderAppearance_t)apIbBox; // VNT9 default
  GetFromRegistryD(szRegistryAppInfoBoxBorder, Temp);
  Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)Temp;

  // VENTA2-ADDON Geometry change and PNA custom font settings
  // depending on infobox geometry and model type
  // I had to move here the font setting because I needed first to
  // know the screen geometry, in the registry!
#if defined(PNA) || defined(FIVV)
  Temp = Appearance.InfoBoxGeom;
  GetFromRegistryD(szRegistryAppInfoBoxGeom, Temp);
  Appearance.InfoBoxGeom = (InfoBoxGeomAppearance_t)Temp;

  if (GlobalModelType == MODELTYPE_PNA_HP31X ) {
    // key transcoding for this one
    StartupStore(TEXT("Loading HP31X settings\n"));
  } else if (GlobalModelType == MODELTYPE_PNA_PN6000 ) {
    StartupStore(TEXT("Loading PN6000 settings\n"));
    // key transcoding for this one
  } else if (GlobalModelType == MODELTYPE_PNA_MIO ) {
    StartupStore(TEXT("Loading MIO settings\n"));
    // currently no special settings from MIO but need to handle hw keys
  } else if (GlobalModelType == MODELTYPE_PNA_NOKIA_500 ) {
    StartupStore(TEXT("Loading Nokia500 settings\n"));
    // key transcoding is made
  } else if (GlobalModelType == MODELTYPE_PNA_MEDION_P5 ) {
    StartupStore(TEXT("Loading Medion settings\n"));
  } else if (GlobalModelType == MODELTYPE_PNA_PNA ) {
    StartupStore(TEXT("Loading default PNA settings\n"));
  } else {
    StartupStore(TEXT("No special regsets for this PDA\n")); // VENTA2
  }

// VENTA-ADDON Model change
  Temp = Appearance.InfoBoxModel;
  GetFromRegistryD(szRegistryAppInfoBoxModel, Temp);
  Appearance.InfoBoxModel = (InfoBoxModelAppearance_t)Temp;
#endif

  Temp = Appearance.StateMessageAlign;
  GetFromRegistryD(szRegistryAppStatusMessageAlignment, Temp);
  Appearance.StateMessageAlign = (StateMessageAlign_t)Temp;

  Temp = Appearance.TextInputStyle;
  GetFromRegistryD(szRegistryAppTextInputStyle, Temp);
  Appearance.TextInputStyle = (TextInputStyle_t)Temp;

  GetFromRegistry(szRegistryAppDefaultMapWidth,
		  Appearance.DefaultMapWidth);
  GetFromRegistry(szRegistryAppInfoBoxColors,
		  Appearance.InfoBoxColors);
  GetFromRegistry(szRegistryAppAveNeedle,
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
  GetFromRegistry(szRegistryAutoMcMode,
		  SetSettingsComputer().AutoMacCreadyMode);
#endif
  GetFromRegistry(szRegistryWaypointsOutOfRange,
		  WaypointsOutOfRange);
  GetFromRegistry(szRegistryOLCRules,
		  SetSettingsComputer().OLCRules);
  GetFromRegistry(szRegistryHandicap,
		  SetSettingsComputer().Handicap);
  GetFromRegistry(szRegistryEnableExternalTriggerCruise,
		  SetSettingsComputer().EnableExternalTriggerCruise);

  GetFromRegistry(szRegistryUTCOffset,
		  SetSettingsComputer().UTCOffset);
  if (SettingsComputer().UTCOffset > 12 * 3600) {
    SetSettingsComputer().UTCOffset -= 24 * 3600;
  }

  GetFromRegistry(szRegistryBlockSTF,
		  SetSettingsComputer().EnableBlockSTF);
  GetFromRegistry(szRegistryAutoZoom,
		  SetSettingsMap().AutoZoom);
  GetFromRegistry(szRegistryMenuTimeout,
		  MenuTimeoutMax);
  GetFromRegistry(szRegistryLockSettingsInFlight,
		  LockSettingsInFlight);
  GetFromRegistry(szRegistryLoggerShort,
		  SetSettingsComputer().LoggerShortName);
  GetFromRegistry(szRegistryEnableFLARMMap,
		  SetSettingsMap().EnableFLARMMap);
  GetFromRegistry(szRegistryEnableFLARMGauge,
		  SetSettingsMap().EnableFLARMGauge);
  GetFromRegistry(szRegistryTerrainContrast,
		  SetSettingsMap().TerrainContrast);
  GetFromRegistry(szRegistryTerrainBrightness,
		  SetSettingsMap().TerrainBrightness);
  GetFromRegistry(szRegistryTerrainRamp,
		  SetSettingsMap().TerrainRamp);

  GetFromRegistry(szRegistryGliderScreenPosition,
		  SetSettingsMap().GliderScreenPosition);
  GetFromRegistry(szRegistryBallastSecsToEmpty,
		  SetSettingsComputer().BallastSecsToEmpty);
  GetFromRegistry(szRegistrySetSystemTimeFromGPS,
		  SetSettingsMap().SetSystemTimeFromGPS);
  GetFromRegistry(szRegistryAutoForceFinalGlide,
		  SetSettingsComputer().AutoForceFinalGlide);
  GetFromRegistry(szRegistryUseCustomFonts,
		  UseCustomFonts);
  GetFromRegistry(szRegistryVoiceClimbRate,
		  SetSettingsComputer().EnableVoiceClimbRate);
  GetFromRegistry(szRegistryVoiceTerrain,
		  SetSettingsComputer().EnableVoiceTerrain);
  GetFromRegistry(szRegistryVoiceWaypointDistance,
		  SetSettingsComputer().EnableVoiceWaypointDistance);
  GetFromRegistry(szRegistryVoiceTaskAltitudeDifference,
		  SetSettingsComputer().EnableVoiceTaskAltitudeDifference);
  GetFromRegistry(szRegistryVoiceMacCready,
		  SetSettingsComputer().EnableVoiceMacCready);
  GetFromRegistry(szRegistryVoiceNewWaypoint,
		  SetSettingsComputer().EnableVoiceNewWaypoint);
  GetFromRegistry(szRegistryVoiceInSector,
		  SetSettingsComputer().EnableVoiceInSector);
  GetFromRegistry(szRegistryVoiceAirspace,
		  SetSettingsComputer().EnableVoiceAirspace);
  GetFromRegistry(szRegistryEnableNavBaroAltitude,
		  SetSettingsComputer().EnableNavBaroAltitude);
  GetFromRegistry(szRegistryLoggerTimeStepCruise,
		  SetSettingsComputer().LoggerTimeStepCruise);
  GetFromRegistry(szRegistryLoggerTimeStepCircling,
		  SetSettingsComputer().LoggerTimeStepCircling);
  GetFromRegistry(szRegistryAbortSafetyUseCurrent,
		  SetSettingsComputer().AbortSafetyUseCurrent);

  Temp = iround(SettingsComputer().SafetyMacCready * 10);
  GetFromRegistryD(szRegistrySafetyMacCready, Temp);
  SetSettingsComputer().SafetyMacCready = Temp / 10.0;

  GetFromRegistry(szRegistryUserLevel, UserLevel);

  Temp = iround(SettingsComputer().risk_gamma * 10);
  GetFromRegistryD(szRegistryRiskGamma, Temp);
  SetSettingsComputer().risk_gamma = Temp / 10.0;

  Temp = (CompassAppearance_t)apCompassAltA; // VNT9 default
  GetFromRegistryD(szRegistryWindArrowStyle, Temp);
  SetSettingsMap().WindArrowStyle = Temp;

  GetFromRegistry(szRegistryDisableAutoLogger,
		  SetSettingsComputer().DisableAutoLogger);
}

void
Profile::SetRegistryAirspaceMode(int i)
{
  CheckIndex(SetSettingsComputer().iAirspaceMode, i);
  CheckIndex(szRegistryAirspaceMode, i);

  DWORD val = SettingsComputer().iAirspaceMode[i];
  SetToRegistry(szRegistryAirspaceMode[i], val);
}

int
Profile::GetRegistryAirspaceMode(int i)
{
  DWORD Temp = 3; // display + warnings
  CheckIndex(szRegistryAirspaceMode, i);
  GetFromRegistryD(szRegistryAirspaceMode[i], Temp);
  return Temp;
}

void
Profile::SaveSoundSettings()
{
  SetToRegistry(szRegistrySoundVolume, (DWORD)SettingsComputer().SoundVolume);
  SetToRegistry(szRegistrySoundDeadband, (DWORD)SettingsComputer().SoundDeadband);
  SetToRegistry(szRegistrySoundAudioVario, SettingsComputer().EnableSoundVario);
  SetToRegistry(szRegistrySoundTask, SettingsComputer().EnableSoundTask);
  SetToRegistry(szRegistrySoundModes, SettingsComputer().EnableSoundModes);
}


void
Profile::SaveWindToRegistry()
{
  DWORD Temp;
  Temp = iround(Basic().WindSpeed);
  SetToRegistry(szRegistryWindSpeed, Temp);
  Temp = iround(Basic().WindDirection);
  SetToRegistry(szRegistryWindBearing, Temp);
  //TODO  SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing);
}

void
Profile::LoadWindFromRegistry()
{
  StartupStore(TEXT("Load wind from registry\n"));

  /* JMW incomplete
  DWORD Temp;
  Temp=0;
  GetFromRegistry(szRegistryWindSpeed,&Temp);
  Calculated().WindSpeed = Temp;
  Temp=0;
  GetFromRegistry(szRegistryWindBearing,&Temp);
  Calculated().WindBearing = Temp;
  */
}
