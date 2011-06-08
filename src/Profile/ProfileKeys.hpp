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

#ifndef XCSOAR_UTILS_PROFILE_HPP
#define XCSOAR_UTILS_PROFILE_HPP

#include <tchar.h>

extern const TCHAR *szProfileColour[];
extern const TCHAR *szProfileBrush[];
extern const TCHAR *szProfileAirspaceMode[];
extern const TCHAR szProfileSpeedUnitsValue[];
extern const TCHAR szProfileDistanceUnitsValue[];
extern const TCHAR szProfileAltitudeUnitsValue[];
extern const TCHAR szProfileTemperatureUnitsValue[];
extern const TCHAR szProfileLiftUnitsValue[];
extern const TCHAR szProfileTaskSpeedUnitsValue[];
extern const TCHAR szProfileDisplayUpValue[];
extern const TCHAR szProfileOrientationCruise[];
extern const TCHAR szProfileOrientationCircling[];
extern const TCHAR szProfileMapShiftBias[];
extern const TCHAR szProfileDisplayText[];
extern const TCHAR szProfileWaypointArrivalHeightDisplay[];
extern const TCHAR szProfileWaypointLabelSelection[];
extern const TCHAR szProfileWaypointLabelStyle[];
extern const TCHAR szProfileWeatherStations[];
extern const TCHAR szProfileSafetyAltitudeArrival[];
extern const TCHAR szProfileSafetyAltitudeTerrain[];
extern const TCHAR szProfileSafteySpeed[];
extern const TCHAR szProfileDryMass[];
extern const TCHAR szProfilePolarID[];
extern const TCHAR szProfilePolar[];
extern const TCHAR szProfilePolarName[];
extern const TCHAR szProfileWaypointFile[];
extern const TCHAR szProfileAdditionalWaypointFile[];
extern const TCHAR szProfileWatchedWaypointFile[];
extern const TCHAR szProfileAirspaceFile[];
extern const TCHAR szProfileAdditionalAirspaceFile[];
extern const TCHAR szProfileAirfieldFile[];
extern const TCHAR szProfileTopographyFile[];
extern const TCHAR szProfilePolarFile[];
extern const TCHAR szProfileTerrainFile[];
extern const TCHAR szProfileLanguageFile[];
extern const TCHAR szProfileStatusFile[];
extern const TCHAR szProfileInputFile[];
extern const TCHAR szProfileAltMode[];
extern const TCHAR szProfileClipAlt[];
extern const TCHAR szProfileAltMargin[];
extern const TCHAR szProfileSnailTrail[];
extern const TCHAR szProfileDrawTopography[];
extern const TCHAR szProfileDrawTerrain[];
extern const TCHAR szProfileSlopeShading[];
extern const TCHAR szProfileSlopeShadingType[];
extern const TCHAR szProfileFinalGlideTerrain[];
extern const TCHAR szProfileAutoWind[];
extern const TCHAR szProfileExternalWind[];
extern const TCHAR szProfileAirspaceWarning[];
extern const TCHAR szProfileAirspaceBlackOutline[];
extern const TCHAR szProfileAirspaceTransparency[];
extern const TCHAR szProfileAirspaceFillMode[];
extern const TCHAR szProfileWarningTime[];
extern const TCHAR szProfileAcknowledgementTime[];
extern const TCHAR szProfileCircleZoom[];
extern const TCHAR szProfileMaxAutoZoomDistance[];
extern const TCHAR szProfileHomeWaypoint[];
extern const TCHAR szProfileHomeLocation[];
extern const TCHAR szProfileTeamcodeRefWaypoint[];
extern const TCHAR szProfilePilotName[];
extern const TCHAR szProfileAircraftType[];
extern const TCHAR szProfileAircraftReg[];
extern const TCHAR szProfileCompetitionId[];
extern const TCHAR szProfileLoggerID[];
extern const TCHAR szProfileLoggerShort[];
extern const TCHAR szProfileSoundVolume[];
extern const TCHAR szProfileSoundDeadband[];
extern const TCHAR szProfileSoundAudioVario[];
extern const TCHAR szProfileSoundTask[];
extern const TCHAR szProfileSoundModes[];
extern const TCHAR szProfileNettoSpeed[];
extern const TCHAR szProfileAutoBlank[];
extern const TCHAR szProfileAverEffTime[]; // VENTA6
extern const TCHAR szProfileVarioGauge[];
extern const TCHAR szProfileDebounceTimeout[];
extern const TCHAR szProfileAppIndLandable[];
extern const TCHAR szProfileAppUseSWLandablesRendering[];
extern const TCHAR szProfileAppLandableRenderingScale[];
extern const TCHAR szProfileAppScaleRunwayLength[];
extern const TCHAR szProfileAppInverseInfoBox[];
extern const TCHAR szProfileAppInfoBoxColors[];
extern const TCHAR szProfileAppGaugeVarioSpeedToFly[];
extern const TCHAR szProfileAppGaugeVarioAvgText[];
extern const TCHAR szProfileAppGaugeVarioMc[];
extern const TCHAR szProfileAppGaugeVarioBugs[];
extern const TCHAR szProfileAppGaugeVarioBallast[];
extern const TCHAR szProfileAppGaugeVarioGross[];
extern const TCHAR szProfileAppStatusMessageAlignment[];
extern const TCHAR szProfileAppTextInputStyle[];
extern const TCHAR szProfileAppDialogTabStyle[];
extern const TCHAR szProfileAppDialogStyle[];
extern const TCHAR szProfileAppInfoBoxBorder[];
extern const TCHAR szProfileAppInfoBoxModel[]; // VENTA-ADDON MODEL CHANGE
extern const TCHAR szProfileAppAveNeedle[];
extern const TCHAR szProfileAutoAdvance[];
extern const TCHAR szProfileUTCOffset[];
extern const TCHAR szProfileBlockSTF[];
extern const TCHAR szProfileAutoZoom[];
extern const TCHAR szProfileMenuTimeout[];
extern const TCHAR szProfileTerrainContrast[];
extern const TCHAR szProfileTerrainBrightness[];
extern const TCHAR szProfileTerrainRamp[];
extern const TCHAR szProfileEnableFLARMMap[];
extern const TCHAR szProfileEnableFLARMGauge[];
extern const TCHAR szProfileAutoCloseFlarmDialog[];
extern const TCHAR szProfileEnableTAGauge[];
extern const TCHAR szProfileEnableThermalProfile[];
extern const TCHAR szProfileSnailTrail[];
extern const TCHAR szProfileTrailDrift[];
extern const TCHAR szProfileDetourCostMarker[];
extern const TCHAR szProfileDisplayTrackBearing[];
extern const TCHAR szProfileUnitsPresetName[];
extern const TCHAR szProfileGliderScreenPosition[];
extern const TCHAR szProfileSetSystemTimeFromGPS[];

