/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Profile/ProfileKeys.hpp"

const TCHAR *szProfileColour[] = {
  _T("Colour0"),
  _T("Colour1"),
  _T("Colour2"),
  _T("Colour3"),
  _T("Colour4"),
  _T("Colour5"),
  _T("Colour6"),
  _T("Colour7"),
  _T("Colour8"),
  _T("Colour9"),
  _T("Colour10"),
  _T("Colour11"),
  _T("Colour12"),
  _T("Colour13"),
  _T("Colour14"),
  _T("Colour15"),
}; // pL

const TCHAR *szProfileBrush[] = {
  _T("Brush0"),
  _T("Brush1"),
  _T("Brush2"),
  _T("Brush3"),
  _T("Brush4"),
  _T("Brush5"),
  _T("Brush6"),
  _T("Brush7"),
  _T("Brush8"),
  _T("Brush9"),
  _T("Brush10"),
  _T("Brush11"),
  _T("Brush12"),
  _T("Brush13"),
  _T("Brush14"),
  _T("Brush15"),
}; // pL

const TCHAR *szProfileAirspaceMode[] = {
  _T("AirspaceMode0"),
  _T("AirspaceMode1"),
  _T("AirspaceMode2"),
  _T("AirspaceMode3"),
  _T("AirspaceMode4"),
  _T("AirspaceMode5"),
  _T("AirspaceMode6"),
  _T("AirspaceMode7"),
  _T("AirspaceMode8"),
  _T("AirspaceMode9"),
  _T("AirspaceMode10"),
  _T("AirspaceMode11"),
  _T("AirspaceMode12"),
  _T("AirspaceMode13"),
  _T("AirspaceMode14"),
  _T("AirspaceMode15"),
}; // pL

