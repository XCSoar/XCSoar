// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace ProfileKeys {

constexpr char FullScreen[] = "FullScreen";
constexpr char UIScale[] = "UIScale";
constexpr char CustomDPI[] = "CustomDPI";
constexpr char Password[] = "Password";
constexpr char AirspaceWarning[] = "AirspaceWarn";
constexpr char AirspaceWarningDialog[] = "AirspaceWarnDialog";
constexpr char AirspaceBlackOutline[] = "AirspaceBlackOutline";
constexpr char AirspaceTransparency[] = "AirspaceTransparency";
constexpr char AirspaceFillMode[] = "AirspaceFillMode";
constexpr char AirspaceLabelSelection[] = "AirspaceLabelSelection";
constexpr char AltMargin[] = "AltMargin";
constexpr char AltMode[] = "AltitudeMode";
constexpr char AltitudeUnitsValue[] = "AltitudeUnit";
constexpr char TemperatureUnitsValue[] = "TemperatureUnit";
constexpr char CircleZoom[] = "CircleZoom";
constexpr char MaxAutoZoomDistance[] = "MaxAutoZoomDistance";
constexpr char ClipAlt[] = "ClipAlt";
constexpr char DisplayText[] = "DisplayText";
constexpr char WaypointArrivalHeightDisplay[] = "WaypointArrivalHeightDisplay";
constexpr char WaypointLabelSelection[] = "WayPointLabelSelection";
constexpr char WaypointLabelStyle[] = "WayPointLabelStyle";
constexpr char WeatherStations[] = "WeatherStations";
constexpr char DisplayUpValue[] = "DisplayUp";
constexpr char OrientationCruise[] = "OrientationCruise";
constexpr char OrientationCircling[] = "OrientationCircling";
constexpr char MapShiftBias[] = "MapShiftBias";
constexpr char DistanceUnitsValue[] = "DistanceUnit";
constexpr char DrawTerrain[] = "DrawTerrain";
constexpr char SlopeShading[] = "SlopeShading";
constexpr char SlopeShadingType[] = "SlopeShadingType";
constexpr char TerrainContours[] = "TerrainContours";
constexpr char DrawTopography[] = "DrawTopology";
constexpr char FinalGlideTerrain[] = "FinalGlideTerrain";
constexpr char AutoWind[] = "AutoWind";
constexpr char ExternalWind[] = "ExternalWind";
constexpr char HomeWaypoint[] = "HomeWaypoint";
constexpr char HomeLocation[] = "HomeLocation";
constexpr char LiftUnitsValue[] = "LiftUnit";
constexpr char PressureUnitsValue[] = "Pressure";
constexpr char WingLoadingUnitValue[] = "WingLoadingUnit";
constexpr char MassUnitValue[] = "MassUnit";
constexpr char RotationUnitValue[] = "RotationUnit";
constexpr char LatLonUnits[] = "LatLonUnits";
constexpr char PolarID[] = "Polar";
constexpr char Polar[] = "PolarInformation";
constexpr char PolarName[] = "PolarName";
constexpr char PolarDegradation[] = "PolarDegradation";
constexpr char AutoBugs[] = "AutoBugs";
constexpr char SafetyAltitudeArrival[] = "SafetyAltitudeArrival";
constexpr char SafetyAltitudeTerrain[] = "SafetyAltitudeTerrain";
constexpr char SafteySpeed[] = "SafteySpeed";
constexpr char DryMass[] = "DryMass";
constexpr char SnailTrail[] = "SnailTrail";
constexpr char TrailDrift[] = "TrailDrift";
constexpr char DetourCostMarker[] = "DetourCostMarker";
constexpr char DisplayTrackBearing[] = "DisplayTrackBearing";
constexpr char SpeedUnitsValue[] = "SpeedUnit";
constexpr char TaskSpeedUnitsValue[] = "TaskSpeedUnit";
constexpr char WarningTime[] = "WarnTime";
constexpr char RepetitiveSound[] = "RepetitiveSound";
constexpr char AcknowledgementTime[] = "AcknowledgementTime";
constexpr char AirfieldFile[] = "AirfieldFile"; // pL
constexpr char AirspaceFile[] = "AirspaceFile"; // pL
constexpr char AdditionalAirspaceFile[] = "AdditionalAirspaceFile"; // pL
constexpr char FlarmFile[] = "FlarmFile";
constexpr char PolarFile[] = "PolarFile"; // pL
constexpr char WaypointFile[] = "WPFile"; // pL
constexpr char AdditionalWaypointFile[] = "AdditionalWPFile"; // pL
constexpr char WatchedWaypointFile[] = "WatchedWPFile"; // pL
constexpr char LanguageFile[] = "LanguageFile"; // pL
constexpr char InputFile[] = "InputFile"; // pL
constexpr char PilotName[] = "PilotName";
constexpr char WeGlideEnabled[] = "WeGlideEnabled";
constexpr char WeGlidePilotID[] = "WeGlidePilotID";
constexpr char WeGlidePilotBirthDate[] = "WeGlidePilotBirthDate";
constexpr char WeGlideAircraftType[] = "WeGlideAircraftType";
constexpr char WeGlideAutomaticUpload[] = "WeGlideAutomaticUpload";
constexpr char CoPilotName[] = "CoPilotName";
constexpr char CrewWeightTemplate[] = "CrewWeightTemplate";
constexpr char AircraftType[] = "AircraftType";
constexpr char AircraftReg[] = "AircraftReg";
constexpr char CompetitionId[] = "AircraftRego";
constexpr char LoggerID[] = "LoggerID";
constexpr char LoggerShort[] = "LoggerShortName";
constexpr char SoundVolume[] = "SoundVolume";
constexpr char SoundDeadband[] = "SoundDeadband";
constexpr char SoundAudioVario[] = "AudioVario2";
constexpr char SoundTask[] = "SoundTask";
constexpr char SoundModes[] = "SoundModes";
constexpr char NettoSpeed[] = "NettoSpeed";
constexpr char AccelerometerZero[] = "AccelerometerZero";

