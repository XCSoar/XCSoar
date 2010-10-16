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

#include "Sizes.h"
#include "Defines.h"
#include "Profile/ProfileKeys.hpp"

const TCHAR *szProfileDisplayType[MAXINFOWINDOWS] = {
  CONF("Info0"),
  CONF("Info1"),
  CONF("Info2"),
  CONF("Info3"),
  CONF("Info4"),
  CONF("Info5"),
  CONF("Info6"),
  CONF("Info7"),
  CONF("Info8"),
  CONF("Info9"),
  CONF("Info10"),
  CONF("Info11"),
  CONF("Info12"),
  CONF("Info13"),
}; // pL

const TCHAR *szProfileColour[] = {
  CONF("Colour0"),
  CONF("Colour1"),
  CONF("Colour2"),
  CONF("Colour3"),
  CONF("Colour4"),
  CONF("Colour5"),
  CONF("Colour6"),
  CONF("Colour7"),
  CONF("Colour8"),
  CONF("Colour9"),
  CONF("Colour10"),
  CONF("Colour11"),
  CONF("Colour12"),
  CONF("Colour13"),
  CONF("Colour14"),
}; // pL

const TCHAR *szProfileBrush[] = {
  CONF("Brush0"),
  CONF("Brush1"),
  CONF("Brush2"),
  CONF("Brush3"),
  CONF("Brush4"),
  CONF("Brush5"),
  CONF("Brush6"),
  CONF("Brush7"),
  CONF("Brush8"),
  CONF("Brush9"),
  CONF("Brush10"),
  CONF("Brush11"),
  CONF("Brush12"),
  CONF("Brush13"),
  CONF("Brush14"),
}; // pL

const TCHAR *szProfileAirspaceMode[] = {
  CONF("AirspaceMode0"),
  CONF("AirspaceMode1"),
  CONF("AirspaceMode2"),
  CONF("AirspaceMode3"),
  CONF("AirspaceMode4"),
  CONF("AirspaceMode5"),
  CONF("AirspaceMode6"),
  CONF("AirspaceMode7"),
  CONF("AirspaceMode8"),
  CONF("AirspaceMode9"),
  CONF("AirspaceMode10"),
  CONF("AirspaceMode11"),
  CONF("AirspaceMode12"),
  CONF("AirspaceMode13"),
  CONF("AirspaceMode14"),
}; // pL

const TCHAR *szProfileAirspacePriority[] = {
  CONF("AirspacePriority0"),
  CONF("AirspacePriority1"),
  CONF("AirspacePriority2"),
  CONF("AirspacePriority3"),
  CONF("AirspacePriority4"),
  CONF("AirspacePriority5"),
  CONF("AirspacePriority6"),
  CONF("AirspacePriority7"),
  CONF("AirspacePriority8"),
  CONF("AirspacePriority9"),
  CONF("AirspacePriority10"),
  CONF("AirspacePriority11"),
  CONF("AirspacePriority12"),
  CONF("AirspacePriority13"),
  CONF("AirspacePriority14"),
}; // pL