const TCHAR szProfileAirspaceWarning[] = _T("AirspaceWarn");
const TCHAR szProfileAirspaceBlackOutline[] = _T("AirspaceBlackOutline");
const TCHAR szProfileAirspaceTransparency[] = _T("AirspaceTransparency");
const TCHAR szProfileAirspaceFillMode[] = _T("AirspaceFillMode");
const TCHAR szProfileAltMargin[] = _T("AltMargin");
const TCHAR szProfileAltMode[] = _T("AltitudeMode");
const TCHAR szProfileAltitudeUnitsValue[] = _T("Altitude");
const TCHAR szProfileTemperatureUnitsValue[] = _T("Temperature");
const TCHAR szProfileCircleZoom[] = _T("CircleZoom");
const TCHAR szProfileMaxAutoZoomDistance[] = _T("MaxAutoZoomDistance");
const TCHAR szProfileClipAlt[] = _T("ClipAlt");
const TCHAR szProfileDisplayText[] = _T("DisplayText");
const TCHAR szProfileWaypointArrivalHeightDisplay[] = _T("WaypointArrivalHeightDisplay");
const TCHAR szProfileWaypointLabelSelection[] = _T("WayPointLabelSelection");
const TCHAR szProfileWaypointLabelStyle[] = _T("WayPointLabelStyle");
const TCHAR szProfileDisplayUpValue[] = _T("DisplayUp");
const TCHAR szProfileOrientationCruise[] = _T("OrientationCruise");
const TCHAR szProfileOrientationCircling[] = _T("OrientationCircling");
const TCHAR szProfileMapShiftBias[] = _T("MapShiftBias");
const TCHAR szProfileDistanceUnitsValue[] = _T("Distance");
const TCHAR szProfileDrawTerrain[] = _T("DrawTerrain");
const TCHAR szProfileSlopeShading[] = _T("SlopeShading");
const TCHAR szProfileSlopeShadingType[] = _T("SlopeShadingType");
const TCHAR szProfileDrawTopography[] = _T("DrawTopology");
const TCHAR szProfileFinalGlideTerrain[] = _T("FinalGlideTerrain");
const TCHAR szProfileAutoWind[] = _T("AutoWind");
const TCHAR szProfileExternalWind[] = _T("ExternalWind");
const TCHAR szProfileHomeWaypoint[] = _T("HomeWaypoint");
const TCHAR szProfileHomeLocation[] = _T("HomeLocation");
const TCHAR szProfileLiftUnitsValue[] = _T("Lift");
const TCHAR szProfileLatLonUnits[] = _T("LatLonUnits");
const TCHAR szProfilePolarID[] = _T("Polar");
const TCHAR szProfilePolar[] = _T("PolarInformation");
const TCHAR szProfilePolarName[] = _T("PolarName");
const TCHAR szProfileSafetyAltitudeArrival[] = _T("SafetyAltitudeArrival");
const TCHAR szProfileSafetyAltitudeTerrain[] = _T("SafetyAltitudeTerrain");
const TCHAR szProfileSafteySpeed[] = _T("SafteySpeed");
const TCHAR szProfileSnailTrail[] = _T("SnailTrail");
const TCHAR szProfileTrailDrift[] = _T("TrailDrift");
const TCHAR szProfileDetourCostMarker[] = _T("DetourCostMarker");
const TCHAR szProfileDisplayTrackBearing[] = _T("DisplayTrackBearing");
const TCHAR szProfileUnitsPresetName[] = _T("UnitsPresetName");
const TCHAR szProfileSpeedUnitsValue[] = _T("Speed");
const TCHAR szProfileTaskSpeedUnitsValue[] = _T("TaskSpeed");
const TCHAR szProfileWarningTime[] = _T("WarnTime");
const TCHAR szProfileAcknowledgementTime[] = _T("AcknowledgementTime");
const TCHAR szProfileAirfieldFile[] = _T("AirfieldFile"); // pL
const TCHAR szProfileAirspaceFile[] = _T("AirspaceFile"); // pL
const TCHAR szProfileAdditionalAirspaceFile[] = _T("AdditionalAirspaceFile"); // pL
const TCHAR szProfilePolarFile[] = _T("PolarFile"); // pL
const TCHAR szProfileTerrainFile[] = _T("TerrainFile"); // pL
const TCHAR szProfileTopographyFile[] = _T("TopologyFile");
const TCHAR szProfileWaypointFile[] = _T("WPFile"); // pL
const TCHAR szProfileAdditionalWaypointFile[] = _T("AdditionalWPFile"); // pL
const TCHAR szProfileWatchedWaypointFile[] = _T("WatchedWPFile"); // pL
const TCHAR szProfileLanguageFile[] = _T("LanguageFile"); // pL
const TCHAR szProfileStatusFile[] = _T("StatusFile"); // pL
const TCHAR szProfileInputFile[] = _T("InputFile"); // pL
const TCHAR szProfilePilotName[] = _T("PilotName");
const TCHAR szProfileAircraftType[] = _T("AircraftType");
const TCHAR szProfileAircraftReg[] = _T("AircraftReg");
const TCHAR szProfileCompetitionId[] = _T("AircraftRego");
const TCHAR szProfileLoggerID[] = _T("LoggerID");
const TCHAR szProfileLoggerShort[] = _T("LoggerShortName");
const TCHAR szProfileSoundVolume[] = _T("SoundVolume");
const TCHAR szProfileSoundDeadband[] = _T("SoundDeadband");
const TCHAR szProfileSoundAudioVario[] = _T("AudioVario");
const TCHAR szProfileSoundTask[] = _T("SoundTask");
const TCHAR szProfileSoundModes[] = _T("SoundModes");
const TCHAR szProfileNettoSpeed[] = _T("NettoSpeed");
const TCHAR szProfileAccelerometerZero[] = _T("AccelerometerZero");

const TCHAR szProfileAutoBlank[] = _T("AutoBlank");
const TCHAR szProfileAverEffTime[] = _T("AverEffTime");
const TCHAR szProfileVarioGauge[] = _T("VarioGauge");

const TCHAR szProfileDebounceTimeout[] = _T("DebounceTimeout");