extern const TCHAR szProfileVoiceClimbRate[];
extern const TCHAR szProfileVoiceTerrain[];
extern const TCHAR szProfileVoiceWaypointDistance[];
extern const TCHAR szProfileVoiceTaskAltitudeDifference[];
extern const TCHAR szProfileVoiceMacCready[];
extern const TCHAR szProfileVoiceNewWaypoint[];
extern const TCHAR szProfileVoiceInSector[];
extern const TCHAR szProfileVoiceAirspace[];

extern const TCHAR szProfileFinishMinHeight[];
extern const TCHAR szProfileStartMaxHeight[];
extern const TCHAR szProfileStartMaxHeightMargin[];
extern const TCHAR szProfileStartMaxSpeed[];
extern const TCHAR szProfileStartMaxSpeedMargin[];
extern const TCHAR szProfileStartHeightRef[];
extern const TCHAR szProfileStartType[];
extern const TCHAR szProfileStartRadius[];
extern const TCHAR szProfileTurnpointType[];
extern const TCHAR szProfileTurnpointRadius[];
extern const TCHAR szProfileFinishType[];
extern const TCHAR szProfileFinishRadius[];
extern const TCHAR szProfileTaskType[];
extern const TCHAR szProfileAATMinTime[];
extern const TCHAR szProfileAATTimeMargin[];

extern const TCHAR szProfileEnableNavBaroAltitude[];

extern const TCHAR szProfileLoggerTimeStepCruise[];
extern const TCHAR szProfileLoggerTimeStepCircling[];

extern const TCHAR szProfileSafetyMacCready[];
extern const TCHAR szProfileAbortTaskMode[];
extern const TCHAR szProfileAutoMcMode[];
extern const TCHAR szProfileAutoMc[];
extern const TCHAR szProfileEnableExternalTriggerCruise[];
extern const TCHAR szProfileOLCRules[];
extern const TCHAR szProfileHandicap[];
extern const TCHAR szProfileSnailWidthScale[];
extern const TCHAR szProfileSnailType[];
extern const TCHAR szProfileLatLonUnits[];
extern const TCHAR szProfileUserLevel[];
extern const TCHAR szProfileRiskGamma[];
extern const TCHAR szProfileWindArrowStyle[];
extern const TCHAR szProfileDisableAutoLogger[];
extern const TCHAR szProfileMapFile[];
extern const TCHAR szProfileBallastSecsToEmpty[];
extern const TCHAR szProfileAccelerometerZero[];
extern const TCHAR szProfileUseCustomFonts[];
extern const TCHAR szProfileFontInfoWindowFont[];
extern const TCHAR szProfileFontTitleWindowFont[];
extern const TCHAR szProfileFontMapWindowFont[];
extern const TCHAR szProfileFontTitleSmallWindowFont[];
extern const TCHAR szProfileFontMapWindowBoldFont[];
extern const TCHAR szProfileFontCDIWindowFont[];
extern const TCHAR szProfileFontMapLabelFont[];
extern const TCHAR szProfileFontMapLabelImportantFont[];
extern const TCHAR szProfileFontStatisticsFont[];
extern const TCHAR szProfileFontBugsBallastFont[];
extern const TCHAR szProfileFontAirspacePressFont[];
extern const TCHAR szProfileFontAirspaceColourDlgFont[];
extern const TCHAR szProfileFontTeamCodeFont[];

extern const TCHAR szProfileInfoBoxGeometry[];

extern const TCHAR szProfileFlarmSideData[];
extern const TCHAR szProfileFlarmAutoZoom[];
extern const TCHAR szProfileFlarmNorthUp[];

extern const TCHAR szProfileIgnoreNMEAChecksum[];
extern const TCHAR szProfileDisplayOrientation[];

extern const TCHAR szProfileClimbMapScale[];
extern const TCHAR szProfileCruiseMapScale[];

extern const TCHAR szProfileRoutePlannerMode[];
extern const TCHAR szProfileRoutePlannerAllowClimb[];
extern const TCHAR szProfileRoutePlannerUseCeiling[];
extern const TCHAR szProfileTurningReach[];
extern const TCHAR szProfileReachPolarMode[];

extern const TCHAR szProfileAircraftSymbol[];

extern const TCHAR szProfileFlarmLocation[];

#endif