const TCHAR szProfileAirspaceWarning[] = CONF("AirspaceWarn");
const TCHAR szProfileAirspaceBlackOutline[] = CONF("AirspaceBlackOutline");
const TCHAR szProfileAltMargin[] = CONF("AltMargin");
const TCHAR szProfileAltMode[] = CONF("AltitudeMode");
const TCHAR szProfileAltitudeUnitsValue[] = CONF("Altitude");
const TCHAR szProfileTemperatureUnitsValue[] = CONF("Temperature");
const TCHAR szProfileCircleZoom[] = CONF("CircleZoom");
const TCHAR szProfileClipAlt[] = CONF("ClipAlt");
const TCHAR szProfileDisplayText[] = CONF("DisplayText");
const TCHAR szProfileDisplayUpValue[] = CONF("DisplayUp");
const TCHAR szProfileDistanceUnitsValue[] = CONF("Distance");
const TCHAR szProfileDrawTerrain[] = CONF("DrawTerrain");
const TCHAR szProfileDrawTopology[] = CONF("DrawTopology");
const TCHAR szProfileFinalGlideTerrain[] = CONF("FinalGlideTerrain");
const TCHAR szProfileAutoWind[] = CONF("AutoWind");
const TCHAR szProfileHomeWaypoint[] = CONF("HomeWaypoint");
const TCHAR szProfileAlternate1[] = CONF("Alternate1"); // VENTA3
const TCHAR szProfileAlternate2[] = CONF("Alternate2");
const TCHAR szProfileLiftUnitsValue[] = CONF("Lift");
const TCHAR szProfileLatLonUnits[] = CONF("LatLonUnits");
const TCHAR szProfilePolarID[] = CONF("Polar"); // pL
const TCHAR szProfileSafetyAltitudeArrival[] = CONF("SafetyAltitudeArrival");
const TCHAR szProfileSafetyAltitudeTerrain[] = CONF("SafetyAltitudeTerrain");
const TCHAR szProfileSafteySpeed[] = CONF("SafteySpeed");
const TCHAR szProfileSnailTrail[] = CONF("SnailTrail");
const TCHAR szProfileTrailDrift[] = CONF("TrailDrift");
const TCHAR szProfileDetourCostMarker[] = CONF("DetourCostMarker");
const TCHAR szProfileSpeedUnitsValue[] = CONF("Speed");
const TCHAR szProfileTaskSpeedUnitsValue[] = CONF("TaskSpeed");
const TCHAR szProfileWarningTime[] = CONF("WarnTime");
const TCHAR szProfileAcknowledgementTime[] = CONF("AcknowledgementTime");
const TCHAR szProfileAirfieldFile[] = CONF("AirfieldFile"); // pL
const TCHAR szProfileAirspaceFile[] = CONF("AirspaceFile"); // pL
const TCHAR szProfileAdditionalAirspaceFile[] = CONF("AdditionalAirspaceFile"); // pL
const TCHAR szProfilePolarFile[] = CONF("PolarFile"); // pL
const TCHAR szProfileTerrainFile[] = CONF("TerrainFile"); // pL
const TCHAR szProfileTopologyFile[] = CONF("TopologyFile"); // pL
const TCHAR szProfileWayPointFile[] = CONF("WPFile"); // pL
const TCHAR szProfileAdditionalWayPointFile[] = CONF("AdditionalWPFile"); // pL
const TCHAR szProfileLanguageFile[] = CONF("LanguageFile"); // pL
const TCHAR szProfileStatusFile[] = CONF("StatusFile"); // pL
const TCHAR szProfileInputFile[] = CONF("InputFile"); // pL
const TCHAR szProfilePilotName[] = CONF("PilotName");
const TCHAR szProfileAircraftType[] = CONF("AircraftType");
const TCHAR szProfileAircraftRego[] = CONF("AircraftRego");
const TCHAR szProfileLoggerID[] = CONF("LoggerID");
const TCHAR szProfileLoggerShort[] = CONF("LoggerShortName");
const TCHAR szProfileSoundVolume[] = CONF("SoundVolume");
const TCHAR szProfileSoundDeadband[] = CONF("SoundDeadband");
const TCHAR szProfileSoundAudioVario[] = CONF("AudioVario");
const TCHAR szProfileSoundTask[] = CONF("SoundTask");
const TCHAR szProfileSoundModes[] = CONF("SoundModes");
const TCHAR szProfileNettoSpeed[] = CONF("NettoSpeed");
const TCHAR szProfileAccelerometerZero[] = CONF("AccelerometerZero");
const TCHAR szProfileCDICruise[] = CONF("CDICruise");
const TCHAR szProfileCDICircling[] = CONF("CDICircling");

const TCHAR szProfileAutoBlank[] = CONF("AutoBlank");
const TCHAR szProfileAutoBacklight[] = CONF("AutoBacklight");
const TCHAR szProfileAutoSoundVolume[] = CONF("AutoSoundVolume");
const TCHAR szProfileGestures[] = CONF("Gestures");
const TCHAR szProfileAverEffTime[] = CONF("AverEffTime");
const TCHAR szProfileVarioGauge[] = CONF("VarioGauge");