constexpr char AverEffTime[] = "AverEffTime";
constexpr char VarioGauge[] = "VarioGauge";

constexpr char AppIndLandable[] = "AppIndLandable";
constexpr char AppUseSWLandablesRendering[] = "AppUseSWLandablesRendering";
constexpr char AppLandableRenderingScale[] = "AppLandableRenderingScale";
constexpr char AppScaleRunwayLength[] = "AppScaleRunwayLength";
constexpr char AppInverseInfoBox[] = "AppInverseInfoBox";
constexpr char AppGaugeVarioSpeedToFly[] = "AppGaugeVarioSpeedToFly";
constexpr char AppGaugeVarioAvgText[] = "AppGaugeVarioAvgText";
constexpr char AppGaugeVarioMc[] = "AppGaugeVarioMc";
constexpr char AppGaugeVarioBugs[] = "AppGaugeVarioBugs";
constexpr char AppGaugeVarioBallast[] = "AppGaugeVarioBallast";
constexpr char AppGaugeVarioGross[] = "AppGaugeVarioGross";
constexpr char AppStatusMessageAlignment[] = "AppStatusMessageAlignment";
constexpr char AppTextInputStyle[] = "AppTextInputStyle";
constexpr char HapticFeedback[] = "HapticFeedback";
constexpr char AppDialogTabStyle[] = "AppDialogTabStyle";
constexpr char AppDialogStyle[] = "AppDialogStyle";
constexpr char AppInfoBoxColors[] = "AppInfoBoxColors";
constexpr char TeamcodeRefWaypoint[] = "TeamcodeRefWaypoint";
constexpr char AppInfoBoxBorder[] = "AppInfoBoxBorder";
constexpr char ShowMenuButton[] = "ShowMenuButton";
constexpr char CursorSize[] = "CursorSize";
constexpr char CursorColorsInverted[] = "CursorColorsInverted";

constexpr char AppAveNeedle[] = "AppAveNeedle";
constexpr char AppAveThermalNeedle[] = "AppAveThermalNeedle";

constexpr char AutoAdvance[] = "AutoAdvance";
constexpr char UTCOffset[] = "UTCOffset";
constexpr char UTCOffsetSigned[] = "UTCOffsetSigned";
constexpr char BlockSTF[] = "BlockSpeedToFly";
constexpr char AutoZoom[] = "AutoZoom";
constexpr char MenuTimeout[] = "MenuTimeout";
constexpr char TerrainContrast[] = "TerrainContrast";
constexpr char TerrainBrightness[] = "TerrainBrightness";
constexpr char TerrainRamp[] = "TerrainRamp";
constexpr char EnableFLARMMap[] = "EnableFLARMDisplay";
constexpr char EnableFLARMGauge[] = "EnableFLARMGauge";
constexpr char AutoCloseFlarmDialog[] = "AutoCloseFlarmDialog";
constexpr char EnableTAGauge[] = "EnableTAGauge";
constexpr char TAPosition[] = "TAPosition";
constexpr char EnableThermalProfile[] = "EnableThermalProfile";
constexpr char GliderScreenPosition[] = "GliderScreenPosition";
constexpr char SetSystemTimeFromGPS[] = "SetSystemTimeFromGPS";

