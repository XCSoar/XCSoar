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

#include "Registry.hpp"
#include "StringUtil.hpp"
#include "Device/device.h"
#include "LogFile.hpp"
#include "Defines.h"

#include <assert.h>
#include <stdlib.h>

#ifndef WIN32
#include <gconf/gconf.h>
#endif

#ifdef WIN32
#define CONF(key) _T(key)
#else /* !WIN32 */
#define CONF(key) ("/apps/XCSoar/" key)
#endif

const TCHAR szRegistryKey[] = CONF(REGKEYNAME);
const TCHAR *szRegistryDisplayType[MAXINFOWINDOWS] = {
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

const TCHAR *szRegistryColour[] = {
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

const TCHAR *szRegistryBrush[] = {
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

const TCHAR *szRegistryAirspaceMode[] = {
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

const TCHAR *szRegistryAirspacePriority[] = {
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

const TCHAR szRegistryAirspaceWarning[] = CONF("AirspaceWarn");
const TCHAR szRegistryAirspaceBlackOutline[] = CONF("AirspaceBlackOutline");
const TCHAR szRegistryAltMargin[] = CONF("AltMargin");
const TCHAR szRegistryAltMode[] = CONF("AltitudeMode");
const TCHAR szRegistryAltitudeUnitsValue[] = CONF("Altitude");
const TCHAR szRegistryCircleZoom[] = CONF("CircleZoom");
const TCHAR szRegistryClipAlt[] = CONF("ClipAlt");
const TCHAR szRegistryDisplayText[] = CONF("DisplayText");
const TCHAR szRegistryDisplayUpValue[] = CONF("DisplayUp");
const TCHAR szRegistryDistanceUnitsValue[] = CONF("Distance");
const TCHAR szRegistryDrawTerrain[] = CONF("DrawTerrain");
const TCHAR szRegistryDrawTopology[] = CONF("DrawTopology");
const TCHAR szRegistryFAISector[] = CONF("FAISector");
const TCHAR szRegistryFinalGlideTerrain[] = CONF("FinalGlideTerrain");
const TCHAR szRegistryAutoWind[] = CONF("AutoWind");
const TCHAR szRegistryHomeWaypoint[] = CONF("HomeWaypoint");
const TCHAR szRegistryAlternate1[] = CONF("Alternate1"); // VENTA3
const TCHAR szRegistryAlternate2[] = CONF("Alternate2");
const TCHAR szRegistryLiftUnitsValue[] = CONF("Lift");
const TCHAR szRegistryLatLonUnits[] = CONF("LatLonUnits");
const TCHAR szRegistryPolarID[] = CONF("Polar"); // pL
const TCHAR szRegistryPort1Index[] = CONF("PortIndex");
const TCHAR szRegistryPort2Index[] = CONF("Port2Index");
const TCHAR szRegistryPort3Index[] = CONF("Port3Index");
const TCHAR szRegistryRegKey[] = CONF("RegKey");
const TCHAR szRegistrySafetyAltitudeArrival[] = CONF("SafetyAltitudeArrival");
const TCHAR szRegistrySafetyAltitudeBreakOff[] = CONF("SafetyAltitudeBreakOff");
const TCHAR szRegistrySafetyAltitudeTerrain[] = CONF("SafetyAltitudeTerrain");
const TCHAR szRegistrySafteySpeed[] = CONF("SafteySpeed");
const TCHAR szRegistrySectorRadius[] = CONF("Radius");
const TCHAR szRegistrySnailTrail[] = CONF("SnailTrail");
const TCHAR szRegistryTrailDrift[] = CONF("TrailDrift");
const TCHAR szRegistryThermalLocator[] = CONF("ThermalLocator");
const TCHAR szRegistryAnimation[] = CONF("Animation");
const TCHAR szRegistrySpeed1Index[] = CONF("SpeedIndex");
const TCHAR szRegistrySpeed2Index[] = CONF("Speed2Index");
const TCHAR szRegistrySpeed3Index[] = CONF("Speed3Index");
const TCHAR szRegistrySpeedUnitsValue[] = CONF("Speed");
const TCHAR szRegistryTaskSpeedUnitsValue[] = CONF("TaskSpeed");
const TCHAR szRegistryStartLine[] = CONF("StartLine");
const TCHAR szRegistryStartRadius[] = CONF("StartRadius");
const TCHAR szRegistryFinishLine[] = CONF("FinishLine");
const TCHAR szRegistryFinishRadius[] = CONF("FinishRadius");
const TCHAR szRegistryWarningTime[] = CONF("WarnTime");
const TCHAR szRegistryAcknowledgementTime[] = CONF("AcknowledgementTime");
const TCHAR szRegistryWindSpeed[] = CONF("WindSpeed");
const TCHAR szRegistryWindBearing[] = CONF("WindBearing");
const TCHAR szRegistryAirfieldFile[] = CONF("AirfieldFile"); // pL
const TCHAR szRegistryAirspaceFile[] = CONF("AirspaceFile"); // pL
const TCHAR szRegistryAdditionalAirspaceFile[] = CONF("AdditionalAirspaceFile"); // pL
const TCHAR szRegistryPolarFile[] = CONF("PolarFile"); // pL
const TCHAR szRegistryTerrainFile[] = CONF("TerrainFile"); // pL
const TCHAR szRegistryTopologyFile[] = CONF("TopologyFile"); // pL
const TCHAR szRegistryWayPointFile[] = CONF("WPFile"); // pL
const TCHAR szRegistryAdditionalWayPointFile[] = CONF("AdditionalWPFile"); // pL
const TCHAR szRegistryLanguageFile[] = CONF("LanguageFile"); // pL
const TCHAR szRegistryStatusFile[] = CONF("StatusFile"); // pL
const TCHAR szRegistryInputFile[] = CONF("InputFile"); // pL
const TCHAR szRegistryPilotName[] = CONF("PilotName");
const TCHAR szRegistryAircraftType[] = CONF("AircraftType");
const TCHAR szRegistryAircraftRego[] = CONF("AircraftRego");
const TCHAR szRegistryLoggerID[] = CONF("LoggerID");
const TCHAR szRegistryLoggerShort[] = CONF("LoggerShortName");
const TCHAR szRegistrySoundVolume[] = CONF("SoundVolume");
const TCHAR szRegistrySoundDeadband[] = CONF("SoundDeadband");
const TCHAR szRegistrySoundAudioVario[] = CONF("AudioVario");
const TCHAR szRegistrySoundTask[] = CONF("SoundTask");
const TCHAR szRegistrySoundModes[] = CONF("SoundModes");
const TCHAR szRegistryNettoSpeed[] = CONF("NettoSpeed");
const TCHAR szRegistryAccelerometerZero[] = CONF("AccelerometerZero");
const TCHAR szRegistryCDICruise[] = CONF("CDICruise");
const TCHAR szRegistryCDICircling[] = CONF("CDICircling");

const TCHAR szRegistryDeviceA[] = CONF("DeviceA");
const TCHAR szRegistryDeviceB[] = CONF("DeviceB");
const TCHAR szRegistryDeviceC[] = CONF("DeviceC");

const TCHAR szRegistryAutoBlank[] = CONF("AutoBlank");
const TCHAR szRegistryAutoBacklight[] = CONF("AutoBacklight");
const TCHAR szRegistryAutoSoundVolume[] = CONF("AutoSoundVolume");
const TCHAR szRegistryExtendedVisualGlide[] = CONF("ExtVisualGlide");
const TCHAR szRegistryVirtualKeys[] = CONF("VirtualKeys");
const TCHAR szRegistryAverEffTime[] = CONF("AverEffTime");
const TCHAR szRegistryVarioGauge[] = CONF("VarioGauge");

const TCHAR szRegistryDebounceTimeout[] = CONF("DebounceTimeout");

const TCHAR szRegistryAppIndFinalGlide[] = CONF("AppIndFinalGlide");
const TCHAR szRegistryAppIndLandable[] = CONF("AppIndLandable");
const TCHAR szRegistryAppInverseInfoBox[] = CONF("AppInverseInfoBox");
const TCHAR szRegistryAppGaugeVarioSpeedToFly[] = CONF("AppGaugeVarioSpeedToFly");
const TCHAR szRegistryAppGaugeVarioAvgText[] = CONF("AppGaugeVarioAvgText");
const TCHAR szRegistryAppGaugeVarioMc[] = CONF("AppGaugeVarioMc");
const TCHAR szRegistryAppGaugeVarioBugs[] = CONF("AppGaugeVarioBugs");
const TCHAR szRegistryAppGaugeVarioBallast[] = CONF("AppGaugeVarioBallast");
const TCHAR szRegistryAppGaugeVarioGross[] = CONF("AppGaugeVarioGross");
const TCHAR szRegistryAppCompassAppearance[] = CONF("AppCompassAppearance");
const TCHAR szRegistryAppStatusMessageAlignment[] = CONF("AppStatusMessageAlignment");
const TCHAR szRegistryAppTextInputStyle[] = CONF("AppTextInputStyle");
const TCHAR szRegistryAppDialogStyle[] = CONF("AppDialogStyle");
const TCHAR szRegistryAppInfoBoxColors[] = CONF("AppInfoBoxColors");
const TCHAR szRegistryAppDefaultMapWidth[] = CONF("AppDefaultMapWidth");
const TCHAR szRegistryTeamcodeRefWaypoint[] = CONF("TeamcodeRefWaypoint");
const TCHAR szRegistryAppInfoBoxBorder[] = CONF("AppInfoBoxBorder");

#if defined(PNA) || defined(FIVV)
const TCHAR szRegistryAppInfoBoxGeom[] = CONF("AppInfoBoxGeom"); // VENTA-ADDON GEOMETRY CONFIG
const TCHAR szRegistryAppInfoBoxModel[] = CONF("AppInfoBoxModel"); // VENTA-ADDON MODEL CONFIG
#endif

const TCHAR szRegistryAppAveNeedle[] = CONF("AppAveNeedle");

const TCHAR szRegistryAutoAdvance[] = CONF("AutoAdvance");
const TCHAR szRegistryUTCOffset[] = CONF("UTCOffset");
const TCHAR szRegistryBlockSTF[] = CONF("BlockSpeedToFly");
const TCHAR szRegistryAutoZoom[] = CONF("AutoZoom");
const TCHAR szRegistryMenuTimeout[] = CONF("MenuTimeout");
const TCHAR szRegistryLockSettingsInFlight[] = CONF("LockSettingsInFlight");
const TCHAR szRegistryTerrainContrast[] = CONF("TerrainContrast");
const TCHAR szRegistryTerrainBrightness[] = CONF("TerrainBrightness");
const TCHAR szRegistryTerrainRamp[] = CONF("TerrainRamp");
const TCHAR szRegistryEnableFLARMMap[] = CONF("EnableFLARMDisplay");
const TCHAR szRegistryEnableFLARMGauge[] = CONF("EnableFLARMGauge");
const TCHAR szRegistryFLARMGaugeBearing[] = CONF("FLARMGaugeBearing");
const TCHAR szRegistryGliderScreenPosition[] = CONF("GliderScreenPosition");
const TCHAR szRegistrySetSystemTimeFromGPS[] = CONF("SetSystemTimeFromGPS");
const TCHAR szRegistryAutoForceFinalGlide[] = CONF("AutoForceFinalGlide");

const TCHAR szRegistryVoiceClimbRate[] = CONF("VoiceClimbRate");
const TCHAR szRegistryVoiceTerrain[] = CONF("VoiceTerrain");
const TCHAR szRegistryVoiceWaypointDistance[] = CONF("VoiceWaypointDistance");
const TCHAR szRegistryVoiceTaskAltitudeDifference[] = CONF("VoiceTaskAltitudeDifference");
const TCHAR szRegistryVoiceMacCready[] = CONF("VoiceMacCready");
const TCHAR szRegistryVoiceNewWaypoint[] = CONF("VoiceNewWaypoint");
const TCHAR szRegistryVoiceInSector[] = CONF("VoiceInSector");
const TCHAR szRegistryVoiceAirspace[] = CONF("VoiceAirspace");

const TCHAR szRegistryFinishMinHeight[] = CONF("FinishMinHeight");
const TCHAR szRegistryStartMaxHeight[] = CONF("StartMaxHeight");
const TCHAR szRegistryStartMaxSpeed[] = CONF("StartMaxSpeed");
const TCHAR szRegistryStartMaxHeightMargin[] = CONF("StartMaxHeightMargin");
const TCHAR szRegistryStartMaxSpeedMargin[] = CONF("StartMaxSpeedMargin");
const TCHAR szRegistryStartHeightRef[] = CONF("StartHeightRef");
const TCHAR szRegistryEnableNavBaroAltitude[] = CONF("EnableNavBaroAltitude");

const TCHAR szRegistryLoggerTimeStepCruise[] = CONF("LoggerTimeStepCruise");
const TCHAR szRegistryLoggerTimeStepCircling[] = CONF("LoggerTimeStepCircling");

const TCHAR szRegistrySafetyMacCready[] = CONF("SafetyMacCready");
const TCHAR szRegistryAbortSafetyUseCurrent[] = CONF("AbortSafetyUseCurrent");
const TCHAR szRegistryAutoMcMode[] = CONF("AutoMcMode");
const TCHAR szRegistryWaypointsOutOfRange[] = CONF("WaypointsOutOfRange");
const TCHAR szRegistryEnableExternalTriggerCruise[] = CONF("EnableExternalTriggerCruise");
const TCHAR szRegistryFAIFinishHeight[] = CONF("FAIFinishHeight");
const TCHAR szRegistryOLCRules[] = CONF("OLCRules");
const TCHAR szRegistryHandicap[] = CONF("Handicap");
const TCHAR szRegistrySnailWidthScale[] = CONF("SnailWidthScale");
const TCHAR szRegistryUserLevel[] = CONF("UserLevel");
const TCHAR szRegistryRiskGamma[] = CONF("RiskGamma");
const TCHAR szRegistryWindArrowStyle[] = CONF("WindArrowStyle");
const TCHAR szRegistryDisableAutoLogger[] = CONF("DisableAutoLogger");
const TCHAR szRegistryMapFile[] = CONF("MapFile"); // pL
const TCHAR szRegistryBallastSecsToEmpty[] = CONF("BallastSecsToEmpty");
const TCHAR szRegistryUseCustomFonts[] = CONF("UseCustomFonts");
const TCHAR szRegistryFontInfoWindowFont[] = CONF("InfoWindowFont");
const TCHAR szRegistryFontTitleWindowFont[] = CONF("TitleWindowFont");
const TCHAR szRegistryFontMapWindowFont[] = CONF("MapWindowFont");
const TCHAR szRegistryFontTitleSmallWindowFont[] = CONF("TeamCodeFont");
const TCHAR szRegistryFontMapWindowBoldFont[] = CONF("MapWindowBoldFont");
const TCHAR szRegistryFontCDIWindowFont[] = CONF("CDIWindowFont");
const TCHAR szRegistryFontMapLabelFont[] = CONF("MapLabelFont");
const TCHAR szRegistryFontStatisticsFont[] = CONF("StatisticsFont");

#ifndef WIN32

class GConf {
protected:
  GConfEngine *engine;

public:
  GConf():engine(gconf_engine_get_default()) {}
  ~GConf() {
    gconf_engine_unref(engine);
  }

  bool get(const char *key, int &value) {
    GError *error = NULL;
    value = gconf_engine_get_int(engine, key, &error);
    if (value == 0 && error != NULL) {
      g_error_free(error);
      return false;
    }

    return true;
  }

  bool get(const char *key, char *value, size_t max_length) {
    gchar *buffer = gconf_engine_get_string(engine, key, NULL);
    if (buffer == NULL)
      return false;

    g_strlcpy(value, buffer, max_length);
    g_free(buffer);
    return true;
  }

  bool set(const char *key, int value) {
    return gconf_engine_set_int(engine, key, value, NULL);
  }

  bool set(const char *key, const char *value) {
    return gconf_engine_set_string(engine, key, value, NULL);
  }
};

#endif /* !WIN32 */

void StoreType(int Index,int the_type)
{
  SetToRegistry(szRegistryDisplayType[Index],(DWORD)the_type);
}

void SetRegistryStringIfAbsent(const TCHAR* name,
			       const TCHAR* value) {

  // VENTA force fonts registry rewrite in PNAs
#if defined(PNA) || defined(FIVV) // VENTA TODO WARNING should really delete the key before creating it TODO
  SetRegistryString(name, value);
#else
  TCHAR temp[MAX_PATH];
  if (GetRegistryString(name, temp, MAX_PATH)) {  // 0==ERROR_SUCCESS
    SetRegistryString(name, value);
  }
#endif
}

//
// NOTE: all registry variables are unsigned!
//
bool GetFromRegistryD(const TCHAR *szRegValue, DWORD &pPos)
{  // returns 0 on SUCCESS, else the non-zero error code
#ifdef WIN32
  HKEY    hKey;
  DWORD    dwSize, dwType;
  long    hRes;
  DWORD defaultVal;

  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, KEY_ALL_ACCESS, &hKey);
  if (hRes != ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return hRes;
    }

  defaultVal = pPos;
  dwSize = sizeof(DWORD);
  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)&pPos, &dwSize);
  if (hRes != ERROR_SUCCESS) {
    pPos = defaultVal;
  }
  RegCloseKey(hKey);
  return hRes;
#else /* !WIN32 */
  int value;
  if (!GConf().get(szRegValue, value))
    return false;

  pPos = (DWORD)value;
  return true;
#endif /* !WIN32 */
}


bool GetFromRegistry(const TCHAR *szRegValue, int &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS) {
    pPos = Temp;
  }
  return res;
}

bool GetFromRegistry(const TCHAR *szRegValue, short &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS) {
    pPos = Temp;
  }
  return res;
}

