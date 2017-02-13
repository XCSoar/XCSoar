/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_UTILS_PROFILE_HPP
#define XCSOAR_UTILS_PROFILE_HPP

namespace ProfileKeys {

extern const char ShowMenuButton[];
extern const char UIScale[];
extern const char CustomDPI[];
extern const char Password[];
extern const char SpeedUnitsValue[];
extern const char DistanceUnitsValue[];
extern const char AltitudeUnitsValue[];
extern const char TemperatureUnitsValue[];
extern const char LiftUnitsValue[];
extern const char PressureUnitsValue[];
extern const char WingLoadingUnitValue[];
extern const char MassUnitValue[];
extern const char TaskSpeedUnitsValue[];
extern const char DisplayUpValue[];
extern const char OrientationCruise[];
extern const char OrientationCircling[];
extern const char MapShiftBias[];
extern const char DisplayText[];
extern const char WaypointArrivalHeightDisplay[];
extern const char WaypointLabelSelection[];
extern const char WaypointLabelStyle[];
extern const char WeatherStations[];
extern const char SafetyAltitudeArrival[];
extern const char SafetyAltitudeTerrain[];
extern const char SafteySpeed[];
extern const char DryMass[];
extern const char PolarID[];
extern const char Polar[];
extern const char PolarName[];
extern const char PolarDegradation[];
extern const char AutoBugs[];
extern const char WaypointFile[];
extern const char AdditionalWaypointFile[];
extern const char WatchedWaypointFile[];
extern const char AirspaceFile[];
extern const char AdditionalAirspaceFile[];
extern const char AirfieldFile[];
extern const char PolarFile[];
extern const char LanguageFile[];
extern const char InputFile[];
extern const char AltMode[];
extern const char ClipAlt[];
extern const char AltMargin[];
extern const char SnailTrail[];
extern const char DrawTopography[];
extern const char DrawTerrain[];
extern const char SlopeShading[];
extern const char SlopeShadingType[];
extern const char TerrainContours[];
extern const char FinalGlideTerrain[];
extern const char AutoWind[];
extern const char ExternalWind[];
extern const char AirspaceWarning[];
extern const char AirspaceWarningDialog[];
extern const char AirspaceBlackOutline[];
extern const char AirspaceTransparency[];
extern const char AirspaceFillMode[];
extern const char AirspaceLabelSelection[];
extern const char WarningTime[];
extern const char RepetitiveSound[];
extern const char AcknowledgementTime[];
extern const char CircleZoom[];
extern const char MaxAutoZoomDistance[];
extern const char HomeWaypoint[];
extern const char HomeLocation[];
extern const char TeamcodeRefWaypoint[];
extern const char PilotName[];
extern const char AircraftType[];
extern const char AircraftReg[];
extern const char CompetitionId[];
extern const char LoggerID[];
extern const char LoggerShort[];
extern const char SoundVolume[];
extern const char SoundDeadband[];
extern const char SoundAudioVario[];
extern const char SoundTask[];
extern const char SoundModes[];
extern const char NettoSpeed[];
extern const char AverEffTime[]; // VENTA6
extern const char VarioGauge[];
extern const char AppIndLandable[];
extern const char AppUseSWLandablesRendering[];
extern const char AppLandableRenderingScale[];
extern const char AppScaleRunwayLength[];
extern const char AppInverseInfoBox[];
extern const char AppInfoBoxColors[];
extern const char AppGaugeVarioSpeedToFly[];
extern const char AppGaugeVarioAvgText[];
extern const char AppGaugeVarioMc[];
extern const char AppGaugeVarioBugs[];
extern const char AppGaugeVarioBallast[];
extern const char AppGaugeVarioGross[];
extern const char AppStatusMessageAlignment[];
extern const char AppTextInputStyle[];
extern const char HapticFeedback[];
extern const char AppDialogTabStyle[];
extern const char AppDialogStyle[];
extern const char AppInfoBoxBorder[];
extern const char AppAveNeedle[];
extern const char AutoAdvance[];
extern const char UTCOffset[];
extern const char UTCOffsetSigned[];
extern const char BlockSTF[];
extern const char AutoZoom[];
extern const char MenuTimeout[];
extern const char TerrainContrast[];
extern const char TerrainBrightness[];
extern const char TerrainRamp[];
extern const char EnableFLARMMap[];
extern const char EnableFLARMGauge[];
extern const char AutoCloseFlarmDialog[];
extern const char EnableTAGauge[];
extern const char EnableThermalProfile[];
extern const char TrailDrift[];
extern const char DetourCostMarker[];
extern const char DisplayTrackBearing[];
extern const char GliderScreenPosition[];
extern const char SetSystemTimeFromGPS[];

extern const char FinishMinHeight[];
extern const char FinishHeightRef[];
extern const char StartMaxHeight[];
extern const char StartMaxHeightMargin[];
extern const char StartMaxSpeed[];
extern const char StartMaxSpeedMargin[];
extern const char StartHeightRef[];
extern const char StartType[];
extern const char StartRadius[];
extern const char TurnpointType[];
extern const char TurnpointRadius[];
extern const char FinishType[];
extern const char FinishRadius[];
extern const char TaskType[];
extern const char AATMinTime[];
extern const char AATTimeMargin[];

extern const char EnableNavBaroAltitude[];

extern const char LoggerTimeStepCruise[];
extern const char LoggerTimeStepCircling[];

extern const char SafetyMacCready[];
extern const char AbortTaskMode[];
extern const char AutoMcMode[];
extern const char AutoMc[];
extern const char EnableExternalTriggerCruise[];
extern const char OLCRules[];
extern const char PredictContest[];
extern const char Handicap[];
extern const char SnailWidthScale[];
extern const char SnailType[];
extern const char LatLonUnits[];
extern const char UserLevel[];
extern const char RiskGamma[];
extern const char PredictWindDrift[];
extern const char WindArrowStyle[];
extern const char EnableFinalGlideBarMC0[];
extern const char FinalGlideBarDisplayMode[];
extern const char EnableVarioBar[];
extern const char ShowFAITriangleAreas[];
extern const char FAITriangleThreshold[];
extern const char AutoLogger[];
extern const char DisableAutoLogger[];
extern const char EnableFlightLogger[];
extern const char EnableNMEALogger[];
extern const char MapFile[];
extern const char BallastSecsToEmpty[];
extern const char AccelerometerZero[];
extern const char DialogFont[];
extern const char FontInfoWindowFont[];
extern const char FontTitleWindowFont[];
extern const char FontMapWindowFont[];
extern const char FontMapWindowBoldFont[];
extern const char FontCDIWindowFont[];
extern const char FontMapLabelFont[];
extern const char FontMapLabelImportantFont[];
extern const char FontStatisticsFont[];
extern const char FontBugsBallastFont[];
extern const char FontAirspacePressFont[];
extern const char FontAirspaceColourDlgFont[];
extern const char FontTeamCodeFont[];

extern const char UseFinalGlideDisplayMode[];

extern const char InfoBoxGeometry[];

extern const char FlarmSideData[];
extern const char FlarmAutoZoom[];
extern const char FlarmNorthUp[];

extern const char IgnoreNMEAChecksum[];
extern const char MapOrientation[];

extern const char ClimbMapScale[];
extern const char CruiseMapScale[];

extern const char RoutePlannerMode[];
extern const char RoutePlannerAllowClimb[];
extern const char RoutePlannerUseCeiling[];
extern const char TurningReach[];
extern const char ReachPolarMode[];

extern const char AircraftSymbol[];

extern const char FlarmLocation[];

extern const char TrackingInterval[];
extern const char TrackingVehicleType[];
extern const char TrackingVehicleName[];
extern const char SkyLinesTrackingEnabled[];
extern const char SkyLinesRoaming[];
extern const char SkyLinesTrackingInterval[];
extern const char SkyLinesTrafficEnabled[];
extern const char SkyLinesNearTrafficEnabled[];
extern const char SkyLinesTrackingKey[];

extern const char CloudEnabled[];
extern const char CloudShowThermals[];
extern const char CloudKey[];

extern const char LiveTrack24Enabled[];
extern const char LiveTrack24Server[];
extern const char LiveTrack24Username[];
extern const char LiveTrack24Password[];

extern const char PCMetUsername[];
extern const char PCMetPassword[];
extern const char PCMetFtpUsername[];
extern const char PCMetFtpPassword[];

extern const char EnableLocationMapItem[];
extern const char EnableArrivalAltitudeMapItem[];

extern const char VarioMinFrequency[];
extern const char VarioZeroFrequency[];
extern const char VarioMaxFrequency[];
extern const char VarioMinPeriod[];
extern const char VarioMaxPeriod[];
extern const char VarioDeadBandEnabled[];
extern const char VarioDeadBandMin[];
extern const char VarioDeadBandMax[];

extern const char PagesDistinctZoom[];

extern const char WaveAssistant[];

extern const char MasterAudioVolume[];

}

#endif