constexpr char FinishMinHeight[] = "FinishMinHeight";
constexpr char FinishHeightRef[] = "FinishHeightRef";
constexpr char StartMaxHeight[] = "StartMaxHeight";
constexpr char StartMaxSpeed[] = "StartMaxSpeed";
constexpr char StartMaxHeightMargin[] = "StartMaxHeightMargin";
constexpr char StartMaxSpeedMargin[] = "StartMaxSpeedMargin";
constexpr char StartHeightRef[] = "StartHeightRef";
constexpr char StartType[] = "StartType";
constexpr char StartRadius[] = "StartRadius";
constexpr char TurnpointType[] = "TurnpointType";
constexpr char TurnpointRadius[] = "TurnpointRadius";
constexpr char FinishType[] = "FinishType";
constexpr char FinishRadius[] = "FinishRadius";
constexpr char TaskType[] = "TaskType";
constexpr char AATMinTime[] = "AATMinTime";
constexpr char AATTimeMargin[] = "AATTimeMargin";
constexpr char PEVStartWaitTime[] = "PEVStartWaitTime";
constexpr char PEVStartWindow[] = "PEVStartWindow";

constexpr char EnableNavBaroAltitude[] = "EnableNavBaroAltitude";

constexpr char LoggerTimeStepCruise[] = "LoggerTimeStepCruise";
constexpr char LoggerTimeStepCircling[] = "LoggerTimeStepCircling";

constexpr char SafetyMacCready[] = "SafetyMacCready";
constexpr char AbortTaskMode[] = "AbortTaskMode";
constexpr char AutoMcMode[] = "AutoMcMode";
constexpr char AutoMc[] = "AutoMc";
constexpr char EnableExternalTriggerCruise[] = "EnableExternalTriggerCruise";
constexpr char CruiseToCirclingModeSwitchThreshold[] = "CruiseToCirclingModeSwitchThreshold";
constexpr char CirclingToCruiseModeSwitchThreshold[] = "CirclingToCruiseModeSwitchThreshold";
constexpr char OLCRules[] = "OLCRules"; // legacy name, key contains contest rules
constexpr char PredictContest[] = "PredictContest";
constexpr char Handicap[] = "Handicap";
constexpr char SnailWidthScale[] = "SnailWidthScale";
constexpr char SnailType[] = "SnailType";
constexpr char UserLevel[] = "UserLevel";
constexpr char RiskGamma[] = "RiskGamma";
constexpr char PredictWindDrift[] = "PredictWindDrift";
constexpr char WindArrowStyle[] = "WindArrowStyle";
constexpr char EnableFinalGlideBarMC0[] = "EnableFinalGlideBarMC0";
constexpr char FinalGlideBarDisplayMode[] = "FinalGlideBarDisplayMode";
constexpr char EnableVarioBar[] = "EnableVarioBar";
constexpr char ShowFAITriangleAreas[] = "ShowFAITriangleAreas";
constexpr char FAITriangleThreshold[] = "FAITriangleThreshold";
constexpr char AutoLogger[] = "AutoLogger";
constexpr char DisableAutoLogger[] = "DisableAutoLogger";
constexpr char EnableFlightLogger[] = "EnableFlightLogger";
constexpr char EnableNMEALogger[] = "EnableNMEALogger";
constexpr char MapFile[] = "MapFile"; // pL
constexpr char BallastSecsToEmpty[] = "BallastSecsToEmpty";
constexpr char DialogFont[] = "DialogFont";
constexpr char FontInfoWindowFont[] = "InfoWindowFont";
constexpr char FontTitleWindowFont[] = "TitleWindowFont";
constexpr char FontMapWindowFont[] = "MapWindowFont";
constexpr char FontMapWindowBoldFont[] = "MapWindowBoldFont";
constexpr char FontCDIWindowFont[] = "CDIWindowFont";
constexpr char FontMapLabelFont[] = "MapLabelFont";
constexpr char FontMapLabelImportantFont[] = "MapLabelImportantFont";
constexpr char FontStatisticsFont[] = "StatisticsFont";
constexpr char FontBugsBallastFont[] = "BugsBallastFont";
constexpr char FontAirspacePressFont[] = "AirspacePressFont";
constexpr char FontAirspaceColourDlgFont[] = "AirspaceColourDlgFont";
constexpr char FontTeamCodeFont[] = "TeamCodeFont";