bool GetFromRegistry(const TCHAR *szRegValue, bool &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS) {
    pPos = Temp>0;
  }
  return res;
}

bool GetFromRegistry(const TCHAR *szRegValue, unsigned &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS) {
    pPos = Temp;
  }
  return res;
}

bool GetFromRegistry(const TCHAR *szRegValue, double &pPos)
{
  DWORD Temp = (DWORD)pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS) {
    pPos = (double)Temp;
  }
  return res;
}


// Implement your code to save value to the registry

HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos)
{
#ifdef WIN32
  HKEY    hKey;
  DWORD    Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS) {
    return FALSE;
  }

  hRes = RegSetValueEx(hKey, szRegValue,0,REG_DWORD, (LPBYTE)&Pos, sizeof(DWORD));
  RegCloseKey(hKey);

  return hRes;
#else /* !WIN32 */
  GConf().set(szRegValue, (int)Pos);

  return 0;
#endif /* !WIN32 */
}

// Set bool value to registry as 1 or 0 - JG
HRESULT SetToRegistry(const TCHAR *szRegValue, bool bVal)
{
	return SetToRegistry(szRegValue, bVal ? DWORD(1) : DWORD(0));
}

// Set int value to registry - JG
HRESULT SetToRegistry(const TCHAR *szRegValue, int nVal)
{
	return SetToRegistry(szRegValue, DWORD(nVal));
}