const TCHAR szProfileDebounceTimeout[] = CONF("DebounceTimeout");

const TCHAR szProfileAppIndFinalGlide[] = CONF("AppIndFinalGlide");
const TCHAR szProfileAppIndLandable[] = CONF("AppIndLandable");
const TCHAR szProfileAppInverseInfoBox[] = CONF("AppInverseInfoBox");
const TCHAR szProfileAppGaugeVarioSpeedToFly[] = CONF("AppGaugeVarioSpeedToFly");
const TCHAR szProfileAppGaugeVarioAvgText[] = CONF("AppGaugeVarioAvgText");
const TCHAR szProfileAppGaugeVarioMc[] = CONF("AppGaugeVarioMc");
const TCHAR szProfileAppGaugeVarioBugs[] = CONF("AppGaugeVarioBugs");
const TCHAR szProfileAppGaugeVarioBallast[] = CONF("AppGaugeVarioBallast");
const TCHAR szProfileAppGaugeVarioGross[] = CONF("AppGaugeVarioGross");
const TCHAR szProfileAppCompassAppearance[] = CONF("AppCompassAppearance");
const TCHAR szProfileAppStatusMessageAlignment[] = CONF("AppStatusMessageAlignment");
const TCHAR szProfileAppTextInputStyle[] = CONF("AppTextInputStyle");
const TCHAR szProfileAppDialogStyle[] = CONF("AppDialogStyle");
const TCHAR szProfileAppInfoBoxColors[] = CONF("AppInfoBoxColors");
const TCHAR szProfileAppDefaultMapWidth[] = CONF("AppDefaultMapWidth");
const TCHAR szProfileTeamcodeRefWaypoint[] = CONF("TeamcodeRefWaypoint");
const TCHAR szProfileAppInfoBoxBorder[] = CONF("AppInfoBoxBorder");

const TCHAR szProfileAppInfoBoxModel[] = CONF("AppInfoBoxModel"); // VENTA-ADDON MODEL CONFIG

const TCHAR szProfileAppAveNeedle[] = CONF("AppAveNeedle");

const TCHAR szProfileAutoAdvance[] = CONF("AutoAdvance");
const TCHAR szProfileUTCOffset[] = CONF("UTCOffset");
const TCHAR szProfileBlockSTF[] = CONF("BlockSpeedToFly");
const TCHAR szProfileAutoZoom[] = CONF("AutoZoom");
const TCHAR szProfileMenuTimeout[] = CONF("MenuTimeout");
const TCHAR szProfileLockSettingsInFlight[] = CONF("LockSettingsInFlight");
const TCHAR szProfileTerrainContrast[] = CONF("TerrainContrast");
const TCHAR szProfileTerrainBrightness[] = CONF("TerrainBrightness");
const TCHAR szProfileTerrainRamp[] = CONF("TerrainRamp");
const TCHAR szProfileEnableFLARMMap[] = CONF("EnableFLARMDisplay");
const TCHAR szProfileEnableFLARMGauge[] = CONF("EnableFLARMGauge");
const TCHAR szProfileEnableTAGauge[] = CONF("EnableTAGauge");
const TCHAR szProfileFLARMGaugeBearing[] = CONF("FLARMGaugeBearing");
const TCHAR szProfileGliderScreenPosition[] = CONF("GliderScreenPosition");
const TCHAR szProfileSetSystemTimeFromGPS[] = CONF("SetSystemTimeFromGPS");

const TCHAR szProfileVoiceClimbRate[] = CONF("VoiceClimbRate");
const TCHAR szProfileVoiceTerrain[] = CONF("VoiceTerrain");
const TCHAR szProfileVoiceWaypointDistance[] = CONF("VoiceWaypointDistance");
const TCHAR szProfileVoiceTaskAltitudeDifference[] = CONF("VoiceTaskAltitudeDifference");
const TCHAR szProfileVoiceMacCready[] = CONF("VoiceMacCready");
const TCHAR szProfileVoiceNewWaypoint[] = CONF("VoiceNewWaypoint");
const TCHAR szProfileVoiceInSector[] = CONF("VoiceInSector");
const TCHAR szProfileVoiceAirspace[] = CONF("VoiceAirspace");

