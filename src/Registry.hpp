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

#include <windows.h>

#include <tchar.h>

extern const TCHAR szProfileKey[];
extern const TCHAR *szProfileDisplayType[];
extern const TCHAR *szProfileColour[];
extern const TCHAR *szProfileBrush[];
extern const TCHAR *szProfileAirspaceMode[];
extern const TCHAR szProfileSpeedUnitsValue[];
extern const TCHAR szProfileDistanceUnitsValue[];
extern const TCHAR szProfileAltitudeUnitsValue[];
extern const TCHAR szProfileLiftUnitsValue[];
extern const TCHAR szProfileTaskSpeedUnitsValue[];
extern const TCHAR szProfileDisplayUpValue[];
extern const TCHAR szProfileDisplayText[];
extern const TCHAR szProfileSafetyAltitudeArrival[];
extern const TCHAR szProfileSafetyAltitudeBreakOff[];
extern const TCHAR szProfileSafetyAltitudeTerrain[];
extern const TCHAR szProfileSafteySpeed[];
extern const TCHAR szProfileFAISector[];
extern const TCHAR szProfileSectorRadius[];
extern const TCHAR szProfilePolarID[];
extern const TCHAR szProfileWayPointFile[];
extern const TCHAR szProfileAdditionalWayPointFile[];
extern const TCHAR szProfileAirspaceFile[];
extern const TCHAR szProfileAdditionalAirspaceFile[];
extern const TCHAR szProfileWindSpeed[];
extern const TCHAR szProfileWindBearing[];
extern const TCHAR szProfileAirfieldFile[];
extern const TCHAR szProfileTopologyFile[];
extern const TCHAR szProfilePolarFile[];
extern const TCHAR szProfileTerrainFile[];
extern const TCHAR szProfileLanguageFile[];
extern const TCHAR szProfileStatusFile[];
extern const TCHAR szProfileInputFile[];
extern const TCHAR szProfileAltMode[];
extern const TCHAR szProfileClipAlt[];
extern const TCHAR szProfileAltMargin[];
extern const TCHAR szProfileRegKey[];
extern const TCHAR szProfileSnailTrail[];
extern const TCHAR szProfileDrawTopology[];
extern const TCHAR szProfileDrawTerrain[];
extern const TCHAR szProfileFinalGlideTerrain[];
extern const TCHAR szProfileAutoWind[];
extern const TCHAR szProfileStartLine[];
extern const TCHAR szProfileStartRadius[];
extern const TCHAR szProfileFinishLine[];
extern const TCHAR szProfileFinishRadius[];
extern const TCHAR *szProfileAirspacePriority[];
extern const TCHAR szProfileAirspaceWarning[];
extern const TCHAR szProfileAirspaceBlackOutline[];
extern const TCHAR szProfileWarningTime[];
extern const TCHAR szProfileAcknowledgementTime[];
extern const TCHAR szProfileCircleZoom[];
extern const TCHAR szProfileHomeWaypoint[];
extern const TCHAR szProfileAlternate1[];         // VENTA3
extern const TCHAR szProfileAlternate2[];
extern const TCHAR szProfileTeamcodeRefWaypoint[];
extern const TCHAR szProfilePilotName[];
extern const TCHAR szProfileAircraftType[];
extern const TCHAR szProfileAircraftRego[];
extern const TCHAR szProfileLoggerID[];
extern const TCHAR szProfileLoggerShort[];
extern const TCHAR szProfileSoundVolume[];
extern const TCHAR szProfileSoundDeadband[];
extern const TCHAR szProfileSoundAudioVario[];
extern const TCHAR szProfileSoundTask[];
extern const TCHAR szProfileSoundModes[];
extern const TCHAR szProfileNettoSpeed[];
extern const TCHAR szProfileCDICruise[];
extern const TCHAR szProfileCDICircling[];
extern const TCHAR szProfileAutoBlank[];
extern const TCHAR szProfileAutoBacklight[]; // VENTA4
extern const TCHAR szProfileAutoSoundVolume[]; // VENTA4
extern const TCHAR szProfileExtendedVisualGlide[]; // VENTA4
extern const TCHAR szProfileVirtualKeys[]; // VENTA5
extern const TCHAR szProfileAverEffTime[]; // VENTA6
extern const TCHAR szProfileVarioGauge[];
extern const TCHAR szProfileDebounceTimeout[];
extern const TCHAR szProfileAppDefaultMapWidth[];
extern const TCHAR szProfileAppIndFinalGlide[];
extern const TCHAR szProfileAppIndLandable[];
extern const TCHAR szProfileAppInverseInfoBox[];
extern const TCHAR szProfileAppInfoBoxColors[];
extern const TCHAR szProfileAppGaugeVarioSpeedToFly[];
extern const TCHAR szProfileAppGaugeVarioAvgText[];
extern const TCHAR szProfileAppGaugeVarioMc[];
extern const TCHAR szProfileAppGaugeVarioBugs[];
extern const TCHAR szProfileAppGaugeVarioBallast[];
extern const TCHAR szProfileAppGaugeVarioGross[];
extern const TCHAR szProfileAppCompassAppearance[];
extern const TCHAR szProfileAppStatusMessageAlignment[];
extern const TCHAR szProfileAppTextInputStyle[];
extern const TCHAR szProfileAppDialogStyle[];
extern const TCHAR szProfileAppInfoBoxBorder[];
#if defined(PNA) || defined(FIVV)
extern const TCHAR szProfileAppInfoBoxGeom[];   // VENTA-ADDON GEOM CHANGE
extern const TCHAR szProfileAppInfoBoxModel[]; // VENTA-ADDON MODEL CHANGE
#endif
extern const TCHAR szProfileAppAveNeedle[];
extern const TCHAR szProfileAutoAdvance[];
extern const TCHAR szProfileUTCOffset[];
extern const TCHAR szProfileBlockSTF[];
extern const TCHAR szProfileAutoZoom[];
extern const TCHAR szProfileMenuTimeout[];
extern const TCHAR szProfileLockSettingsInFlight[];
extern const TCHAR szProfileTerrainContrast[];
extern const TCHAR szProfileTerrainBrightness[];
extern const TCHAR szProfileTerrainRamp[];
extern const TCHAR szProfileEnableFLARMMap[];
extern const TCHAR szProfileEnableFLARMGauge[];
extern const TCHAR szProfileSnailTrail[];
extern const TCHAR szProfileTrailDrift[];
extern const TCHAR szProfileThermalLocator[];
extern const TCHAR szProfileGliderScreenPosition[];
extern const TCHAR szProfileAnimation[];
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

