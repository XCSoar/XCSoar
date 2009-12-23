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

#ifndef XCSOAR_REGISTRY_HPP
#define XCSOAR_REGISTRY_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tchar.h>

void ReadPort1Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void ReadPort2Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void ReadPort3Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void WritePort1Settings(DWORD PortIndex, DWORD SpeedIndex);
void WritePort2Settings(DWORD PortIndex, DWORD SpeedIndex);
void WritePort3Settings(DWORD PortIndex, DWORD SpeedIndex);

extern const TCHAR szRegistryKey[];
extern const TCHAR *szRegistryDisplayType[];
extern const TCHAR *szRegistryColour[];
extern const TCHAR *szRegistryBrush[];
extern const TCHAR *szRegistryAirspaceMode[];
extern const TCHAR szRegistrySpeedUnitsValue[];
extern const TCHAR szRegistryDistanceUnitsValue[];
extern const TCHAR szRegistryAltitudeUnitsValue[];
extern const TCHAR szRegistryLiftUnitsValue[];
extern const TCHAR szRegistryTaskSpeedUnitsValue[];
extern const TCHAR szRegistryDisplayUpValue[];
extern const TCHAR szRegistryDisplayText[];
extern const TCHAR szRegistrySafetyAltitudeArrival[];
extern const TCHAR szRegistrySafetyAltitudeBreakOff[];
extern const TCHAR szRegistrySafetyAltitudeTerrain[];
extern const TCHAR szRegistrySafteySpeed[];
extern const TCHAR szRegistryFAISector[];
extern const TCHAR szRegistrySectorRadius[];
extern const TCHAR szRegistryPolarID[];
extern const TCHAR szRegistryWayPointFile[];
extern const TCHAR szRegistryAdditionalWayPointFile[];
extern const TCHAR szRegistryAirspaceFile[];
extern const TCHAR szRegistryAdditionalAirspaceFile[];
extern const TCHAR szRegistryWindSpeed[];
extern const TCHAR szRegistryWindBearing[];
extern const TCHAR szRegistryAirfieldFile[];
extern const TCHAR szRegistryTopologyFile[];
extern const TCHAR szRegistryPolarFile[];
extern const TCHAR szRegistryTerrainFile[];
extern const TCHAR szRegistryLanguageFile[];
extern const TCHAR szRegistryStatusFile[];
extern const TCHAR szRegistryInputFile[];
extern const TCHAR szRegistryAltMode[];
extern const TCHAR szRegistryClipAlt[];
extern const TCHAR szRegistryAltMargin[];
extern const TCHAR szRegistryRegKey[];
extern const TCHAR szRegistrySnailTrail[];
extern const TCHAR szRegistryDrawTopology[];
extern const TCHAR szRegistryDrawTerrain[];
extern const TCHAR szRegistryFinalGlideTerrain[];
extern const TCHAR szRegistryAutoWind[];
extern const TCHAR szRegistryStartLine[];
extern const TCHAR szRegistryStartRadius[];
extern const TCHAR szRegistryFinishLine[];
extern const TCHAR szRegistryFinishRadius[];
extern const TCHAR *szRegistryAirspacePriority[];
extern const TCHAR szRegistryAirspaceWarning[];
extern const TCHAR szRegistryAirspaceBlackOutline[];
extern const TCHAR szRegistryWarningTime[];
extern const TCHAR szRegistryAcknowledgementTime[];
extern const TCHAR szRegistryCircleZoom[];
extern const TCHAR szRegistryHomeWaypoint[];
extern const TCHAR szRegistryAlternate1[];         // VENTA3
extern const TCHAR szRegistryAlternate2[];
extern const TCHAR szRegistryTeamcodeRefWaypoint[];
extern const TCHAR szRegistryPilotName[];
extern const TCHAR szRegistryAircraftType[];
extern const TCHAR szRegistryAircraftRego[];
extern const TCHAR szRegistryLoggerID[];
extern const TCHAR szRegistryLoggerShort[];
extern const TCHAR szRegistrySoundVolume[];
extern const TCHAR szRegistrySoundDeadband[];
extern const TCHAR szRegistrySoundAudioVario[];
extern const TCHAR szRegistrySoundTask[];
extern const TCHAR szRegistrySoundModes[];
extern const TCHAR szRegistryNettoSpeed[];
extern const TCHAR szRegistryCDICruise[];
extern const TCHAR szRegistryCDICircling[];
extern const TCHAR szRegistryAutoBlank[];
extern const TCHAR szRegistryAutoBacklight[]; // VENTA4
extern const TCHAR szRegistryAutoSoundVolume[]; // VENTA4
extern const TCHAR szRegistryExtendedVisualGlide[]; // VENTA4
extern const TCHAR szRegistryVirtualKeys[]; // VENTA5
extern const TCHAR szRegistryAverEffTime[]; // VENTA6
extern const TCHAR szRegistryVarioGauge[];
extern const TCHAR szRegistryDebounceTimeout[];
extern const TCHAR szRegistryAppDefaultMapWidth[];
extern const TCHAR szRegistryAppIndFinalGlide[];
extern const TCHAR szRegistryAppIndLandable[];
extern const TCHAR szRegistryAppInverseInfoBox[];
extern const TCHAR szRegistryAppInfoBoxColors[];
extern const TCHAR szRegistryAppGaugeVarioSpeedToFly[];
extern const TCHAR szRegistryAppGaugeVarioAvgText[];
extern const TCHAR szRegistryAppGaugeVarioMc[];
extern const TCHAR szRegistryAppGaugeVarioBugs[];
extern const TCHAR szRegistryAppGaugeVarioBallast[];
extern const TCHAR szRegistryAppGaugeVarioGross[];
extern const TCHAR szRegistryAppCompassAppearance[];
extern const TCHAR szRegistryAppStatusMessageAlignment[];
extern const TCHAR szRegistryAppTextInputStyle[];
extern const TCHAR szRegistryAppInfoBoxBorder[];
#if defined(PNA) || defined(FIVV)
extern const TCHAR szRegistryAppInfoBoxGeom[];   // VENTA-ADDON GEOM CHANGE
extern const TCHAR szRegistryAppInfoBoxModel[]; // VENTA-ADDON MODEL CHANGE
#endif
extern const TCHAR szRegistryAppAveNeedle[];
extern const TCHAR szRegistryAutoAdvance[];
extern const TCHAR szRegistryUTCOffset[];
extern const TCHAR szRegistryBlockSTF[];
extern const TCHAR szRegistryAutoZoom[];
extern const TCHAR szRegistryMenuTimeout[];
extern const TCHAR szRegistryLockSettingsInFlight[];
extern const TCHAR szRegistryTerrainContrast[];
extern const TCHAR szRegistryTerrainBrightness[];
extern const TCHAR szRegistryTerrainRamp[];
extern const TCHAR szRegistryEnableFLARMMap[];
extern const TCHAR szRegistryEnableFLARMGauge[];
extern const TCHAR szRegistrySnailTrail[];
extern const TCHAR szRegistryTrailDrift[];
extern const TCHAR szRegistryThermalLocator[];
extern const TCHAR szRegistryGliderScreenPosition[];
extern const TCHAR szRegistryAnimation[];
extern const TCHAR szRegistrySetSystemTimeFromGPS[];
extern const TCHAR szRegistryAutoForceFinalGlide[];

