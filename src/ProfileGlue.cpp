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
#include "LogFile.hpp"
#include "Appearance.hpp"
#include "InfoBoxManager.hpp"
#include "GlideRatio.hpp"
#include "Screen/Fonts.hpp"
#include "Asset.hpp"
#include "Dialogs/XML.hpp"
#include "WayPointFile.hpp"
#include "UtilsText.hpp"
#include "UtilsFont.hpp"
#include "InfoBoxLayout.hpp"

#include <assert.h>

static void
SetAltairDefaults()
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
  if (InfoBoxManager::IsEmpty(1))
    return;

  bool iszero_fg = InfoBoxManager::IsEmpty(2);
  bool iszero_aux = InfoBoxManager::IsEmpty(3);
  if (!iszero_fg && !iszero_aux)
    return;

  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i) {
    if (iszero_fg)
      InfoBoxManager::setType(i, InfoBoxManager::getType(i, 1), 2);
    if (iszero_aux)
      InfoBoxManager::setType(i, InfoBoxManager::getType(i, 1), 3);
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
    SetAltairDefaults();

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

  Temp = 0;
  Get(szProfileLatLonUnits, Temp);
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
    Temp = InfoBoxManager::getTypeAll(i);
    Get(szProfileDisplayType[i], Temp);
    InfoBoxManager::setTypeAll(i, Temp);
  }

  // check against V3 infotypes
  CheckInfoTypes();

  Temp = XCSoarInterface::SetSettingsMap().DisplayOrientation;
  Get(szProfileDisplayUpValue, Temp);
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
  Get(szProfileDisplayText, Temp);
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
  case 6:
    XCSoarInterface::SetSettingsMap().DisplayTextType = DISPLAYUNTILSPACE;
    break;
  }

  Temp = XCSoarInterface::SetSettingsComputer().AltitudeMode;
  Get(szProfileAltMode, Temp);
  XCSoarInterface::SetSettingsComputer().AltitudeMode = (AirspaceDisplayMode_t)Temp;

  Get(szProfileClipAlt,
      XCSoarInterface::SetSettingsComputer().ClipAltitude);
  Get(szProfileAltMargin,
      XCSoarInterface::SetSettingsComputer().AltWarningMargin);

  Get(szProfileSafetyAltitudeArrival,
      XCSoarInterface::SetSettingsComputer().safety_height_arrival);
  Get(szProfileSafetyAltitudeTerrain,
      XCSoarInterface::SetSettingsComputer().safety_height_terrain);
  Get(szProfileSafteySpeed,
      XCSoarInterface::SetSettingsComputer().SafetySpeed);
  Get(szProfilePolarID,
      XCSoarInterface::SetSettingsComputer().POLARID);

  Get(szProfileRegKey, strRegKey, 65);

  for (i = 0; i < AIRSPACECLASSCOUNT; i++) {
    XCSoarInterface::SetSettingsComputer().iAirspaceMode[i] =
        GetAirspaceMode(i);

    Get(szProfileBrush[i],
        XCSoarInterface::SetSettingsMap().iAirspaceBrush[i]);
    Get(szProfileColour[i],
        XCSoarInterface::SetSettingsMap().iAirspaceColour[i]);
    if (XCSoarInterface::SettingsMap().iAirspaceColour[i] >= NUMAIRSPACECOLORS) {
      XCSoarInterface::SetSettingsMap().iAirspaceColour[i] = 0;
    }
    if (XCSoarInterface::SettingsMap().iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES) {
      XCSoarInterface::SetSettingsMap().iAirspaceBrush[i] = 0;
    }
  }

  Get(szProfileAirspaceBlackOutline,
      XCSoarInterface::SetSettingsMap().bAirspaceBlackOutline);
  Get(szProfileSnailTrail,
      XCSoarInterface::SetSettingsMap().TrailActive);

  Get(szProfileTrailDrift,
      XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  Get(szProfileAnimation, XCSoarInterface::EnableAnimation);

  Get(szProfileDrawTopology,
      XCSoarInterface::SetSettingsMap().EnableTopology);

  Get(szProfileDrawTerrain,
      XCSoarInterface::SetSettingsMap().EnableTerrain);

  Get(szProfileFinalGlideTerrain,
      XCSoarInterface::SetSettingsComputer().FinalGlideTerrain);

  Get(szProfileAutoWind,
      XCSoarInterface::SetSettingsComputer().AutoWindMode);

  Get(szProfileCircleZoom,
      XCSoarInterface::SetSettingsMap().CircleZoom);

  Get(szProfileHomeWaypoint,
      XCSoarInterface::SetSettingsComputer().HomeWaypoint);

  Temp = XCSoarInterface::SettingsComputer().Alternate1;
  if (Get(szProfileAlternate1, Temp)) {
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    XCSoarInterface::SetSettingsComputer().Alternate1 = Temp;
    XCSoarInterface::SetSettingsComputer().EnableAlternate1 = true;
  } else {
    XCSoarInterface::SetSettingsComputer().Alternate1 = -1;
    XCSoarInterface::SetSettingsComputer().EnableAlternate1 = false;
  }

  Temp = XCSoarInterface::SettingsComputer().Alternate2;
  if (Get(szProfileAlternate2, Temp)) {
    XCSoarInterface::SetSettingsComputer().Alternate2 = Temp;
    XCSoarInterface::SetSettingsComputer().EnableAlternate2 = true;
  } else {
    XCSoarInterface::SetSettingsComputer().Alternate2 = -1;
    XCSoarInterface::SetSettingsComputer().EnableAlternate2 = false;
  }

  Get(szProfileSnailWidthScale,
      XCSoarInterface::SetSettingsMap().SnailWidthScale);

  Get(szProfileTeamcodeRefWaypoint,
      XCSoarInterface::SetSettingsComputer().TeamCodeRefWaypoint);

  Get(szProfileAirspaceWarning,
      XCSoarInterface::SetSettingsComputer().EnableAirspaceWarnings);

  Get(szProfileWarningTime,
      XCSoarInterface::SetSettingsComputer().WarningTime);

  Get(szProfileAcknowledgementTime,
      XCSoarInterface::SetSettingsComputer().AcknowledgementTime);

  Get(szProfileSoundVolume,
      XCSoarInterface::SetSettingsComputer().SoundVolume);

  Get(szProfileSoundDeadband,
      XCSoarInterface::SetSettingsComputer().SoundDeadband);

  Get(szProfileSoundAudioVario,
      XCSoarInterface::SetSettingsComputer().EnableSoundVario);

  Get(szProfileSoundTask,
      XCSoarInterface::SetSettingsComputer().EnableSoundTask);

  Get(szProfileSoundModes,
      XCSoarInterface::SetSettingsComputer().EnableSoundModes);

  XCSoarInterface::SetSettingsMap().EnableCDICruise = 0;
  XCSoarInterface::SetSettingsMap().EnableCDICircling = 0;

#ifdef HAVE_BLANK
  Get(szProfileAutoBlank,
      XCSoarInterface::SetSettingsMap().EnableAutoBlank);
#endif

  Get(szProfileAutoBacklight,
      XCSoarInterface::EnableAutoBacklight);
  Get(szProfileAutoSoundVolume,
      XCSoarInterface::EnableAutoSoundVolume);
  Get(szProfileExtendedVisualGlide,
      XCSoarInterface::SetSettingsMap().ExtendedVisualGlide);

  Get(szProfileGestures,
      XCSoarInterface::SetSettingsComputer().EnableGestures);

  Temp = (AverEffTime_t)ae2minutes;
  Get(szProfileAverEffTime,Temp);
  XCSoarInterface::SetSettingsComputer().AverEffTime = Temp;

#if defined(GNAV) || defined(PCGNAV)
  Temp = 0;
#else
  Temp = 250;
#endif
  Get(szProfileDebounceTimeout, Temp);
  XCSoarInterface::debounceTimeout = Temp;

  /* JMW broken
  Temp = 100;
  Get(szProfileAccelerometerZero, Temp);
  AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    Set(szProfileAccelerometerZero, Temp);
  }
  */

  // new appearance variables

  //Temp = Appearance.IndFinalGlide;
  Temp = (IndFinalGlide_t)fgFinalGlideDefault;
  Get(szProfileAppIndFinalGlide, Temp);
  Appearance.IndFinalGlide = (IndFinalGlide_t)Temp;

  Temp = Appearance.IndLandable;
  Get(szProfileAppIndLandable, Temp);
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

  Temp = Appearance.CompassAppearance;
  Get(szProfileAppCompassAppearance, Temp);
  Appearance.CompassAppearance = (CompassAppearance_t)Temp;

  Temp = (InfoBoxBorderAppearance_t)apIbBox;
  Get(szProfileAppInfoBoxBorder, Temp);
  Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)Temp;

  // VENTA2-ADDON Geometry change and PNA custom font settings
  // depending on infobox geometry and model type
  // I had to move here the font setting because I needed first to
  // know the screen geometry, in the registry!