#ifndef HAVE_POSIX /* DWORD==unsigned on WINE, would be duplicate */
HRESULT SetToRegistry(const TCHAR *szRegValue, unsigned nVal)
{
	return SetToRegistry(szRegValue, DWORD(nVal));
}
#endif

/**
 * Reads a value from the registry file
 * @param szRegValue Name of the value that should be read
 * @param pPos Pointer to the output buffer
 * @param dwSize Maximum size of the output buffer
 */
BOOL GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
#ifdef WIN32
  HKEY    hKey;
  DWORD   dwType = REG_SZ;
  long    hRes;
  unsigned int i;
  for (i=0; i<dwSize; i++) {
    pPos[i]=0;
  }

  pPos[0]= '\0';
  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, KEY_READ /*KEY_ALL_ACCESS*/, &hKey);
  if (hRes != ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return hRes;
    }

  dwSize *= 2;

  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);

  RegCloseKey(hKey);
  return hRes;
#else /* !WIN32 */
  return GConf().get(szRegValue, pPos, dwSize);
#endif /* !WIN32 */
}

/**
 * Writes a value to the registry
 * @param szRegValue Name of the value that should be written
 * @param Pos Value that should be written
 */
HRESULT SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos)
{
#ifdef WIN32
  HKEY    hKey;
  DWORD    Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS)
    {
      return FALSE;
    }

  hRes = RegSetValueEx(hKey, szRegValue,0,REG_SZ, (LPBYTE)Pos, (_tcslen(Pos)+1)*sizeof(TCHAR));
  RegCloseKey(hKey);

  return hRes;
