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

#ifndef XCSOAR_UTILS_PROFILE_HPP
#define XCSOAR_UTILS_PROFILE_HPP

#include <tchar.h>

#ifdef WIN32
#define CONF(key) _T(key)
#else /* !WIN32 */
#define CONF(key) ("/apps/XCSoar/" key)
#endif

#define REGKEYNAME  "Software\\MPSR\\XCSoar"

#define szProfileKey CONF(REGKEYNAME)

#define szProfileDisplayType(x) CONF("Info" "x")
#define szProfileColour(x) CONF("Colour" "x")
#define szProfileBrush(x) CONF("Brush" "x")
#define szProfileAirspaceMode(x) CONF("AirspaceMode" "x")
#define szProfileAirspacePriority(x) CONF("AirspacePriority" "x")

#define szProfileAirspaceWarning CONF("AirspaceWarn")
#define szProfileAirspaceBlackOutline CONF("AirspaceBlackOutline")
#define szProfileAltMargin CONF("AltMargin")
#define szProfileAltMode CONF("AltitudeMode")
#define szProfileAltitudeUnitsValue CONF("Altitude")
#define szProfileCircleZoom CONF("CircleZoom")
#define szProfileClipAlt CONF("ClipAlt")
#define szProfileDisplayText CONF("DisplayText")
#define szProfileDisplayUpValue CONF("DisplayUp")
#define szProfileDistanceUnitsValue CONF("Distance")
#define szProfileDrawTerrain CONF("DrawTerrain")
#define szProfileDrawTopology CONF("DrawTopology")
#define szProfileAutoWind CONF("AutoWind")
#define szProfileHomeWaypoint CONF("HomeWaypoint")
#define szProfileAlternate1 CONF("Alternate1") // VENTA3
#define szProfileAlternate2 CONF("Alternate2")
#define szProfileLiftUnitsValue CONF("Lift")
#define szProfileLatLonUnits CONF("LatLonUnits")
#define szProfilePolarID CONF("Polar") // pL
#define szProfileRegKey CONF("RegKey")
#define szProfileSafetyAltitudeArrival CONF("SafetyAltitudeArrival")
#define szProfileSafetyAltitudeTerrain CONF("SafetyAltitudeTerrain")
#define szProfileSafteySpeed CONF("SafteySpeed")
#define szProfileSnailTrail CONF("SnailTrail")
#define szProfileTrailDrift CONF("TrailDrift")
#define szProfileAnimation CONF("Animation")
#define szProfileSpeedUnitsValue CONF("Speed")
#define szProfileTaskSpeedUnitsValue CONF("TaskSpeed")
#define szProfileWarningTime CONF("WarnTime")
#define szProfileAcknowledgementTime CONF("AcknowledgementTime")
#define szProfileWindSpeed CONF("WindSpeed")
#define szProfileWindBearing CONF("WindBearing")
#define szProfileAirfieldFile CONF("AirfieldFile") // pL
#define szProfileAirspaceFile CONF("AirspaceFile") // pL
#define szProfileAdditionalAirspaceFile CONF("AdditionalAirspaceFile") // pL
#define szProfilePolarFile CONF("PolarFile") // pL
#define szProfileTerrainFile CONF("TerrainFile") // pL
#define szProfileTopologyFile CONF("TopologyFile") // pL
#define szProfileWayPointFile CONF("WPFile")
#define szProfileAdditionalWayPointFile CONF("AdditionalWPFile") // pL
#define szProfileLanguageFile CONF("LanguageFile") // pL
#define szProfileStatusFile CONF("StatusFile") // pL
#define szProfileInputFile CONF("InputFile") // pL
#define szProfilePilotName CONF("PilotName")
#define szProfileAircraftType CONF("AircraftType")
#define szProfileAircraftRego CONF("AircraftRego")
#define szProfileLoggerID CONF("LoggerID")
#define szProfileLoggerShort CONF("LoggerShortName")
#define szProfileSoundVolume CONF("SoundVolume")
#define szProfileSoundDeadband CONF("SoundDeadband")
#define szProfileSoundAudioVario CONF("AudioVario")
#define szProfileSoundTask CONF("SoundTask")
#define szProfileSoundModes CONF("SoundModes")
#define szProfileNettoSpeed CONF("NettoSpeed")
#define szProfileAccelerometerZero CONF("AccelerometerZero")
#define szProfileCDICruise CONF("CDICruise")
#define szProfileCDICircling CONF("CDICircling")

#define szProfileAutoBlank CONF("AutoBlank")
#define szProfileAutoBacklight CONF("AutoBacklight")
#define szProfileAutoSoundVolume CONF("AutoSoundVolume")
#define szProfileExtendedVisualGlide CONF("ExtVisualGlide")
#define szProfileVirtualKeys CONF("VirtualKeys")
#define szProfileAverEffTime CONF("AverEffTime")
#define szProfileVarioGauge CONF("VarioGauge")

#define szProfileDebounceTimeout CONF("DebounceTimeout")