constexpr char UseFinalGlideDisplayMode[] = "UseFinalGlideDisplayMode";
constexpr char InfoBoxGeometry[] = "InfoBoxGeometry";

constexpr char FlarmSideData[] = "FlarmRadarSideData";
constexpr char FlarmAutoZoom[] = "FlarmRadarAutoZoom";
constexpr char FlarmNorthUp[] = "FlarmRadarNorthUp";

constexpr char IgnoreNMEAChecksum[] = "IgnoreNMEAChecksum";
constexpr char MapOrientation[] = "DisplayOrientation";

constexpr char ClimbMapScale[] = "ClimbMapScale";
constexpr char CruiseMapScale[] = "CruiseMapScale";

constexpr char RoutePlannerMode[] = "RoutePlannerMode";
constexpr char RoutePlannerAllowClimb[] = "RoutePlannerAllowClimb";
constexpr char RoutePlannerUseCeiling[] = "RoutePlannerUseCeiling";
constexpr char TurningReach[] = "TurningReach";
constexpr char ReachPolarMode[] = "ReachPolarMode";

constexpr char AircraftSymbol[] = "AircraftSymbol";

constexpr char FlarmLocation[] = "FlarmLocation";

constexpr char SkyLinesTrackingEnabled[] = "SkyLinesTrackingEnabled";
constexpr char SkyLinesRoaming[] = "SkyLinesRoaming";
constexpr char SkyLinesTrackingInterval[] = "SkyLinesTrackingInterval";
constexpr char SkyLinesTrafficEnabled[] = "SkyLinesTrafficEnabled";
constexpr char SkyLinesNearTrafficEnabled[] = "SkyLinesNearTrafficEnabled";
constexpr char SkyLinesTrafficMapMode[] = "SkyLinesTrafficMapMode";
constexpr char SkyLinesTrackingKey[] = "SkyLinesTrackingKey";

constexpr char CloudEnabled[] = "CloudEnabled";
constexpr char CloudShowThermals[] = "CloudShowThermals";
constexpr char CloudKey[] = "CloudKey";

constexpr char LiveTrack24Enabled[] = "LiveTrack24Enabled";
constexpr char LiveTrack24Server[] = "LiveTrack24Server";
constexpr char LiveTrack24Username[] = "LiveTrack24Username";
constexpr char LiveTrack24Password[] = "LiveTrack24Password";
constexpr char LiveTrack24TrackingInterval[] = "TrackingInterval";
constexpr char LiveTrack24TrackingVehicleType[] = "TrackingVehicleType";
constexpr char LiveTrack24TrackingVehicleName[] = "TrackingVehicleName";

constexpr char PCMetUsername[] = "PCMetUsername";
constexpr char PCMetPassword[] = "PCMetPassword";
constexpr char PCMetFtpUsername[] = "PCMetFtpUsername";
constexpr char PCMetFtpPassword[] = "PCMetFtpPassword";

constexpr char EnableThermalInformationMap[] = "EnableThermalInformationMap";

constexpr char EnableLocationMapItem[] = "EnableLocationMapItem";
constexpr char EnableArrivalAltitudeMapItem[] = "EnableArrivalAltitudeMapItem";

constexpr char VarioMinFrequency[] = "VarioMinFrequency";
constexpr char VarioZeroFrequency[] = "VarioZeroFrequency";
constexpr char VarioMaxFrequency[] = "VarioMaxFrequency";
constexpr char VarioMinPeriod[] = "VarioMinPeriod";
constexpr char VarioMaxPeriod[] = "VarioMaxPeriod";
constexpr char VarioDeadBandEnabled[] = "VarioDeadBandEnabled";
constexpr char VarioDeadBandMin[] = "VarioDeadBandMin";
constexpr char VarioDeadBandMax[] = "VarioDeadBandMax";

constexpr char PagesDistinctZoom[] = "PagesDistinctZoom";

constexpr char WaveAssistant[] = "WaveAssistant";

constexpr char MasterAudioVolume[] = "MasterAudioVolume";

constexpr char RaspFile[] = "RaspFile";

}