const TCHAR szProfileAppIndLandable[] = _T("AppIndLandable");
const TCHAR szProfileAppUseSWLandablesRendering[] = _T("AppUseSWLandablesRendering");
const TCHAR szProfileAppLandableRenderingScale[] = _T("AppLandableRenderingScale");
const TCHAR szProfileAppScaleRunwayLength[] = _T("AppScaleRunwayLength");
const TCHAR szProfileAppInverseInfoBox[] = _T("AppInverseInfoBox");
const TCHAR szProfileAppGaugeVarioSpeedToFly[] = _T("AppGaugeVarioSpeedToFly");
const TCHAR szProfileAppGaugeVarioAvgText[] = _T("AppGaugeVarioAvgText");
const TCHAR szProfileAppGaugeVarioMc[] = _T("AppGaugeVarioMc");
const TCHAR szProfileAppGaugeVarioBugs[] = _T("AppGaugeVarioBugs");
const TCHAR szProfileAppGaugeVarioBallast[] = _T("AppGaugeVarioBallast");
const TCHAR szProfileAppGaugeVarioGross[] = _T("AppGaugeVarioGross");
const TCHAR szProfileAppStatusMessageAlignment[] = _T("AppStatusMessageAlignment");
const TCHAR szProfileAppTextInputStyle[] = _T("AppTextInputStyle");
const TCHAR szProfileAppDialogTabStyle[] = _T("AppDialogTabStyle");
const TCHAR szProfileAppDialogStyle[] = _T("AppDialogStyle");
const TCHAR szProfileAppInfoBoxColors[] = _T("AppInfoBoxColors");
const TCHAR szProfileTeamcodeRefWaypoint[] = _T("TeamcodeRefWaypoint");
const TCHAR szProfileAppInfoBoxBorder[] = _T("AppInfoBoxBorder");

const TCHAR szProfileAppInfoBoxModel[] = _T("AppInfoBoxModel"); // VENTA-ADDON MODEL CONFIG

const TCHAR szProfileAppAveNeedle[] = _T("AppAveNeedle");

const TCHAR szProfileAutoAdvance[] = _T("AutoAdvance");
const TCHAR szProfileUTCOffset[] = _T("UTCOffset");
const TCHAR szProfileBlockSTF[] = _T("BlockSpeedToFly");
const TCHAR szProfileAutoZoom[] = _T("AutoZoom");
const TCHAR szProfileMenuTimeout[] = _T("MenuTimeout");
const TCHAR szProfileTerrainContrast[] = _T("TerrainContrast");
const TCHAR szProfileTerrainBrightness[] = _T("TerrainBrightness");
const TCHAR szProfileTerrainRamp[] = _T("TerrainRamp");
const TCHAR szProfileEnableFLARMMap[] = _T("EnableFLARMDisplay");
const TCHAR szProfileEnableFLARMGauge[] = _T("EnableFLARMGauge");
const TCHAR szProfileAutoCloseFlarmDialog[] = _T("AutoCloseFlarmDialog");
const TCHAR szProfileEnableTAGauge[] = _T("EnableTAGauge");
const TCHAR szProfileEnableThermalProfile[] = _T("EnableThermalProfile");
const TCHAR szProfileFLARMGaugeBearing[] = _T("FLARMGaugeBearing");
const TCHAR szProfileGliderScreenPosition[] = _T("GliderScreenPosition");
const TCHAR szProfileSetSystemTimeFromGPS[] = _T("SetSystemTimeFromGPS");

const TCHAR szProfileVoiceClimbRate[] = _T("VoiceClimbRate");
const TCHAR szProfileVoiceTerrain[] = _T("VoiceTerrain");
const TCHAR szProfileVoiceWaypointDistance[] = _T("VoiceWaypointDistance");
const TCHAR szProfileVoiceTaskAltitudeDifference[] = _T("VoiceTaskAltitudeDifference");
const TCHAR szProfileVoiceMacCready[] = _T("VoiceMacCready");
const TCHAR szProfileVoiceNewWaypoint[] = _T("VoiceNewWaypoint");
const TCHAR szProfileVoiceInSector[] = _T("VoiceInSector");
const TCHAR szProfileVoiceAirspace[] = _T("VoiceAirspace");

const TCHAR szProfileFinishMinHeight[] = _T("FinishMinHeight");
const TCHAR szProfileStartMaxHeight[] = _T("StartMaxHeight");
const TCHAR szProfileStartMaxSpeed[] = _T("StartMaxSpeed");
const TCHAR szProfileStartMaxHeightMargin[] = _T("StartMaxHeightMargin");
const TCHAR szProfileStartMaxSpeedMargin[] = _T("StartMaxSpeedMargin");
const TCHAR szProfileStartHeightRef[] = _T("StartHeightRef");
const TCHAR szProfileStartType[] = _T("StartType");
const TCHAR szProfileStartRadius[] = _T("StartRadius");
const TCHAR szProfileTurnpointType[] = _T("TurnpointType");
const TCHAR szProfileTurnpointRadius[] = _T("TurnpointRadius");
const TCHAR szProfileFinishType[] = _T("FinishType");
const TCHAR szProfileFinishRadius[] = _T("FinishRadius");
const TCHAR szProfileTaskType[] = _T("TaskType");
const TCHAR szProfileAATMinTime[] = _T("AATMinTime");
const TCHAR szProfileAATTimeMargin[] = _T("AATTimeMargin");