#if defined(PNA) || defined(FIVV)
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
  Get(szProfileAppInfoBoxModel, Temp);
  Appearance.InfoBoxModel = (InfoBoxModelAppearance_t)Temp;
#endif

  Temp = Appearance.StateMessageAlign;
  Get(szProfileAppStatusMessageAlignment, Temp);
  Appearance.StateMessageAlign = (StateMessageAlign_t)Temp;

  Appearance.TextInputStyle = Get(szProfileAppTextInputStyle, Temp)
    ? (TextInputStyle_t)Temp
    : tiDefault;

  Temp = g_eDialogStyle;
  Get(szProfileAppDialogStyle, Temp);
  g_eDialogStyle = (DialogStyle_t)Temp;

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

  {
    unsigned t = XCSoarInterface::SettingsComputer().auto_mc_mode;
    Get(szProfileAutoMcMode, t);
    XCSoarInterface::SetSettingsComputer().auto_mc_mode = (TaskBehaviour::AutoMCMode_t)t;
  }

  Get(szProfileWaypointsOutOfRange,
                  WayPointFile::WaypointsOutOfRangeSetting);
  {
    unsigned t = XCSoarInterface::SettingsComputer().olc_rules;
    Get(szProfileOLCRules, t);
    XCSoarInterface::SetSettingsComputer().olc_rules = (OLCRules)t;
  }
  Get(szProfileHandicap,
      XCSoarInterface::SetSettingsComputer().olc_handicap);
  Get(szProfileEnableExternalTriggerCruise,
      XCSoarInterface::SetSettingsComputer().EnableExternalTriggerCruise);

  Get(szProfileUTCOffset,
      XCSoarInterface::SetSettingsComputer().UTCOffset);
  if (XCSoarInterface::SettingsComputer().UTCOffset > 12 * 3600)
    XCSoarInterface::SetSettingsComputer().UTCOffset -= 24 * 3600;

  Get(szProfileBlockSTF,
      XCSoarInterface::SetSettingsComputer().EnableBlockSTF);
  Get(szProfileAutoZoom,
      XCSoarInterface::SetSettingsMap().AutoZoom);
  Get(szProfileMenuTimeout,
      XCSoarInterface::MenuTimeoutMax);
  Get(szProfileLockSettingsInFlight,
      XCSoarInterface::LockSettingsInFlight);
  Get(szProfileLoggerShort,
      XCSoarInterface::SetSettingsComputer().LoggerShortName);
  Get(szProfileEnableFLARMMap,
      XCSoarInterface::SetSettingsMap().EnableFLARMMap);
  Get(szProfileEnableFLARMGauge,
      XCSoarInterface::SetSettingsMap().EnableFLARMGauge);
  Get(szProfileTerrainContrast,
      XCSoarInterface::SetSettingsMap().TerrainContrast);
  Get(szProfileTerrainBrightness,
      XCSoarInterface::SetSettingsMap().TerrainBrightness);
  Get(szProfileTerrainRamp,
      XCSoarInterface::SetSettingsMap().TerrainRamp);

  Get(szProfileGliderScreenPosition,
      XCSoarInterface::SetSettingsMap().GliderScreenPosition);
  Get(szProfileBallastSecsToEmpty,
      XCSoarInterface::SetSettingsComputer().BallastSecsToEmpty);
  Get(szProfileSetSystemTimeFromGPS,
      XCSoarInterface::SetSettingsMap().SetSystemTimeFromGPS);
  Get(szProfileUseCustomFonts,
      Appearance.UseCustomFonts);
  Get(szProfileVoiceClimbRate,
      XCSoarInterface::SetSettingsComputer().EnableVoiceClimbRate);
  Get(szProfileVoiceTerrain,
      XCSoarInterface::SetSettingsComputer().EnableVoiceTerrain);
  Get(szProfileVoiceWaypointDistance,
      XCSoarInterface::SetSettingsComputer().EnableVoiceWaypointDistance);
  Get(szProfileVoiceTaskAltitudeDifference,
      XCSoarInterface::SetSettingsComputer().EnableVoiceTaskAltitudeDifference);
  Get(szProfileVoiceMacCready,
      XCSoarInterface::SetSettingsComputer().EnableVoiceMacCready);
  Get(szProfileVoiceNewWaypoint,
      XCSoarInterface::SetSettingsComputer().EnableVoiceNewWaypoint);
  Get(szProfileVoiceInSector,
      XCSoarInterface::SetSettingsComputer().EnableVoiceInSector);
  Get(szProfileVoiceAirspace,
      XCSoarInterface::SetSettingsComputer().EnableVoiceAirspace);
  Get(szProfileEnableNavBaroAltitude,
      XCSoarInterface::SetSettingsComputer().EnableNavBaroAltitude);
  Get(szProfileLoggerTimeStepCruise,
      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCruise);
  Get(szProfileLoggerTimeStepCircling,
      XCSoarInterface::SetSettingsComputer().LoggerTimeStepCircling);
  Get(szProfileAbortSafetyUseCurrent,
      XCSoarInterface::SetSettingsComputer().safety_mc_use_current);

  Temp = iround(XCSoarInterface::SettingsComputer().safety_mc * 10);
  Get(szProfileSafetyMacCready, Temp);
  XCSoarInterface::SetSettingsComputer().safety_mc = Temp / 10.0;

  Get(szProfileUserLevel, XCSoarInterface::UserLevel);

  Temp = iround(XCSoarInterface::SettingsComputer().risk_gamma * 10);
  Get(szProfileRiskGamma, Temp);
  XCSoarInterface::SetSettingsComputer().risk_gamma = Temp / 10.0;

  Temp = (CompassAppearance_t)apCompassAltA;
  Get(szProfileWindArrowStyle, Temp);
  XCSoarInterface::SetSettingsMap().WindArrowStyle = Temp;

  Get(szProfileDisableAutoLogger,
      XCSoarInterface::SetSettingsComputer().DisableAutoLogger);
}

