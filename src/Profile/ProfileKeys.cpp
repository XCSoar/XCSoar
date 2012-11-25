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

#include "Profile/ProfileKeys.hpp"

namespace ProfileKeys {

const TCHAR AirspaceWarning[] = _T("AirspaceWarn");
const TCHAR AirspaceBlackOutline[] = _T("AirspaceBlackOutline");
const TCHAR AirspaceTransparency[] = _T("AirspaceTransparency");
const TCHAR AirspaceFillMode[] = _T("AirspaceFillMode");
const TCHAR AltMargin[] = _T("AltMargin");
const TCHAR AltMode[] = _T("AltitudeMode");
const TCHAR AltitudeUnitsValue[] = _T("AltitudeUnit");
const TCHAR TemperatureUnitsValue[] = _T("TemperatureUnit");
const TCHAR CircleZoom[] = _T("CircleZoom");
const TCHAR MaxAutoZoomDistance[] = _T("MaxAutoZoomDistance");
const TCHAR ClipAlt[] = _T("ClipAlt");
const TCHAR DisplayText[] = _T("DisplayText");
const TCHAR WaypointArrivalHeightDisplay[] = _T("WaypointArrivalHeightDisplay");
const TCHAR WaypointLabelSelection[] = _T("WayPointLabelSelection");
const TCHAR WaypointLabelStyle[] = _T("WayPointLabelStyle");
const TCHAR WeatherStations[] = _T("WeatherStations");
const TCHAR DisplayUpValue[] = _T("DisplayUp");
const TCHAR OrientationCruise[] = _T("OrientationCruise");
const TCHAR OrientationCircling[] = _T("OrientationCircling");
const TCHAR MapShiftBias[] = _T("MapShiftBias");
const TCHAR DistanceUnitsValue[] = _T("DistanceUnit");
const TCHAR DrawTerrain[] = _T("DrawTerrain");
const TCHAR SlopeShading[] = _T("SlopeShading");
const TCHAR SlopeShadingType[] = _T("SlopeShadingType");
const TCHAR DrawTopography[] = _T("DrawTopology");
const TCHAR FinalGlideTerrain[] = _T("FinalGlideTerrain");
const TCHAR AutoWind[] = _T("AutoWind");
const TCHAR ExternalWind[] = _T("ExternalWind");
const TCHAR HomeWaypoint[] = _T("HomeWaypoint");
const TCHAR HomeLocation[] = _T("HomeLocation");
const TCHAR LiftUnitsValue[] = _T("LiftUnit");
const TCHAR PressureUnitsValue[] = _T("Pressure");
const TCHAR LatLonUnits[] = _T("LatLonUnits");
const TCHAR PolarID[] = _T("Polar");
const TCHAR Polar[] = _T("PolarInformation");
const TCHAR PolarName[] = _T("PolarName");
const TCHAR PolarDegradation[] = _T("PolarDegradation");
const TCHAR SafetyAltitudeArrival[] = _T("SafetyAltitudeArrival");
const TCHAR SafetyAltitudeTerrain[] = _T("SafetyAltitudeTerrain");
const TCHAR SafteySpeed[] = _T("SafteySpeed");
const TCHAR DryMass[] = _T("DryMass");
const TCHAR SnailTrail[] = _T("SnailTrail");
const TCHAR TrailDrift[] = _T("TrailDrift");
const TCHAR DetourCostMarker[] = _T("DetourCostMarker");
const TCHAR DisplayTrackBearing[] = _T("DisplayTrackBearing");
const TCHAR SpeedUnitsValue[] = _T("SpeedUnit");
const TCHAR TaskSpeedUnitsValue[] = _T("TaskSpeedUnit");
const TCHAR WarningTime[] = _T("WarnTime");
const TCHAR AcknowledgementTime[] = _T("AcknowledgementTime");
const TCHAR AirfieldFile[] = _T("AirfieldFile"); // pL
const TCHAR AirspaceFile[] = _T("AirspaceFile"); // pL
const TCHAR AdditionalAirspaceFile[] = _T("AdditionalAirspaceFile"); // pL
const TCHAR PolarFile[] = _T("PolarFile"); // pL
const TCHAR WaypointFile[] = _T("WPFile"); // pL
const TCHAR AdditionalWaypointFile[] = _T("AdditionalWPFile"); // pL
const TCHAR WatchedWaypointFile[] = _T("WatchedWPFile"); // pL
const TCHAR LanguageFile[] = _T("LanguageFile"); // pL
const TCHAR StatusFile[] = _T("StatusFile"); // pL
const TCHAR InputFile[] = _T("InputFile"); // pL
const TCHAR PilotName[] = _T("PilotName");
const TCHAR AircraftType[] = _T("AircraftType");
const TCHAR AircraftReg[] = _T("AircraftReg");
const TCHAR CompetitionId[] = _T("AircraftRego");
const TCHAR LoggerID[] = _T("LoggerID");
const TCHAR LoggerShort[] = _T("LoggerShortName");
const TCHAR SoundVolume[] = _T("SoundVolume");
const TCHAR SoundDeadband[] = _T("SoundDeadband");
const TCHAR SoundAudioVario[] = _T("AudioVario2");
const TCHAR SoundTask[] = _T("SoundTask");
const TCHAR SoundModes[] = _T("SoundModes");
const TCHAR NettoSpeed[] = _T("NettoSpeed");
const TCHAR AccelerometerZero[] = _T("AccelerometerZero");

const TCHAR AutoBlank[] = _T("AutoBlank");
const TCHAR AverEffTime[] = _T("AverEffTime");
const TCHAR VarioGauge[] = _T("VarioGauge");

const TCHAR AppIndLandable[] = _T("AppIndLandable");
const TCHAR AppUseSWLandablesRendering[] = _T("AppUseSWLandablesRendering");
const TCHAR AppLandableRenderingScale[] = _T("AppLandableRenderingScale");
const TCHAR AppScaleRunwayLength[] = _T("AppScaleRunwayLength");
const TCHAR AppInverseInfoBox[] = _T("AppInverseInfoBox");
const TCHAR AppGaugeVarioSpeedToFly[] = _T("AppGaugeVarioSpeedToFly");
const TCHAR AppGaugeVarioAvgText[] = _T("AppGaugeVarioAvgText");
const TCHAR AppGaugeVarioMc[] = _T("AppGaugeVarioMc");
const TCHAR AppGaugeVarioBugs[] = _T("AppGaugeVarioBugs");
const TCHAR AppGaugeVarioBallast[] = _T("AppGaugeVarioBallast");
const TCHAR AppGaugeVarioGross[] = _T("AppGaugeVarioGross");
const TCHAR AppStatusMessageAlignment[] = _T("AppStatusMessageAlignment");
const TCHAR AppTextInputStyle[] = _T("AppTextInputStyle");
const TCHAR HapticFeedback[] = _T("HapticFeedback");
const TCHAR AppDialogTabStyle[] = _T("AppDialogTabStyle");
const TCHAR AppDialogStyle[] = _T("AppDialogStyle");
const TCHAR AppInfoBoxColors[] = _T("AppInfoBoxColors");
const TCHAR TeamcodeRefWaypoint[] = _T("TeamcodeRefWaypoint");
const TCHAR AppInfoBoxBorder[] = _T("AppInfoBoxBorder");

const TCHAR AppInfoBoxModel[] = _T("AppInfoBoxModel"); // VENTA-ADDON MODEL CONFIG

const TCHAR AppAveNeedle[] = _T("AppAveNeedle");

const TCHAR AutoAdvance[] = _T("AutoAdvance");
const TCHAR UTCOffset[] = _T("UTCOffset");
const TCHAR UTCOffsetSigned[] = _T("UTCOffsetSigned");
const TCHAR BlockSTF[] = _T("BlockSpeedToFly");
const TCHAR AutoZoom[] = _T("AutoZoom");
const TCHAR MenuTimeout[] = _T("MenuTimeout");
const TCHAR TerrainContrast[] = _T("TerrainContrast");
const TCHAR TerrainBrightness[] = _T("TerrainBrightness");
const TCHAR TerrainRamp[] = _T("TerrainRamp");
const TCHAR EnableFLARMMap[] = _T("EnableFLARMDisplay");
const TCHAR EnableFLARMGauge[] = _T("EnableFLARMGauge");
const TCHAR AutoCloseFlarmDialog[] = _T("AutoCloseFlarmDialog");
const TCHAR EnableTAGauge[] = _T("EnableTAGauge");
const TCHAR EnableThermalProfile[] = _T("EnableThermalProfile");
const TCHAR FLARMGaugeBearing[] = _T("FLARMGaugeBearing");
const TCHAR GliderScreenPosition[] = _T("GliderScreenPosition");
const TCHAR SetSystemTimeFromGPS[] = _T("SetSystemTimeFromGPS");

const TCHAR VoiceClimbRate[] = _T("VoiceClimbRate");
const TCHAR VoiceTerrain[] = _T("VoiceTerrain");
const TCHAR VoiceWaypointDistance[] = _T("VoiceWaypointDistance");
const TCHAR VoiceTaskAltitudeDifference[] = _T("VoiceTaskAltitudeDifference");
const TCHAR VoiceMacCready[] = _T("VoiceMacCready");
const TCHAR VoiceNewWaypoint[] = _T("VoiceNewWaypoint");
const TCHAR VoiceInSector[] = _T("VoiceInSector");
const TCHAR VoiceAirspace[] = _T("VoiceAirspace");

const TCHAR FinishMinHeight[] = _T("FinishMinHeight");
const TCHAR FinishHeightRef[] = _T("FinishHeightRef");
const TCHAR StartMaxHeight[] = _T("StartMaxHeight");
const TCHAR StartMaxSpeed[] = _T("StartMaxSpeed");
const TCHAR StartMaxHeightMargin[] = _T("StartMaxHeightMargin");
const TCHAR StartMaxSpeedMargin[] = _T("StartMaxSpeedMargin");
const TCHAR StartHeightRef[] = _T("StartHeightRef");
const TCHAR StartType[] = _T("StartType");
const TCHAR StartRadius[] = _T("StartRadius");
const TCHAR TurnpointType[] = _T("TurnpointType");
const TCHAR TurnpointRadius[] = _T("TurnpointRadius");
const TCHAR FinishType[] = _T("FinishType");
const TCHAR FinishRadius[] = _T("FinishRadius");
const TCHAR TaskType[] = _T("TaskType");
const TCHAR AATMinTime[] = _T("AATMinTime");
const TCHAR AATTimeMargin[] = _T("AATTimeMargin");

const TCHAR EnableNavBaroAltitude[] = _T("EnableNavBaroAltitude");

const TCHAR LoggerTimeStepCruise[] = _T("LoggerTimeStepCruise");
const TCHAR LoggerTimeStepCircling[] = _T("LoggerTimeStepCircling");

const TCHAR SafetyMacCready[] = _T("SafetyMacCready");
const TCHAR AbortTaskMode[] = _T("AbortTaskMode");
const TCHAR AutoMcMode[] = _T("AutoMcMode");
const TCHAR AutoMc[] = _T("AutoMc");
const TCHAR EnableExternalTriggerCruise[] = _T("EnableExternalTriggerCruise");
const TCHAR OLCRules[] = _T("OLCRules");
const TCHAR PredictContest[] = _T("PredictContest");
const TCHAR Handicap[] = _T("Handicap");
const TCHAR SnailWidthScale[] = _T("SnailWidthScale");
const TCHAR SnailType[] = _T("SnailType");
const TCHAR UserLevel[] = _T("UserLevel");
const TCHAR RiskGamma[] = _T("RiskGamma");
const TCHAR PredictWindDrift[] = _T("PredictWindDrift");
const TCHAR WindArrowStyle[] = _T("WindArrowStyle");
const TCHAR EnableFinalGlideBarMC0[] = _T("EnableFinalGlideBarMC0");
const TCHAR ShowFAITriangleAreas[] = _T("ShowFAITriangleAreas");
const TCHAR AutoLogger[] = _T("AutoLogger");
const TCHAR DisableAutoLogger[] = _T("DisableAutoLogger");
const TCHAR EnableFlightLogger[] = _T("EnableFlightLogger");
const TCHAR EnableNMEALogger[] = _T("EnableNMEALogger");
const TCHAR MapFile[] = _T("MapFile"); // pL
const TCHAR BallastSecsToEmpty[] = _T("BallastSecsToEmpty");
const TCHAR UseCustomFonts[] = _T("UseCustomFonts");
const TCHAR FontInfoWindowFont[] = _T("InfoWindowFont");
const TCHAR FontTitleWindowFont[] = _T("TitleWindowFont");
const TCHAR FontMapWindowFont[] = _T("MapWindowFont");
const TCHAR FontTitleSmallWindowFont[] = _T("TeamCodeFont");
const TCHAR FontMapWindowBoldFont[] = _T("MapWindowBoldFont");
const TCHAR FontCDIWindowFont[] = _T("CDIWindowFont");
const TCHAR FontMapLabelFont[] = _T("MapLabelFont");
const TCHAR FontMapLabelImportantFont[] = _T("MapLabelImportantFont");
const TCHAR FontStatisticsFont[] = _T("StatisticsFont");
const TCHAR FontBugsBallastFont[] = _T("BugsBallastFont");
const TCHAR FontAirspacePressFont[] = _T("AirspacePressFont");
const TCHAR FontAirspaceColourDlgFont[] = _T("AirspaceColourDlgFont");
const TCHAR FontTeamCodeFont[] = _T("TeamCodeFont");

const TCHAR UseFinalGlideDisplayMode[] = _T("UseFinalGlideDisplayMode");
const TCHAR InfoBoxGeometry[] = _T("InfoBoxGeometry");

const TCHAR FlarmSideData[] = _T("FlarmRadarSideData");
const TCHAR FlarmAutoZoom[] = _T("FlarmRadarAutoZoom");
const TCHAR FlarmNorthUp[] = _T("FlarmRadarNorthUp");

const TCHAR IgnoreNMEAChecksum[] = _T("IgnoreNMEAChecksum");
const TCHAR DisplayOrientation[] = _T("DisplayOrientation");

const TCHAR ClimbMapScale[] = _T("ClimbMapScale");
const TCHAR CruiseMapScale[] = _T("CruiseMapScale");

const TCHAR RoutePlannerMode[] = _T("RoutePlannerMode");
const TCHAR RoutePlannerAllowClimb[] = _T("RoutePlannerAllowClimb");
const TCHAR RoutePlannerUseCeiling[] = _T("RoutePlannerUseCeiling");
const TCHAR TurningReach[] = _T("TurningReach");
const TCHAR ReachPolarMode[] = _T("ReachPolarMode");

const TCHAR AircraftSymbol[] = _T("AircraftSymbol");

const TCHAR FlarmLocation[] = _T("FlarmLocation");

const TCHAR TrackingInterval[] = _T("TrackingInterval");
const TCHAR TrackingVehicleType[] = _T("TrackingVehicleType");
const TCHAR SkyLinesTrackingEnabled[] = _T("SkyLinesTrackingEnabled");
const TCHAR SkyLinesTrackingInterval[] = _T("SkyLinesTrackingInterval");
const TCHAR SkyLinesTrafficEnabled[] = _T("SkyLinesTrafficEnabled");
const TCHAR SkyLinesTrackingKey[] = _T("SkyLinesTrackingKey");
const TCHAR LiveTrack24Enabled[] = _T("LiveTrack24Enabled");
const TCHAR LiveTrack24Server[] = _T("LiveTrack24Server");
const TCHAR LiveTrack24Username[] = _T("LiveTrack24Username");
const TCHAR LiveTrack24Password[] = _T("LiveTrack24Password");

const TCHAR EnableLocationMapItem[] = _T("EnableLocationMapItem");
const TCHAR EnableArrivalAltitudeMapItem[] = _T("EnableArrivalAltitudeMapItem");

const TCHAR VarioMinFrequency[] = _T("VarioMinFrequency");
const TCHAR VarioZeroFrequency[] = _T("VarioZeroFrequency");
const TCHAR VarioMaxFrequency[] = _T("VarioMaxFrequency");
const TCHAR VarioMinPeriod[] = _T("VarioMinPeriod");
const TCHAR VarioMaxPeriod[] = _T("VarioMaxPeriod");
const TCHAR VarioDeadBandEnabled[] = _T("VarioDeadBandEnabled");
const TCHAR VarioDeadBandMin[] = _T("VarioDeadBandMin");
const TCHAR VarioDeadBandMax[] = _T("VarioDeadBandMax");

}