#else /* !WIN32 */
  return GConf().set(szRegValue, Pos);
#endif /* !WIN32 */
}

void ReadPort1Settings(DWORD *PortIndex, DWORD *SpeedIndex)
{
  DWORD Temp=0;

  if(GetFromRegistryD(szRegistryPort1Index,Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistryD(szRegistrySpeed1Index,Temp)==ERROR_SUCCESS)
    (*SpeedIndex) = Temp;
}


void WritePort1Settings(DWORD PortIndex, DWORD SpeedIndex)
{
  SetToRegistry(szRegistryPort1Index, PortIndex);
  SetToRegistry(szRegistrySpeed1Index, SpeedIndex);
}

void ReadPort2Settings(DWORD *PortIndex, DWORD *SpeedIndex)
{
  DWORD Temp=0;

  if(GetFromRegistryD(szRegistryPort2Index,Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistryD(szRegistrySpeed2Index,Temp)==ERROR_SUCCESS)
    (*SpeedIndex) = Temp;
}


void WritePort2Settings(DWORD PortIndex, DWORD SpeedIndex)
{
  SetToRegistry(szRegistryPort2Index, PortIndex);
  SetToRegistry(szRegistrySpeed2Index, SpeedIndex);
}

void ReadPort3Settings(DWORD *PortIndex, DWORD *SpeedIndex)
{
  DWORD Temp=0;

  if(GetFromRegistryD(szRegistryPort3Index,Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistryD(szRegistrySpeed3Index,Temp)==ERROR_SUCCESS)
    (*SpeedIndex) = Temp;
}


void WritePort3Settings(DWORD PortIndex, DWORD SpeedIndex)
{
  SetToRegistry(szRegistryPort3Index, PortIndex);
  SetToRegistry(szRegistrySpeed3Index, SpeedIndex);
}

#define CheckIndex(x, i)    assert((i>=0) && (sizeof(x)/sizeof(x[0]) > (unsigned)i));

void SetRegistryColour(int i, DWORD c)
{

  CheckIndex(szRegistryColour, i);

  SetToRegistry(szRegistryColour[i] ,c) ;
}


void SetRegistryBrush(int i, DWORD c)
{
  CheckIndex(szRegistryBrush, i);

  SetToRegistry(szRegistryBrush[i] ,c) ;
}

void ReadDeviceSettings(const int devIdx, TCHAR *Name){

  Name[0] = '\0';

  if (devIdx == 0){
    GetRegistryString(szRegistryDeviceA , Name, DEVNAMESIZE);
    return;
  }

  if (devIdx == 1){
    GetRegistryString(szRegistryDeviceB , Name, DEVNAMESIZE);
    return;
  }

  if (devIdx == 2){
    GetRegistryString(szRegistryDeviceC , Name, DEVNAMESIZE);
    return;
  }

}

void WriteDeviceSettings(const int devIdx, const TCHAR *Name){

  if (devIdx == 0)
    SetRegistryString(szRegistryDeviceA , Name);

  if (devIdx == 1)
    SetRegistryString(szRegistryDeviceB , Name);

  if (devIdx == 2)
    SetRegistryString(szRegistryDeviceC , Name);
}

// Registry file handling

const static size_t nMaxValueNameSize = MAX_PATH + 6; //255 + 1 + /r/n
const static size_t nMaxValueValueSize = MAX_PATH * 2 + 6; // max regkey name is 256 chars + " = "
const static size_t nMaxClassSize = MAX_PATH + 6;
const static size_t nMaxKeyNameSize = MAX_PATH + 6;

static bool LoadRegistryFromFile_inner(const TCHAR *szFile, bool wide=true)
{
  StartupStore(TEXT("Loading registry from %s\n"), szFile);
  bool found = false;
  FILE *fp=NULL;
  if (!string_is_empty(szFile))
#ifndef __GNUC__
    if (wide) {
      fp = _tfopen(szFile, TEXT("rb"));
    } else {
      fp = _tfopen(szFile, TEXT("rt"));
    }
#else
    fp = _tfopen(szFile, TEXT("rb"));    //20060515:sgi add b
#endif
  if(fp == NULL) {
    // error
    return false;
  }
  TCHAR winval[nMaxValueValueSize];
  TCHAR wname[nMaxValueValueSize];
  TCHAR wvalue[nMaxValueValueSize];
  int j;

#ifdef __GNUC__
  char inval[nMaxValueValueSize];
  char name [nMaxValueValueSize];
  char value [nMaxValueValueSize];
    if (wide) {
#endif
      while (_fgetts(winval, nMaxValueValueSize, fp)) {
#ifdef _UNICODE
        if (winval[0] > 255) { // not reading corectly, probably narrow file.
          break;
        }
#endif /* _UNICODE */
        if (_stscanf(winval, TEXT("%[^#=\r\n ]=\"%[^\r\n\"]\"[\r\n]"), wname, wvalue) == 2) {
          if (!string_is_empty(wname)) {
	    SetRegistryString(wname, wvalue);
	    found = true;
	  }
        } else if (_stscanf(winval, TEXT("%[^#=\r\n ]=%d[\r\n]"), wname, &j) == 2) {
          if (!string_is_empty(wname)) {
	    SetToRegistry(wname, j);
	    found = true;
	  }
        } else if (_stscanf(winval, TEXT("%[^#=\r\n ]=\"\"[\r\n]"), wname) == 1) {
          if (!string_is_empty(wname)) {
	    SetRegistryString(wname, TEXT(""));
	    found = true;
	  }
        } else {
	  //		assert(false);	// Invalid line reached
        }
      }

#ifdef __GNUC__
    } else {
      while (fgets(inval, nMaxValueValueSize, fp)) {
        if (sscanf(inval, "%[^#=\r\n ]=\"%[^\r\n\"]\"[\r\n]", name, value) == 2) {
	  if (strlen(name)>0) {
#ifdef _UNICODE
	    mbstowcs(wname, name, strlen(name)+1);
	    mbstowcs(wvalue, value, strlen(value)+1);
#else
            strcpy(wname, name);
            strcpy(wvalue, value);
#endif
	    SetRegistryString(wname, wvalue);
	    found = true;
	  }
        } else if (sscanf(inval, "%[^#=\r\n ]=%d[\r\n]", name, &j) == 2) {
	  if (strlen(name)>0) {
#ifdef _UNICODE
	    mbstowcs(wname, name, strlen(name)+1);
#else
            strcpy(wname, name);
#endif
	    SetToRegistry(wname, j);
	    found = true;
	  }
        } else if (sscanf(inval, "%[^#=\r\n ]=\"\"[\r\n]", name) == 1) {
	  if (strlen(name)>0) {
#ifdef _UNICODE
	    mbstowcs(wname, name, strlen(name)+1);
#else
            strcpy(wname, name);
#endif
	    SetRegistryString(wname, TEXT(""));
	    found = true;
	  }
        } else {
	  //		assert(false);	// Invalid line reached
        }
      }
    }
#endif

  fclose(fp);

  return found;
}

void LoadRegistryFromFile(const TCHAR *szFile) {
#ifndef __GNUC__
  if (!LoadRegistryFromFile_inner(szFile,true)) { // legacy, wide chars
    LoadRegistryFromFile_inner(szFile,false);       // new, non-wide chars
  }
#else
  if (!LoadRegistryFromFile_inner(szFile,false)) { // new, non-wide chars
    LoadRegistryFromFile_inner(szFile,true);       // legacy, wide chars
  }
#endif
}

void SaveRegistryToFile(const TCHAR *szFile)
{
#ifdef WIN32
  TCHAR lpstrName[nMaxKeyNameSize+1];
  //  TCHAR lpstrClass[nMaxClassSize+1];
#ifdef __GNUC__
  union {
    BYTE pValue[nMaxValueValueSize+4];
    DWORD dValue;
  } uValue;
#else
  BYTE pValue[nMaxValueValueSize+1];

  char sName[MAX_PATH];
  char sValue[MAX_PATH];

#endif

  HKEY hkFrom;
  LONG res = ::RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey,
			    0, KEY_ALL_ACCESS, &hkFrom);

  if (ERROR_SUCCESS != res) {
    return;
  }

  FILE *fp=NULL;
  if (!string_is_empty(szFile))
    fp = _tfopen(szFile, TEXT("wb"));  //20060515:sgi add b
  if(fp == NULL) {
    // error
    ::RegCloseKey(hkFrom);
    return;
  }

  for (int i = 0;;i++) {
    DWORD nType;
    DWORD nValueSize = nMaxValueValueSize;
    DWORD nNameSize = nMaxKeyNameSize;
//    DWORD nClassSize = nMaxClassSize;

    lpstrName[0] = _T('\0'); // null terminate, just in case

    LONG res = ::RegEnumValue(hkFrom, i, lpstrName,
			      &nNameSize, 0,
#ifdef __GNUC__
			      &nType, uValue.pValue,
#else
			      &nType, pValue,
#endif
			      &nValueSize);

    if (ERROR_NO_MORE_ITEMS == res) {
      break;
    }
    if ((nNameSize<=0)||(nNameSize>nMaxKeyNameSize)) {
      continue; // in case things get wierd
    }

    lpstrName[nNameSize] = _T('\0'); // null terminate, just in case

    if (_tcslen(lpstrName)>1) {

      // type 1 text
      // type 4 integer (valuesize 4)

      if (nType==4) { // data
#ifdef __GNUC__
	fprintf(fp,"%S=%d\r\n", lpstrName, uValue.dValue);
#else
	wcstombs(sName,lpstrName,nMaxKeyNameSize+1);
	fprintf(fp,"%s=%d\r\n", sName, *((DWORD*)pValue));
#endif
      } else
      // XXX SCOTT - Check that the output data (lpstrName and pValue) do not contain \r or \n
      if (nType==1) { // text
	if (nValueSize>0) {
#ifdef __GNUC__
	  uValue.pValue[nValueSize]= 0; // null terminate, just in case
	  uValue.pValue[nValueSize+1]= 0; // null terminate, just in case
          if (!string_is_empty((const TCHAR*)uValue.pValue)) {
	    fprintf(fp,"%S=\"%S\"\r\n", lpstrName, uValue.pValue);
	  } else {
	    fprintf(fp,"%S=\"\"\r\n", lpstrName);
	  }
#else
          if (!string_is_empty((const TCHAR*)pValue)) {
	    pValue[nValueSize]= 0; // null terminate, just in case
	    pValue[nValueSize+1]= 0; // null terminate, just in case
	    wcstombs(sName,lpstrName,nMaxKeyNameSize+1);
	    wcstombs(sValue,(TCHAR*)pValue,nMaxKeyNameSize+1);
	    fprintf(fp,"%s=\"%s\"\r\n", sName, sValue);
	  } else {
	    wcstombs(sName,lpstrName,nMaxKeyNameSize+1);
	    fprintf(fp,"%s=\"\"\r\n", sName);
	  }
#endif
	} else {
#ifdef __GNUC__
	  fprintf(fp,"%S=\"\"\r\n", lpstrName);
#else
	  fprintf(fp,"%s=\"\"\r\n", lpstrName);
#endif
	}
      }
    }

  }
#ifdef __GNUC__
  // JMW why flush agressively?
  fflush(fp);
#endif

#ifdef __GNUC__
  fprintf(fp,"\r\n"); // end of file
#endif

  fclose(fp);

  ::RegCloseKey(hkFrom);
#else /* !WIN32 */
  // XXX implement
#endif /* !WIN32 */
}