extern const TCHAR szRegistryVoiceClimbRate[];
extern const TCHAR szRegistryVoiceTerrain[];
extern const TCHAR szRegistryVoiceWaypointDistance[];
extern const TCHAR szRegistryVoiceTaskAltitudeDifference[];
extern const TCHAR szRegistryVoiceMacCready[];
extern const TCHAR szRegistryVoiceNewWaypoint[];
extern const TCHAR szRegistryVoiceInSector[];
extern const TCHAR szRegistryVoiceAirspace[];

extern const TCHAR szRegistryFinishMinHeight[];
extern const TCHAR szRegistryStartMaxHeight[];
extern const TCHAR szRegistryStartMaxHeightMargin[];
extern const TCHAR szRegistryStartMaxSpeed[];
extern const TCHAR szRegistryStartMaxSpeedMargin[];
extern const TCHAR szRegistryStartHeightRef[];

extern const TCHAR szRegistryEnableNavBaroAltitude[];

extern const TCHAR szRegistryLoggerTimeStepCruise[];
extern const TCHAR szRegistryLoggerTimeStepCircling[];

extern const TCHAR szRegistrySafetyMacCready[];
extern const TCHAR szRegistryAbortSafetyUseCurrent[];
extern const TCHAR szRegistryAutoMcMode[];
extern const TCHAR szRegistryWaypointsOutOfRange[];
extern const TCHAR szRegistryEnableExternalTriggerCruise[];
extern const TCHAR szRegistryFAIFinishHeight[];
extern const TCHAR szRegistryOLCRules[];
extern const TCHAR szRegistryHandicap[];
extern const TCHAR szRegistrySnailWidthScale[];
extern const TCHAR szRegistryLatLonUnits[];
extern const TCHAR szRegistryUserLevel[];
extern const TCHAR szRegistryRiskGamma[];
extern const TCHAR szRegistryWindArrowStyle[];
extern const TCHAR szRegistryDisableAutoLogger[];
extern const TCHAR szRegistryMapFile[];
extern const TCHAR szRegistryBallastSecsToEmpty[];
extern const TCHAR szRegistryAccelerometerZero[];
extern const TCHAR szRegistryUseCustomFonts[];
extern const TCHAR szRegistryFontInfoWindowFont[];
extern const TCHAR szRegistryFontTitleWindowFont[];
extern const TCHAR szRegistryFontMapWindowFont[];
extern const TCHAR szRegistryFontTitleSmallWindowFont[];
extern const TCHAR szRegistryFontMapWindowBoldFont[];
extern const TCHAR szRegistryFontCDIWindowFont[];
extern const TCHAR szRegistryFontMapLabelFont[];
extern const TCHAR szRegistryFontStatisticsFont[];