const TCHAR szProfileEnableNavBaroAltitude[] = _T("EnableNavBaroAltitude");

const TCHAR szProfileLoggerTimeStepCruise[] = _T("LoggerTimeStepCruise");
const TCHAR szProfileLoggerTimeStepCircling[] = _T("LoggerTimeStepCircling");

const TCHAR szProfileSafetyMacCready[] = _T("SafetyMacCready");
const TCHAR szProfileAbortTaskMode[] = _T("AbortTaskMode");
const TCHAR szProfileAutoMcMode[] = _T("AutoMcMode");
const TCHAR szProfileAutoMc[] = _T("AutoMc");
const TCHAR szProfileEnableExternalTriggerCruise[] = _T("EnableExternalTriggerCruise");
const TCHAR szProfileOLCRules[] = _T("OLCRules");
const TCHAR szProfileHandicap[] = _T("Handicap");
const TCHAR szProfileSnailWidthScale[] = _T("SnailWidthScale");
const TCHAR szProfileSnailType[] = _T("SnailType");
const TCHAR szProfileUserLevel[] = _T("UserLevel");
const TCHAR szProfileRiskGamma[] = _T("RiskGamma");
const TCHAR szProfileWindArrowStyle[] = _T("WindArrowStyle");
const TCHAR szProfileDisableAutoLogger[] = _T("DisableAutoLogger");
const TCHAR szProfileMapFile[] = _T("MapFile"); // pL
const TCHAR szProfileBallastSecsToEmpty[] = _T("BallastSecsToEmpty");
const TCHAR szProfileUseCustomFonts[] = _T("UseCustomFonts");
const TCHAR szProfileFontInfoWindowFont[] = _T("InfoWindowFont");
const TCHAR szProfileFontTitleWindowFont[] = _T("TitleWindowFont");
const TCHAR szProfileFontMapWindowFont[] = _T("MapWindowFont");
const TCHAR szProfileFontTitleSmallWindowFont[] = _T("TeamCodeFont");
const TCHAR szProfileFontMapWindowBoldFont[] = _T("MapWindowBoldFont");
const TCHAR szProfileFontCDIWindowFont[] = _T("CDIWindowFont");
const TCHAR szProfileFontMapLabelFont[] = _T("MapLabelFont");
const TCHAR szProfileFontMapLabelImportantFont[] = _T("MapLabelImportantFont");
const TCHAR szProfileFontStatisticsFont[] = _T("StatisticsFont");
const TCHAR szProfileFontBugsBallastFont[] = _T("BugsBallastFont");
const TCHAR szProfileFontAirspacePressFont[] = _T("AirspacePressFont");
const TCHAR szProfileFontAirspaceColourDlgFont[] = _T("AirspaceColourDlgFont");
const TCHAR szProfileFontTeamCodeFont[] = _T("TeamCodeFont");

const TCHAR szProfileInfoBoxGeometry[] = _T("InfoBoxGeometry");

const TCHAR szProfileFlarmSideData[] = _T("FlarmRadarSideData");
const TCHAR szProfileFlarmAutoZoom[] = _T("FlarmRadarAutoZoom");
const TCHAR szProfileFlarmNorthUp[] = _T("FlarmRadarNorthUp");

const TCHAR szProfileIgnoreNMEAChecksum[] = _T("IgnoreNMEAChecksum");
const TCHAR szProfileDisplayOrientation[] = _T("DisplayOrientation");

const TCHAR szProfileClimbMapScale[] = _T("ClimbMapScale");
const TCHAR szProfileCruiseMapScale[] = _T("CruiseMapScale");

const TCHAR szProfileRoutePlannerMode[] = _T("RoutePlannerMode");
const TCHAR szProfileRoutePlannerAllowClimb[] = _T("RoutePlannerAllowClimb");
const TCHAR szProfileRoutePlannerUseCeiling[] = _T("RoutePlannerUseCeiling");
const TCHAR szProfileTurningReach[] = _T("TurningReach");
const TCHAR szProfileReachPolarMode[] = _T("ReachPolarMode");

const TCHAR szProfileAircraftSymbol[] = _T("AircraftSymbol");
