// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

namespace ProfileKeys {

constexpr std::string_view FullScreen = "FullScreen";
constexpr std::string_view UIScale = "UIScale";
constexpr std::string_view CustomDPI = "CustomDPI";
constexpr std::string_view DarkMode = "DarkMode";
constexpr std::string_view Password = "Password";
constexpr std::string_view AirspaceWarning = "AirspaceWarn";
constexpr std::string_view AirspaceWarningDialog = "AirspaceWarnDialog";
constexpr std::string_view AirspaceBlackOutline = "AirspaceBlackOutline";
constexpr std::string_view AirspaceTransparency = "AirspaceTransparency";
constexpr std::string_view AirspaceFillMode = "AirspaceFillMode";
constexpr std::string_view AirspaceLabelSelection = "AirspaceLabelSelection";
constexpr std::string_view AltMargin = "AltMargin";
constexpr std::string_view AltMode = "AltitudeMode";
constexpr std::string_view AltitudeUnitsValue = "AltitudeUnit";
constexpr std::string_view TemperatureUnitsValue = "TemperatureUnit";
constexpr std::string_view CircleZoom = "CircleZoom";
constexpr std::string_view MaxAutoZoomDistance = "MaxAutoZoomDistance";
constexpr std::string_view ClipAlt = "ClipAlt";
constexpr std::string_view DisplayText = "DisplayText";
constexpr std::string_view WaypointArrivalHeightDisplay = "WaypointArrivalHeightDisplay";
constexpr std::string_view WaypointLabelSelection = "WayPointLabelSelection";
constexpr std::string_view WaypointLabelStyle = "WayPointLabelStyle";
constexpr std::string_view WeatherStations = "WeatherStations";
constexpr std::string_view DisplayUpValue = "DisplayUp";
constexpr std::string_view OrientationCruise = "OrientationCruise";
constexpr std::string_view OrientationCircling = "OrientationCircling";
constexpr std::string_view MapShiftBias = "MapShiftBias";
constexpr std::string_view DistanceUnitsValue = "DistanceUnit";
constexpr std::string_view DrawTerrain = "DrawTerrain";
constexpr std::string_view SlopeShading = "SlopeShading";
constexpr std::string_view SlopeShadingType = "SlopeShadingType";
constexpr std::string_view TerrainContours = "TerrainContours";
constexpr std::string_view DrawTopography = "DrawTopology";
constexpr std::string_view FinalGlideTerrain = "FinalGlideTerrain";
constexpr std::string_view AutoWind = "AutoWind";
constexpr std::string_view ExternalWind = "ExternalWind";
constexpr std::string_view HomeWaypoint = "HomeWaypoint";
constexpr std::string_view HomeLocation = "HomeLocation";
constexpr std::string_view LiftUnitsValue = "LiftUnit";
constexpr std::string_view PressureUnitsValue = "Pressure";
constexpr std::string_view WingLoadingUnitValue = "WingLoadingUnit";
constexpr std::string_view MassUnitValue = "MassUnit";
constexpr std::string_view RotationUnitValue = "RotationUnit";
constexpr std::string_view LatLonUnits = "LatLonUnits";
constexpr std::string_view PolarID = "Polar";
constexpr std::string_view Polar = "PolarInformation";
constexpr std::string_view PolarName = "PolarName";
constexpr std::string_view PolarDegradation = "PolarDegradation";
constexpr std::string_view AutoBugs = "AutoBugs";
constexpr std::string_view SafetyAltitudeArrival = "SafetyAltitudeArrival";
constexpr std::string_view SafetyAltitudeTerrain = "SafetyAltitudeTerrain";
constexpr std::string_view SafteySpeed = "SafteySpeed";
constexpr std::string_view DryMass = "DryMass";
constexpr std::string_view SnailTrail = "SnailTrail";
constexpr std::string_view TrailDrift = "TrailDrift";
constexpr std::string_view DetourCostMarker = "DetourCostMarker";
constexpr std::string_view DisplayTrackBearing = "DisplayTrackBearing";
constexpr std::string_view SpeedUnitsValue = "SpeedUnit";
constexpr std::string_view TaskSpeedUnitsValue = "TaskSpeedUnit";
constexpr std::string_view WarningTime = "WarnTime";
constexpr std::string_view RepetitiveSound = "RepetitiveSound";
constexpr std::string_view AcknowledgementTime = "AcknowledgementTime";
constexpr std::string_view AirfieldFile = "AirfieldFile"; // pL
constexpr std::string_view AirspaceFile = "AirspaceFile"; // pL
constexpr std::string_view AdditionalAirspaceFile = "AdditionalAirspaceFile"; // pL
constexpr std::string_view FlarmFile = "FlarmFile";
constexpr std::string_view PolarFile = "PolarFile"; // pL
constexpr std::string_view WaypointFile = "WPFile"; // pL
constexpr std::string_view AdditionalWaypointFile = "AdditionalWPFile"; // pL
constexpr std::string_view WatchedWaypointFile = "WatchedWPFile"; // pL
constexpr std::string_view LanguageFile = "LanguageFile"; // pL
constexpr std::string_view InputFile = "InputFile"; // pL
constexpr std::string_view PilotName = "PilotName";
constexpr std::string_view WeGlideEnabled = "WeGlideEnabled";
constexpr std::string_view WeGlidePilotID = "WeGlidePilotID";
constexpr std::string_view WeGlidePilotBirthDate = "WeGlidePilotBirthDate";
constexpr std::string_view WeGlideAircraftType = "WeGlideAircraftType";
constexpr std::string_view WeGlideAutomaticUpload = "WeGlideAutomaticUpload";
constexpr std::string_view CoPilotName = "CoPilotName";
constexpr std::string_view CrewWeightTemplate = "CrewWeightTemplate";
constexpr std::string_view AircraftType = "AircraftType";
constexpr std::string_view AircraftReg = "AircraftReg";
constexpr std::string_view CompetitionId = "AircraftRego";
constexpr std::string_view LoggerID = "LoggerID";
constexpr std::string_view LoggerShort = "LoggerShortName";
constexpr std::string_view SoundVolume = "SoundVolume";
constexpr std::string_view SoundDeadband = "SoundDeadband";
constexpr std::string_view SoundAudioVario = "AudioVario2";
constexpr std::string_view SoundTask = "SoundTask";
constexpr std::string_view SoundModes = "SoundModes";
constexpr std::string_view NettoSpeed = "NettoSpeed";
constexpr std::string_view AccelerometerZero = "AccelerometerZero";

constexpr std::string_view AverEffTime = "AverEffTime";
constexpr std::string_view VarioGauge = "VarioGauge";

constexpr std::string_view AppIndLandable = "AppIndLandable";
constexpr std::string_view AppUseSWLandablesRendering = "AppUseSWLandablesRendering";
constexpr std::string_view AppLandableRenderingScale = "AppLandableRenderingScale";
constexpr std::string_view AppScaleRunwayLength = "AppScaleRunwayLength";

/** deprecated, use #DarkMode */
constexpr std::string_view AppInverseInfoBox = "AppInverseInfoBox";

constexpr std::string_view AppGaugeVarioSpeedToFly = "AppGaugeVarioSpeedToFly";
constexpr std::string_view AppGaugeVarioAvgText = "AppGaugeVarioAvgText";
constexpr std::string_view AppGaugeVarioMc = "AppGaugeVarioMc";
constexpr std::string_view AppGaugeVarioBugs = "AppGaugeVarioBugs";
constexpr std::string_view AppGaugeVarioBallast = "AppGaugeVarioBallast";
constexpr std::string_view AppGaugeVarioGross = "AppGaugeVarioGross";
constexpr std::string_view AppStatusMessageAlignment = "AppStatusMessageAlignment";
constexpr std::string_view AppTextInputStyle = "AppTextInputStyle";
constexpr std::string_view HapticFeedback = "HapticFeedback";
constexpr std::string_view AppDialogTabStyle = "AppDialogTabStyle";
constexpr std::string_view AppDialogStyle = "AppDialogStyle";
constexpr std::string_view AppInfoBoxColors = "AppInfoBoxColors";
constexpr std::string_view TeamcodeRefWaypoint = "TeamcodeRefWaypoint";
constexpr std::string_view AppInfoBoxBorder = "AppInfoBoxBorder";
constexpr std::string_view ShowMenuButton = "ShowMenuButton";
constexpr std::string_view CursorSize = "CursorSize";
constexpr std::string_view CursorColorsInverted = "CursorColorsInverted";

constexpr std::string_view AppAveNeedle = "AppAveNeedle";
constexpr std::string_view AppAveThermalNeedle = "AppAveThermalNeedle";

constexpr std::string_view AutoAdvance = "AutoAdvance";
constexpr std::string_view UTCOffset = "UTCOffset";
constexpr std::string_view UTCOffsetSigned = "UTCOffsetSigned";
constexpr std::string_view BlockSTF = "BlockSpeedToFly";
constexpr std::string_view AutoZoom = "AutoZoom";
constexpr std::string_view MenuTimeout = "MenuTimeout";
constexpr std::string_view TerrainContrast = "TerrainContrast";
constexpr std::string_view TerrainBrightness = "TerrainBrightness";
constexpr std::string_view TerrainRamp = "TerrainRamp";
constexpr std::string_view EnableFLARMMap = "EnableFLARMDisplay";
constexpr std::string_view FadeTraffic = "FadeTraffic";
constexpr std::string_view EnableFLARMGauge = "EnableFLARMGauge";
constexpr std::string_view AutoCloseFlarmDialog = "AutoCloseFlarmDialog";
constexpr std::string_view EnableTAGauge = "EnableTAGauge";
constexpr std::string_view TAPosition = "TAPosition";
constexpr std::string_view EnableThermalProfile = "EnableThermalProfile";
constexpr std::string_view GliderScreenPosition = "GliderScreenPosition";
constexpr std::string_view SetSystemTimeFromGPS = "SetSystemTimeFromGPS";

constexpr std::string_view FinishMinHeight = "FinishMinHeight";
constexpr std::string_view FinishHeightRef = "FinishHeightRef";
constexpr std::string_view StartMaxHeight = "StartMaxHeight";
constexpr std::string_view StartMaxSpeed = "StartMaxSpeed";
constexpr std::string_view StartMaxHeightMargin = "StartMaxHeightMargin";
constexpr std::string_view StartMaxSpeedMargin = "StartMaxSpeedMargin";
constexpr std::string_view StartHeightRef = "StartHeightRef";
constexpr std::string_view StartType = "StartType";
constexpr std::string_view StartRadius = "StartRadius";
constexpr std::string_view TurnpointType = "TurnpointType";
constexpr std::string_view TurnpointRadius = "TurnpointRadius";
constexpr std::string_view FinishType = "FinishType";
constexpr std::string_view FinishRadius = "FinishRadius";
constexpr std::string_view TaskType = "TaskType";
constexpr std::string_view AATMinTime = "AATMinTime";
constexpr std::string_view AATTimeMargin = "AATTimeMargin";
constexpr std::string_view PEVStartWaitTime = "PEVStartWaitTime";
constexpr std::string_view PEVStartWindow = "PEVStartWindow";

constexpr std::string_view EnableNavBaroAltitude = "EnableNavBaroAltitude";

constexpr std::string_view LoggerTimeStepCruise = "LoggerTimeStepCruise";
constexpr std::string_view LoggerTimeStepCircling = "LoggerTimeStepCircling";

constexpr std::string_view SafetyMacCready = "SafetyMacCready";
constexpr std::string_view AbortTaskMode = "AbortTaskMode";
constexpr std::string_view AutoMcMode = "AutoMcMode";
constexpr std::string_view AutoMc = "AutoMc";
constexpr std::string_view EnableExternalTriggerCruise = "EnableExternalTriggerCruise";
constexpr std::string_view CruiseToCirclingModeSwitchThreshold = "CruiseToCirclingModeSwitchThreshold";
constexpr std::string_view CirclingToCruiseModeSwitchThreshold = "CirclingToCruiseModeSwitchThreshold";
constexpr std::string_view OLCRules = "OLCRules"; // legacy name, key contains contest rules
constexpr std::string_view PredictContest = "PredictContest";
constexpr std::string_view Handicap = "Handicap";
constexpr std::string_view SnailWidthScale = "SnailWidthScale";
constexpr std::string_view SnailType = "SnailType";
constexpr std::string_view UserLevel = "UserLevel";
constexpr std::string_view RiskGamma = "RiskGamma";
constexpr std::string_view PredictWindDrift = "PredictWindDrift";
constexpr std::string_view WindArrowStyle = "WindArrowStyle";
constexpr std::string_view EnableFinalGlideBarMC0 = "EnableFinalGlideBarMC0";
constexpr std::string_view FinalGlideBarDisplayMode = "FinalGlideBarDisplayMode";
constexpr std::string_view EnableVarioBar = "EnableVarioBar";
constexpr std::string_view ShowFAITriangleAreas = "ShowFAITriangleAreas";
constexpr std::string_view FAITriangleThreshold = "FAITriangleThreshold";
constexpr std::string_view AutoLogger = "AutoLogger";
constexpr std::string_view DisableAutoLogger = "DisableAutoLogger";
constexpr std::string_view EnableFlightLogger = "EnableFlightLogger";
constexpr std::string_view EnableNMEALogger = "EnableNMEALogger";
constexpr std::string_view MapFile = "MapFile"; // pL
constexpr std::string_view BallastSecsToEmpty = "BallastSecsToEmpty";
constexpr std::string_view DialogFont = "DialogFont";
constexpr std::string_view FontInfoWindowFont = "InfoWindowFont";
constexpr std::string_view FontTitleWindowFont = "TitleWindowFont";
constexpr std::string_view FontMapWindowFont = "MapWindowFont";
constexpr std::string_view FontMapWindowBoldFont = "MapWindowBoldFont";
constexpr std::string_view FontCDIWindowFont = "CDIWindowFont";
constexpr std::string_view FontMapLabelFont = "MapLabelFont";
constexpr std::string_view FontMapLabelImportantFont = "MapLabelImportantFont";
constexpr std::string_view FontStatisticsFont = "StatisticsFont";
constexpr std::string_view FontBugsBallastFont = "BugsBallastFont";
constexpr std::string_view FontAirspacePressFont = "AirspacePressFont";
constexpr std::string_view FontAirspaceColourDlgFont = "AirspaceColourDlgFont";
constexpr std::string_view FontTeamCodeFont = "TeamCodeFont";

constexpr std::string_view UseFinalGlideDisplayMode = "UseFinalGlideDisplayMode";
constexpr std::string_view InfoBoxGeometry = "InfoBoxGeometry";
constexpr std::string_view InfoBoxTitleScale = "InfoBoxTitleScale";

constexpr std::string_view FlarmSideData = "FlarmRadarSideData";
constexpr std::string_view FlarmAutoZoom = "FlarmRadarAutoZoom";
constexpr std::string_view FlarmNorthUp = "FlarmRadarNorthUp";

constexpr std::string_view IgnoreNMEAChecksum = "IgnoreNMEAChecksum";
constexpr std::string_view MapOrientation = "DisplayOrientation";

constexpr std::string_view ClimbMapScale = "ClimbMapScale";
constexpr std::string_view CruiseMapScale = "CruiseMapScale";

constexpr std::string_view RoutePlannerMode = "RoutePlannerMode";
constexpr std::string_view RoutePlannerAllowClimb = "RoutePlannerAllowClimb";
constexpr std::string_view RoutePlannerUseCeiling = "RoutePlannerUseCeiling";
constexpr std::string_view TurningReach = "TurningReach";
constexpr std::string_view ReachPolarMode = "ReachPolarMode";

constexpr std::string_view AircraftSymbol = "AircraftSymbol";

constexpr std::string_view FlarmLocation = "FlarmLocation";

constexpr std::string_view SkyLinesTrackingEnabled = "SkyLinesTrackingEnabled";
constexpr std::string_view SkyLinesRoaming = "SkyLinesRoaming";
constexpr std::string_view SkyLinesTrackingInterval = "SkyLinesTrackingInterval";
constexpr std::string_view SkyLinesTrafficEnabled = "SkyLinesTrafficEnabled";
constexpr std::string_view SkyLinesNearTrafficEnabled = "SkyLinesNearTrafficEnabled";
constexpr std::string_view SkyLinesTrafficMapMode = "SkyLinesTrafficMapMode";
constexpr std::string_view SkyLinesTrackingKey = "SkyLinesTrackingKey";

constexpr std::string_view CloudEnabled = "CloudEnabled";
constexpr std::string_view CloudShowThermals = "CloudShowThermals";
constexpr std::string_view CloudKey = "CloudKey";

constexpr std::string_view LiveTrack24Enabled = "LiveTrack24Enabled";
constexpr std::string_view LiveTrack24Server = "LiveTrack24Server";
constexpr std::string_view LiveTrack24Username = "LiveTrack24Username";
constexpr std::string_view LiveTrack24Password = "LiveTrack24Password";
constexpr std::string_view LiveTrack24TrackingInterval = "TrackingInterval";
constexpr std::string_view LiveTrack24TrackingVehicleType = "TrackingVehicleType";
constexpr std::string_view LiveTrack24TrackingVehicleName = "TrackingVehicleName";

constexpr std::string_view PCMetUsername = "PCMetUsername";
constexpr std::string_view PCMetPassword = "PCMetPassword";
constexpr std::string_view PCMetFtpUsername = "PCMetFtpUsername";
constexpr std::string_view PCMetFtpPassword = "PCMetFtpPassword";

constexpr std::string_view EnableThermalInformationMap = "EnableThermalInformationMap";

constexpr std::string_view EnableLocationMapItem = "EnableLocationMapItem";
constexpr std::string_view EnableArrivalAltitudeMapItem = "EnableArrivalAltitudeMapItem";

constexpr std::string_view VarioMinFrequency = "VarioMinFrequency";
constexpr std::string_view VarioZeroFrequency = "VarioZeroFrequency";
constexpr std::string_view VarioMaxFrequency = "VarioMaxFrequency";
constexpr std::string_view VarioMinPeriod = "VarioMinPeriod";
constexpr std::string_view VarioMaxPeriod = "VarioMaxPeriod";
constexpr std::string_view VarioDeadBandEnabled = "VarioDeadBandEnabled";
constexpr std::string_view VarioDeadBandMin = "VarioDeadBandMin";
constexpr std::string_view VarioDeadBandMax = "VarioDeadBandMax";

constexpr std::string_view PagesDistinctZoom = "PagesDistinctZoom";

constexpr std::string_view WaveAssistant = "WaveAssistant";

constexpr std::string_view MasterAudioVolume = "MasterAudioVolume";

constexpr std::string_view RaspFile = "RaspFile";

}