bool GetFromRegistryD(const TCHAR *szRegValue, DWORD &pPos);
bool GetFromRegistry(const TCHAR *szRegValue, int &pPos);
bool GetFromRegistry(const TCHAR *szRegValue, short &pPos);
bool GetFromRegistry(const TCHAR *szRegValue, bool &pPos);
bool GetFromRegistry(const TCHAR *szRegValue, unsigned &pPos);
bool GetFromRegistry(const TCHAR *szRegValue, double &pPos);

HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos);
HRESULT SetToRegistry(const TCHAR *szRegValue, bool bVal);	// JG
HRESULT SetToRegistry(const TCHAR *szRegValue, int nVal);	// JG

#ifndef HAVE_POSIX /* DWORD==unsigned on WINE, would be duplicate */
HRESULT SetToRegistry(const TCHAR *szRegValue, unsigned nVal);	// JG
#endif

BOOL GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize);
HRESULT SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos);

void
SetRegistryStringIfAbsent(const TCHAR *name, const TCHAR *value);

void SetRegistryColour(int i, DWORD c);
void SetRegistryBrush(int i, DWORD c);
void SetRegistryAirspacePriority(int i);
void SetRegistryAirspaceMode(int i);
int GetRegistryAirspaceMode(int i);
void StoreType(int Index,int InfoType);

void ReadDeviceSettings(const int devIdx, TCHAR *Name);
void WriteDeviceSettings(const int devIdx, const TCHAR *Name);

void SaveRegistryToFile(const TCHAR* szFile);
void LoadRegistryFromFile(const TCHAR* szFile);

#endif