#define szProfileAppIndFinalGlide CONF("AppIndFinalGlide")
#define szProfileAppIndLandable CONF("AppIndLandable")
#define szProfileAppInverseInfoBox CONF("AppInverseInfoBox")
#define szProfileAppGaugeVarioSpeedToFly CONF("AppGaugeVarioSpeedToFly")
#define szProfileAppGaugeVarioAvgText CONF("AppGaugeVarioAvgText")
#define szProfileAppGaugeVarioMc CONF("AppGaugeVarioMc")
#define szProfileAppGaugeVarioBugs CONF("AppGaugeVarioBugs")
#define szProfileAppGaugeVarioBallast CONF("AppGaugeVarioBallast")
#define szProfileAppGaugeVarioGross CONF("AppGaugeVarioGross")
#define szProfileAppCompassAppearance CONF("AppCompassAppearance")
#define szProfileAppStatusMessageAlignment CONF("AppStatusMessageAlignment")
#define szProfileAppTextInputStyle CONF("AppTextInputStyle")
#define szProfileAppDialogStyle CONF("AppDialogStyle")
#define szProfileAppInfoBoxColors CONF("AppInfoBoxColors")
#define szProfileAppDefaultMapWidth CONF("AppDefaultMapWidth")
#define szProfileTeamcodeRefWaypoint CONF("TeamcodeRefWaypoint")
#define szProfileAppInfoBoxBorder CONF("AppInfoBoxBorder")

#if defined(PNA) || defined(FIVV)
#define szProfileAppInfoBoxGeom CONF("AppInfoBoxGeom") // VENTA-ADDON GEOMETRY CONFIG
#define szProfileAppInfoBoxModel CONF("AppInfoBoxModel") // VENTA-ADDON MODEL CONFIG
#endif

#define szProfileAppAveNeedle CONF("AppAveNeedle")

#define szProfileAutoAdvance CONF("AutoAdvance")
#define szProfileUTCOffset CONF("UTCOffset")
#define szProfileBlockSTF CONF("BlockSpeedToFly")
#define szProfileAutoZoom CONF("AutoZoom")
#define szProfileMenuTimeout CONF("MenuTimeout")
#define szProfileLockSettingsInFlight CONF("LockSettingsInFlight")
#define szProfileTerrainContrast CONF("TerrainContrast")
#define szProfileTerrainBrightness CONF("TerrainBrightness")
#define szProfileTerrainRamp CONF("TerrainRamp")
#define szProfileEnableFLARMMap CONF("EnableFLARMDisplay")
#define szProfileEnableFLARMGauge CONF("EnableFLARMGauge")
#define szProfileFLARMGaugeBearing CONF("FLARMGaugeBearing")
#define szProfileGliderScreenPosition CONF("GliderScreenPosition")
#define szProfileSetSystemTimeFromGPS CONF("SetSystemTimeFromGPS")

#define szProfileVoiceClimbRate CONF("VoiceClimbRate")
#define szProfileVoiceTerrain CONF("VoiceTerrain")
#define szProfileVoiceWaypointDistance CONF("VoiceWaypointDistance")
#define szProfileVoiceTaskAltitudeDifference CONF("VoiceTaskAltitudeDifference")
#define szProfileVoiceMacCready CONF("VoiceMacCready")
#define szProfileVoiceNewWaypoint CONF("VoiceNewWaypoint")
#define szProfileVoiceInSector CONF("VoiceInSector")
#define szProfileVoiceAirspace CONF("VoiceAirspace")

#define szProfileFinishMinHeight CONF("FinishMinHeight")
#define szProfileStartMaxHeight CONF("StartMaxHeight")
#define szProfileStartMaxSpeed CONF("StartMaxSpeed")
#define szProfileStartMaxHeightMargin CONF("StartMaxHeightMargin")
#define szProfileStartMaxSpeedMargin CONF("StartMaxSpeedMargin")
#define szProfileStartHeightRef CONF("StartHeightRef")
#define szProfileEnableNavBaroAltitude CONF("EnableNavBaroAltitude")

#define szProfileLoggerTimeStepCruise CONF("LoggerTimeStepCruise")
#define szProfileLoggerTimeStepCircling CONF("LoggerTimeStepCircling")

#define szProfileSafetyMacCready CONF("SafetyMacCready")
#define szProfileAbortSafetyUseCurrent CONF("AbortSafetyUseCurrent")
#define szProfileAutoMcMode CONF("AutoMcMode")
#define szProfileWaypointsOutOfRange CONF("WaypointsOutOfRange")
#define szProfileEnableExternalTriggerCruise CONF("EnableExternalTriggerCruise")
#define szProfileOLCRules CONF("OLCRules")
#define szProfileHandicap CONF("Handicap")
#define szProfileSnailWidthScale CONF("SnailWidthScale")
#define szProfileUserLevel CONF("UserLevel")
#define szProfileRiskGamma CONF("RiskGamma")
#define szProfileWindArrowStyle CONF("WindArrowStyle")
#define szProfileDisableAutoLogger CONF("DisableAutoLogger")
#define szProfileMapFile CONF("MapFile") // pL
#define szProfileBallastSecsToEmpty CONF("BallastSecsToEmpty")
#define szProfileUseCustomFonts CONF("UseCustomFonts")
#define szProfileFontInfoWindowFont CONF("InfoWindowFont")
#define szProfileFontTitleWindowFont CONF("TitleWindowFont")
#define szProfileFontMapWindowFont CONF("MapWindowFont")
#define szProfileFontTitleSmallWindowFont CONF("TeamCodeFont")
#define szProfileFontMapWindowBoldFont CONF("MapWindowBoldFont")
#define szProfileFontCDIWindowFont CONF("CDIWindowFont")
#define szProfileFontMapLabelFont CONF("MapLabelFont")
#define szProfileFontStatisticsFont CONF("StatisticsFont")

#endif