extern const TCHAR szProfileEnableNavBaroAltitude[];

extern const TCHAR szProfileLoggerTimeStepCruise[];
extern const TCHAR szProfileLoggerTimeStepCircling[];

extern const TCHAR szProfileSafetyMacCready[];
extern const TCHAR szProfileAbortSafetyUseCurrent[];
extern const TCHAR szProfileAutoMcMode[];
extern const TCHAR szProfileWaypointsOutOfRange[];
extern const TCHAR szProfileEnableExternalTriggerCruise[];
extern const TCHAR szProfileFAIFinishHeight[];
extern const TCHAR szProfileOLCRules[];
extern const TCHAR szProfileHandicap[];
extern const TCHAR szProfileSnailWidthScale[];
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
extern const TCHAR szProfileFontStatisticsFont[];

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

bool
GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize);

bool
SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos);

void
SetRegistryStringIfAbsent(const TCHAR *name, const TCHAR *value);

void SetRegistryColour(int i, DWORD c);
void SetRegistryBrush(int i, DWORD c);
void SetRegistryAirspacePriority(int i);
void SetRegistryAirspaceMode(int i);
int GetRegistryAirspaceMode(int i);
void StoreType(int Index,int InfoType);

struct DeviceConfig {
  enum port_type {
    /**
     * Serial port, i.e. COMx / RS-232.
     */
    SERIAL,

    /**
     * Attempt to auto-discover the GPS source.
     *
     * On Windows CE, this opens the GPS Intermediate Driver
     * Multiplexer:
     * http://msdn.microsoft.com/en-us/library/bb202042.aspx
     */
    AUTO,
  };

  port_type port_type;

  unsigned port_index;

  unsigned speed_index;

  TCHAR driver_name[32];
};

void
ReadDeviceConfig(unsigned n, DeviceConfig &config);

void
WriteDeviceConfig(unsigned n, const DeviceConfig &config);

void SaveRegistryToFile(const TCHAR* szFile);
void LoadRegistryFromFile(const TCHAR* szFile);

#endif