const TCHAR szProfileFinishMinHeight[] = CONF("FinishMinHeight");
const TCHAR szProfileStartMaxHeight[] = CONF("StartMaxHeight");
const TCHAR szProfileStartMaxSpeed[] = CONF("StartMaxSpeed");
const TCHAR szProfileStartMaxHeightMargin[] = CONF("StartMaxHeightMargin");
const TCHAR szProfileStartMaxSpeedMargin[] = CONF("StartMaxSpeedMargin");
const TCHAR szProfileStartHeightRef[] = CONF("StartHeightRef");
const TCHAR szProfileEnableNavBaroAltitude[] = CONF("EnableNavBaroAltitude");

const TCHAR szProfileLoggerTimeStepCruise[] = CONF("LoggerTimeStepCruise");
const TCHAR szProfileLoggerTimeStepCircling[] = CONF("LoggerTimeStepCircling");

const TCHAR szProfileSafetyMacCready[] = CONF("SafetyMacCready");
const TCHAR szProfileAbortSafetyUseCurrent[] = CONF("AbortSafetyUseCurrent");
const TCHAR szProfileAutoMcMode[] = CONF("AutoMcMode");
const TCHAR szProfileEnableExternalTriggerCruise[] = CONF("EnableExternalTriggerCruise");
const TCHAR szProfileOLCRules[] = CONF("OLCRules");
const TCHAR szProfileHandicap[] = CONF("Handicap");
const TCHAR szProfileSnailWidthScale[] = CONF("SnailWidthScale");
const TCHAR szProfileUserLevel[] = CONF("UserLevel");
const TCHAR szProfileRiskGamma[] = CONF("RiskGamma");
const TCHAR szProfileWindArrowStyle[] = CONF("WindArrowStyle");
const TCHAR szProfileDisableAutoLogger[] = CONF("DisableAutoLogger");
const TCHAR szProfileMapFile[] = CONF("MapFile"); // pL
const TCHAR szProfileBallastSecsToEmpty[] = CONF("BallastSecsToEmpty");
const TCHAR szProfileUseCustomFonts[] = CONF("UseCustomFonts");
const TCHAR szProfileFontInfoWindowFont[] = CONF("InfoWindowFont");
const TCHAR szProfileFontTitleWindowFont[] = CONF("TitleWindowFont");
const TCHAR szProfileFontMapWindowFont[] = CONF("MapWindowFont");
const TCHAR szProfileFontTitleSmallWindowFont[] = CONF("TeamCodeFont");
const TCHAR szProfileFontMapWindowBoldFont[] = CONF("MapWindowBoldFont");
const TCHAR szProfileFontCDIWindowFont[] = CONF("CDIWindowFont");
const TCHAR szProfileFontMapLabelFont[] = CONF("MapLabelFont");
const TCHAR szProfileFontStatisticsFont[] = CONF("StatisticsFont");
const TCHAR szProfileFontBugsBallastFont[] = CONF("BugsBallastFont");
const TCHAR szProfileFontAirspacePressFont[] = CONF("AirspacePressFont");
const TCHAR szProfileFontAirspaceColourDlgFont[] = CONF("AirspaceColourDlgFont");
const TCHAR szProfileFontTeamCodeFont[] = CONF("TeamCodeFont");

const TCHAR szProfileInfoBoxGeometry[] = CONF("InfoBoxGeometry");

const TCHAR szProfileFlarmSideData[] = CONF("FlarmRadarSideData");
const TCHAR szProfileFlarmAutoZoom[] = CONF("FlarmRadarAutoZoom");
const TCHAR szProfileFlarmNorthUp[] = CONF("FlarmRadarNorthUp");

const TCHAR szProfileIgnoreNMEAChecksum[] = CONF("IgnoreNMEAChecksum");
const TCHAR szProfileDisplayOrientation[] = CONF("DisplayOrientation");
