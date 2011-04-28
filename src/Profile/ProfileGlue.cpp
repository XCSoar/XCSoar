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

#include "Profile/Profile.hpp"
#include "Profile/UnitsConfig.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "LogFile.hpp"
#include "Appearance.hpp"
#include "GlideRatio.hpp"
#include "Screen/Fonts.hpp"
#include "Dialogs/XML.hpp"
#include "Interface.hpp"
#include "Task/TaskBehaviour.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

#include <assert.h>
#include <stdio.h>

void
Profile::Use()
{
  unsigned Temp = 0;

  LogStartUp(_T("Read registry settings"));

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

  unsigned sdtemp = 0;
  Get(szProfileStartType, sdtemp);
  settings_task.sector_defaults.start_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;
  Get(szProfileStartRadius,
      settings_task.sector_defaults.start_radius);

  Get(szProfileTurnpointType, sdtemp);
      settings_task.sector_defaults.turnpoint_type =
          (AbstractTaskFactory::LegalPointType_t)sdtemp;
  Get(szProfileTurnpointRadius,
      settings_task.sector_defaults.turnpoint_radius);

  Get(szProfileFinishType, sdtemp);
      settings_task.sector_defaults.finish_type =
          (AbstractTaskFactory::LegalPointType_t)sdtemp;
  Get(szProfileFinishRadius,
      settings_task.sector_defaults.finish_radius);

  Get(szProfileTaskType, sdtemp);
      settings_task.task_type_default =
          (TaskBehaviour::Factory_t)sdtemp;

  Get(szProfileAATMinTime,
      osettings_task.aat_min_time);

  Get(szProfileAATTimeMargin,
      settings_task.optimise_targets_margin);

  LoadUnits();
  Profile::GetInfoBoxManagerConfig(infoBoxManagerConfig);

  SETTINGS_MAP &settings_map = XCSoarInterface::SetSettingsMap();

  bool orientation_found = false;

  Temp = NORTHUP;
  if (Get(szProfileOrientationCircling, Temp))
    orientation_found = true;

  switch (Temp) {
  case TRACKUP:
    settings_map.OrientationCircling = TRACKUP;
    break;
  case NORTHUP:
    settings_map.OrientationCircling = NORTHUP;
    break;
  case TARGETUP:
    settings_map.OrientationCircling = TARGETUP;
    break;
  }

  Temp = NORTHUP;
  if (Get(szProfileOrientationCruise, Temp))
    orientation_found = true;

  switch (Temp) {
  case TRACKUP:
    settings_map.OrientationCruise = TRACKUP;
    break;
  case NORTHUP:
    settings_map.OrientationCruise = NORTHUP;
    break;
  case TARGETUP:
    settings_map.OrientationCruise = TARGETUP;
    break;
  }

  if (!orientation_found) {
    Temp = 1;
    Get(szProfileDisplayUpValue, Temp);
    switch (Temp) {
    case 0:
      settings_map.OrientationCruise = TRACKUP;
      settings_map.OrientationCircling = TRACKUP;
      break;
    case 1:
      settings_map.OrientationCruise = NORTHUP;
      settings_map.OrientationCircling = NORTHUP;
      break;
    case 2:
      settings_map.OrientationCruise = TRACKUP;
      settings_map.OrientationCircling = NORTHUP;
      break;
    case 3:
      settings_map.OrientationCruise = TRACKUP;
      settings_map.OrientationCircling = TARGETUP;
      break;
    case 4:
      settings_map.OrientationCruise = NORTHUP;
      settings_map.OrientationCircling = TRACKUP;
      break;
    }
  }

  GetEnum(szProfileMapShiftBias, settings_map.MapShiftBias);

  // NOTE: settings_map.WayPointLabelSelection must be loaded after this code
  GetEnum(szProfileDisplayText, settings_map.DisplayTextType);
  if (settings_map.DisplayTextType == OBSOLETE_DONT_USE_DISPLAYNAMEIFINTASK) {
    // pref migration. The migrated value of DisplayTextType and
    // WayPointLabelSelection will not be written to the config file
    // unless the user explicitly changes the corresponding setting manually.
    // This requires ordering because a manually changed WayPointLabelSelection
    // may be overwritten by the following migration code.
    settings_map.DisplayTextType = DISPLAYNAME;
    settings_map.WayPointLabelSelection = wlsTaskWayPoints;
  }
  else if (settings_map.DisplayTextType == OBSOLETE_DONT_USE_DISPLAYNUMBER)
    settings_map.DisplayTextType = DISPLAYNAME;


  // NOTE: settings_map.DisplayTextType must be loaded before this code
  //       due to pref migration dependencies!
  GetEnum(szProfileWayPointLabelSelection,
          settings_map.WayPointLabelSelection);

  GetEnum(szProfileWaypointArrivalHeightDisplay,
          settings_map.WaypointArrivalHeightDisplay);

  GetEnum(szProfileWaypointLabelStyle, settings_map.LandableRenderMode);

  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();

  GetEnum(szProfileAltMode, settings_computer.AltitudeMode);

  Get(szProfileClipAlt,
      settings_computer.ClipAltitude);
  Get(szProfileAltMargin,
      settings_computer.airspace_warnings.AltWarningMargin);

  Get(szProfileSafetyAltitudeArrival,
      settings_computer.safety_height_arrival);
  Get(szProfileSafetyAltitudeTerrain,
      settings_computer.route_planner.safety_height_terrain);

  LoadAirspaceConfig();

  Get(szProfileAirspaceBlackOutline,
      settings_map.bAirspaceBlackOutline);

#ifndef ENABLE_OPENGL
  Get(szProfileAirspaceTransparency, settings_map.airspace_transparency);
  GetEnum(szProfileAirspaceFillMode, settings_map.AirspaceFillMode);
#endif

  Get(szProfileSnailTrail,
      settings_map.TrailActive);

  Get(szProfileTrailDrift,
      settings_map.EnableTrailDrift);

  Get(szProfileDetourCostMarker,
      settings_map.EnableDetourCostMarker);

  GetEnum(szProfileDisplayTrackBearing, settings_map.DisplayTrackBearing);

  Get(szProfileDrawTopography,
      settings_map.EnableTopography);

  Get(szProfileDrawTerrain,
      settings_map.EnableTerrain);

  Temp = settings_map.SlopeShadingType;
  if (!Get(szProfileSlopeShadingType, Temp)) {
    bool old_profile_setting = true;
    if (Get(szProfileSlopeShading, old_profile_setting))
      // 0: OFF, 3: Wind
      Temp = old_profile_setting ? 3 : 0;
  }
  settings_map.SlopeShadingType = (SlopeShadingType_t)Temp;

  GetEnum(szProfileFinalGlideTerrain,
          settings_computer.FinalGlideTerrain);

  Get(szProfileAutoWind,
      settings_computer.AutoWindMode);

  Get(szProfileExternalWind,
      settings_computer.ExternalWind);

  Get(szProfileCircleZoom,
      settings_map.CircleZoom);

  Get(szProfileMaxAutoZoomDistance,
      settings_map.MaxAutoZoomDistance);

  Get(szProfileHomeWaypoint,
      settings_computer.HomeWaypoint);

  settings_computer.HomeLocationAvailable =
    GetGeoPoint(szProfileHomeLocation, settings_computer.HomeLocation);

  Get(szProfileSnailWidthScale,
      settings_map.SnailScaling);

  GetEnum(szProfileSnailType, settings_map.SnailType);

  Get(szProfileTeamcodeRefWaypoint,
      settings_computer.TeamCodeRefWaypoint);

  Get(szProfileAirspaceWarning,
      settings_computer.EnableAirspaceWarnings);

  Get(szProfileWarningTime,
      settings_computer.airspace_warnings.WarningTime);

  Get(szProfileAcknowledgementTime,
      settings_computer.airspace_warnings.AcknowledgementTime);

  Get(szProfileSoundVolume,
      settings_computer.SoundVolume);

  Get(szProfileSoundDeadband,
      settings_computer.SoundDeadband);

  Get(szProfileSoundAudioVario,
      settings_computer.EnableSoundVario);

  Get(szProfileSoundTask,
      settings_computer.EnableSoundTask);

  Get(szProfileSoundModes,
      settings_computer.EnableSoundModes);

#ifdef HAVE_BLANK
  Get(szProfileAutoBlank,
      settings_map.EnableAutoBlank);
#endif

  if (Get(szProfileAverEffTime, Temp))
    settings_computer.AverEffTime = Temp;

  Get(szProfileDebounceTimeout, XCSoarInterface::debounceTimeout);

  /* JMW broken
  if (Get(szProfileAccelerometerZero, Temp))
    AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    Set(szProfileAccelerometerZero, Temp);
  }
  */

  // new appearance variables
  GetEnum(szProfileAppIndLandable, Appearance.IndLandable);

  Get(szProfileAppUseSWLandablesRendering,
      Appearance.UseSWLandablesRendering);

  Get(szProfileAppLandableRenderingScale,
      Appearance.LandableRenderingScale);

  Get(szProfileAppScaleRunwayLength,
      Appearance.ScaleRunwayLength);
  
  Get(szProfileAppInverseInfoBox,
		  Appearance.InverseInfoBox);

  GetEnum(szProfileAppInfoBoxBorder, Appearance.InfoBoxBorder);
  GetEnum(szProfileAppStatusMessageAlignment, Appearance.StateMessageAlign);
  GetEnum(szProfileAppTextInputStyle, Appearance.TextInputStyle);
  GetEnum(szProfileAppDialogTabStyle, Appearance.DialogTabStyle);
  GetEnum(szProfileAppDialogStyle, DialogStyleSetting);

  Get(szProfileAppInfoBoxColors,
		  Appearance.InfoBoxColors);

  Get(szProfileAutoMc, settings_computer.auto_mc);
  GetEnum(szProfileAutoMcMode, settings_computer.auto_mc_mode);

  if (GetEnum(szProfileOLCRules, settings_computer.contest))
  {
      if ( settings_computer.contest == OLC_Sprint )
        settings_computer.contest = OLC_League; // Handle out-dated Sprint rule in profile
  }

  Get(szProfileHandicap,
      settings_computer.contest_handicap);
  Get(szProfileEnableExternalTriggerCruise,
      settings_computer.EnableExternalTriggerCruise);

  Get(szProfileUTCOffset,
      settings_computer.UTCOffset);
  if (settings_computer.UTCOffset > 12 * 3600)
    settings_computer.UTCOffset -= 24 * 3600;

  Get(szProfileBlockSTF,
      settings_computer.EnableBlockSTF);
  Get(szProfileAutoZoom,
      settings_map.AutoZoom);
  Get(szProfileMenuTimeout,
      XCSoarInterface::MenuTimeoutMax);
  Get(szProfileLoggerShort,
      settings_computer.LoggerShortName);
  Get(szProfileEnableFLARMMap,
      settings_map.EnableFLARMMap);
  Get(szProfileEnableFLARMGauge,
      settings_map.EnableFLARMGauge);
  Get(szProfileAutoCloseFlarmDialog,
      settings_map.AutoCloseFlarmDialog);
  Get(szProfileEnableTAGauge,
      settings_map.EnableTAGauge);
  Get(szProfileTerrainContrast,
      settings_map.TerrainContrast);
  Get(szProfileTerrainBrightness,
      settings_map.TerrainBrightness);
  Get(szProfileTerrainRamp,
      settings_map.TerrainRamp);

  Get(szProfileGliderScreenPosition,
      settings_map.GliderScreenPosition);
  Get(szProfileBallastSecsToEmpty,
      settings_computer.BallastSecsToEmpty);
  Get(szProfileSetSystemTimeFromGPS,
      settings_map.SetSystemTimeFromGPS);
  Get(szProfileUseCustomFonts,
      Appearance.UseCustomFonts);
  Get(szProfileVoiceClimbRate,
      settings_computer.EnableVoiceClimbRate);
  Get(szProfileVoiceTerrain,
      settings_computer.EnableVoiceTerrain);
  Get(szProfileVoiceWaypointDistance,
      settings_computer.EnableVoiceWaypointDistance);
  Get(szProfileVoiceTaskAltitudeDifference,
      settings_computer.EnableVoiceTaskAltitudeDifference);
  Get(szProfileVoiceMacCready,
      settings_computer.EnableVoiceMacCready);
  Get(szProfileVoiceNewWaypoint,
      settings_computer.EnableVoiceNewWaypoint);
  Get(szProfileVoiceInSector,
      settings_computer.EnableVoiceInSector);
  Get(szProfileVoiceAirspace,
      settings_computer.EnableVoiceAirspace);
  Get(szProfileEnableNavBaroAltitude,
      settings_computer.EnableNavBaroAltitude);
  Get(szProfileLoggerTimeStepCruise,
      settings_computer.LoggerTimeStepCruise);
  Get(szProfileLoggerTimeStepCircling,
      settings_computer.LoggerTimeStepCircling);

  if (Get(szProfileSafetyMacCready, Temp))
    settings_computer.safety_mc = fixed(Temp) / 10;

  GetEnum(szProfileAbortTaskMode, settings_computer.abort_task_mode);

  GetEnum(szProfileRoutePlannerMode, settings_computer.route_planner.mode);

  Get(szProfileRoutePlannerAllowClimb,
      settings_computer.route_planner.allow_climb);

  Get(szProfileRoutePlannerUseCeiling,
      settings_computer.route_planner.use_ceiling);

  GetEnum(szProfileTurningReach,
          settings_computer.route_planner.reach_calc_mode);

  GetEnum(szProfileReachPolarMode,
          settings_computer.route_planner.reach_polar_mode);

  if (Get(szProfileRiskGamma, Temp))
    settings_computer.risk_gamma = fixed(Temp) / 10;

  if (Get(szProfileWindArrowStyle, Temp))
    settings_map.WindArrowStyle = Temp;

  Get(szProfileDisableAutoLogger,
      settings_computer.DisableAutoLogger);

  GetEnum(szProfileAircraftSymbol, Appearance.AircraftSymbol);
}

void
Profile::SetSoundSettings()
{
  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();

  Set(szProfileSoundVolume,
      settings_computer.SoundVolume);
  Set(szProfileSoundDeadband,
      settings_computer.SoundDeadband);
  Set(szProfileSoundAudioVario,
      settings_computer.EnableSoundVario);
  Set(szProfileSoundTask,
      settings_computer.EnableSoundTask);
  Set(szProfileSoundModes,
      settings_computer.EnableSoundModes);
}