void
Profile::SetSoundSettings()
{
  Set(szProfileSoundVolume,
      XCSoarInterface::SettingsComputer().SoundVolume);
  Set(szProfileSoundDeadband,
      XCSoarInterface::SettingsComputer().SoundDeadband);
  Set(szProfileSoundAudioVario,
      XCSoarInterface::SettingsComputer().EnableSoundVario);
  Set(szProfileSoundTask,
      XCSoarInterface::SettingsComputer().EnableSoundTask);
  Set(szProfileSoundModes,
      XCSoarInterface::SettingsComputer().EnableSoundModes);
}

void
Profile::GetWind()
{
  LogStartUp(_T("Load wind from registry"));

  /* JMW incomplete
  DWORD Temp;
  Temp=0;
  Get(szProfileWindSpeed,&Temp);
  Calculated().WindSpeed = Temp;
  Temp=0;
  Get(szProfileWindBearing,&Temp);
  Calculated().WindBearing = Temp;
  */
}

void
Profile::SetWind()
{
  int Temp;
  Temp = iround(XCSoarInterface::Basic().wind.norm);
  Set(szProfileWindSpeed, Temp);
  Temp = iround(XCSoarInterface::Basic().wind.bearing.value_degrees());
  Set(szProfileWindBearing, Temp);
  //TODO  SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing);
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

  Set(Name, TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));

  if (!Get(Name, Buffer, sizeof(Buffer) / sizeof(TCHAR)))
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

int
Profile::GetAirspaceMode(int i)
{
  int Temp = 3; // display + warnings
  Get(szProfileAirspaceMode[i], Temp);
  return Temp;
}

void
Profile::SetAirspaceMode(int i)
{
  int val = XCSoarInterface::SettingsComputer().iAirspaceMode[i];
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

const TCHAR *
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
