/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include <tchar.h>

namespace ProfileKeys {

extern const TCHAR SpeedUnitsValue[];
extern const TCHAR DistanceUnitsValue[];
extern const TCHAR AltitudeUnitsValue[];
extern const TCHAR TemperatureUnitsValue[];
extern const TCHAR LiftUnitsValue[];
extern const TCHAR PressureUnitsValue[];
extern const TCHAR TaskSpeedUnitsValue[];
extern const TCHAR DisplayUpValue[];
extern const TCHAR OrientationCruise[];
extern const TCHAR OrientationCircling[];
extern const TCHAR MapShiftBias[];
extern const TCHAR DisplayText[];
extern const TCHAR WaypointArrivalHeightDisplay[];
extern const TCHAR WaypointLabelSelection[];
extern const TCHAR WaypointLabelStyle[];
extern const TCHAR WeatherStations[];
extern const TCHAR SafetyAltitudeArrival[];
extern const TCHAR SafetyAltitudeTerrain[];
extern const TCHAR SafteySpeed[];
extern const TCHAR DryMass[];
extern const TCHAR PolarID[];
extern const TCHAR Polar[];
extern const TCHAR PolarName[];
extern const TCHAR PolarDegradation[];
extern const TCHAR WaypointFile[];
extern const TCHAR AdditionalWaypointFile[];
extern const TCHAR WatchedWaypointFile[];
extern const TCHAR AirspaceFile[];
extern const TCHAR AdditionalAirspaceFile[];
extern const TCHAR AirfieldFile[];
extern const TCHAR PolarFile[];
extern const TCHAR LanguageFile[];
extern const TCHAR StatusFile[];
extern const TCHAR InputFile[];
extern const TCHAR AltMode[];
extern const TCHAR ClipAlt[];
extern const TCHAR AltMargin[];
extern const TCHAR SnailTrail[];
extern const TCHAR DrawTopography[];
extern const TCHAR DrawTerrain[];
extern const TCHAR SlopeShading[];
extern const TCHAR SlopeShadingType[];
extern const TCHAR FinalGlideTerrain[];
extern const TCHAR AutoWind[];
extern const TCHAR ExternalWind[];
extern const TCHAR AirspaceWarning[];
extern const TCHAR AirspaceBlackOutline[];
extern const TCHAR AirspaceTransparency[];
extern const TCHAR AirspaceFillMode[];
extern const TCHAR WarningTime[];
extern const TCHAR AcknowledgementTime[];
extern const TCHAR CircleZoom[];
extern const TCHAR MaxAutoZoomDistance[];
extern const TCHAR HomeWaypoint[];
extern const TCHAR HomeLocation[];
extern const TCHAR TeamcodeRefWaypoint[];
extern const TCHAR PilotName[];
extern const TCHAR AircraftType[];
extern const TCHAR AircraftReg[];
extern const TCHAR CompetitionId[];
extern const TCHAR LoggerID[];
extern const TCHAR LoggerShort[];
extern const TCHAR SoundVolume[];
extern const TCHAR SoundDeadband[];
extern const TCHAR SoundAudioVario[];
extern const TCHAR SoundTask[];
extern const TCHAR SoundModes[];
extern const TCHAR NettoSpeed[];
extern const TCHAR AutoBlank[];
extern const TCHAR AverEffTime[]; // VENTA6
extern const TCHAR VarioGauge[];
extern const TCHAR AppIndLandable[];
extern const TCHAR AppUseSWLandablesRendering[];
extern const TCHAR AppLandableRenderingScale[];
extern const TCHAR AppScaleRunwayLength[];
extern const TCHAR AppInverseInfoBox[];
extern const TCHAR AppInfoBoxColors[];
extern const TCHAR AppGaugeVarioSpeedToFly[];
extern const TCHAR AppGaugeVarioAvgText[];
extern const TCHAR AppGaugeVarioMc[];
extern const TCHAR AppGaugeVarioBugs[];
extern const TCHAR AppGaugeVarioBallast[];
extern const TCHAR AppGaugeVarioGross[];
extern const TCHAR AppStatusMessageAlignment[];
extern const TCHAR AppTextInputStyle[];
extern const TCHAR HapticFeedback[];
extern const TCHAR AppDialogTabStyle[];
extern const TCHAR AppDialogStyle[];
extern const TCHAR AppInfoBoxBorder[];
extern const TCHAR AppInfoBoxModel[]; // VENTA-ADDON MODEL CHANGE
extern const TCHAR AppAveNeedle[];
extern const TCHAR AutoAdvance[];
extern const TCHAR UTCOffset[];
extern const TCHAR UTCOffsetSigned[];
extern const TCHAR BlockSTF[];
extern const TCHAR AutoZoom[];
extern const TCHAR MenuTimeout[];
extern const TCHAR TerrainContrast[];
extern const TCHAR TerrainBrightness[];
extern const TCHAR TerrainRamp[];
extern const TCHAR EnableFLARMMap[];
extern const TCHAR EnableFLARMGauge[];
extern const TCHAR AutoCloseFlarmDialog[];
extern const TCHAR EnableTAGauge[];
extern const TCHAR EnableThermalProfile[];
extern const TCHAR TrailDrift[];
extern const TCHAR DetourCostMarker[];
extern const TCHAR DisplayTrackBearing[];
extern const TCHAR GliderScreenPosition[];
extern const TCHAR SetSystemTimeFromGPS[];

extern const TCHAR VoiceClimbRate[];
extern const TCHAR VoiceTerrain[];
extern const TCHAR VoiceWaypointDistance[];
extern const TCHAR VoiceTaskAltitudeDifference[];
extern const TCHAR VoiceMacCready[];
extern const TCHAR VoiceNewWaypoint[];
extern const TCHAR VoiceInSector[];
extern const TCHAR VoiceAirspace[];

extern const TCHAR FinishMinHeight[];
extern const TCHAR FinishHeightRef[];
extern const TCHAR StartMaxHeight[];
extern const TCHAR StartMaxHeightMargin[];
extern const TCHAR StartMaxSpeed[];
extern const TCHAR StartMaxSpeedMargin[];
extern const TCHAR StartHeightRef[];
extern const TCHAR StartType[];
extern const TCHAR StartRadius[];
extern const TCHAR TurnpointType[];
extern const TCHAR TurnpointRadius[];
extern const TCHAR FinishType[];
extern const TCHAR FinishRadius[];
extern const TCHAR TaskType[];
extern const TCHAR AATMinTime[];
extern const TCHAR AATTimeMargin[];

extern const TCHAR EnableNavBaroAltitude[];

extern const TCHAR LoggerTimeStepCruise[];
extern const TCHAR LoggerTimeStepCircling[];

extern const TCHAR SafetyMacCready[];
extern const TCHAR AbortTaskMode[];
extern const TCHAR AutoMcMode[];
extern const TCHAR AutoMc[];
extern const TCHAR EnableExternalTriggerCruise[];
extern const TCHAR OLCRules[];
extern const TCHAR PredictContest[];
extern const TCHAR Handicap[];
extern const TCHAR SnailWidthScale[];
extern const TCHAR SnailType[];
extern const TCHAR LatLonUnits[];
extern const TCHAR UserLevel[];
extern const TCHAR RiskGamma[];
extern const TCHAR PredictWindDrift[];
extern const TCHAR WindArrowStyle[];
extern const TCHAR EnableFinalGlideBarMC0[];
extern const TCHAR ShowFAITriangleAreas[];
extern const TCHAR AutoLogger[];
extern const TCHAR DisableAutoLogger[];
extern const TCHAR EnableFlightLogger[];
extern const TCHAR EnableNMEALogger[];
extern const TCHAR MapFile[];
extern const TCHAR BallastSecsToEmpty[];
extern const TCHAR AccelerometerZero[];
extern const TCHAR UseCustomFonts[];
extern const TCHAR FontInfoWindowFont[];
extern const TCHAR FontTitleWindowFont[];
extern const TCHAR FontMapWindowFont[];
extern const TCHAR FontTitleSmallWindowFont[];
extern const TCHAR FontMapWindowBoldFont[];
extern const TCHAR FontCDIWindowFont[];
extern const TCHAR FontMapLabelFont[];
extern const TCHAR FontMapLabelImportantFont[];
extern const TCHAR FontStatisticsFont[];
extern const TCHAR FontBugsBallastFont[];
extern const TCHAR FontAirspacePressFont[];
extern const TCHAR FontAirspaceColourDlgFont[];
extern const TCHAR FontTeamCodeFont[];

extern const TCHAR UseFinalGlideDisplayMode[];

extern const TCHAR InfoBoxGeometry[];

extern const TCHAR FlarmSideData[];
extern const TCHAR FlarmAutoZoom[];
extern const TCHAR FlarmNorthUp[];

extern const TCHAR IgnoreNMEAChecksum[];
extern const TCHAR DisplayOrientation[];

extern const TCHAR ClimbMapScale[];
extern const TCHAR CruiseMapScale[];

extern const TCHAR RoutePlannerMode[];
extern const TCHAR RoutePlannerAllowClimb[];
extern const TCHAR RoutePlannerUseCeiling[];
extern const TCHAR TurningReach[];
extern const TCHAR ReachPolarMode[];

extern const TCHAR AircraftSymbol[];

extern const TCHAR FlarmLocation[];

extern const TCHAR TrackingInterval[];
extern const TCHAR TrackingVehicleType[];
extern const TCHAR SkyLinesTrackingEnabled[];
extern const TCHAR SkyLinesTrackingInterval[];
extern const TCHAR SkyLinesTrafficEnabled[];
extern const TCHAR SkyLinesTrackingKey[];
extern const TCHAR LiveTrack24Enabled[];
extern const TCHAR LiveTrack24Server[];
extern const TCHAR LiveTrack24Username[];
extern const TCHAR LiveTrack24Password[];

extern const TCHAR EnableLocationMapItem[];
extern const TCHAR EnableArrivalAltitudeMapItem[];

extern const TCHAR VarioMinFrequency[];
extern const TCHAR VarioZeroFrequency[];
extern const TCHAR VarioMaxFrequency[];
extern const TCHAR VarioMinPeriod[];
extern const TCHAR VarioMaxPeriod[];
extern const TCHAR VarioDeadBandEnabled[];
extern const TCHAR VarioDeadBandMin[];
extern const TCHAR VarioDeadBandMax[];

extern const TCHAR PagesDistinctZoom[];

}

#endif
