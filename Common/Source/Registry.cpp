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
#include "Device/device.h"
#include "LogFile.hpp"
#include "Defines.h"

#include <assert.h>
#include <stdlib.h>

const TCHAR szRegistryKey[] = TEXT(REGKEYNAME);
const TCHAR *szRegistryDisplayType[MAXINFOWINDOWS] =
  { TEXT("Info0"),
    TEXT("Info1"),
    TEXT("Info2"),
    TEXT("Info3"),
    TEXT("Info4"),
    TEXT("Info5"),
    TEXT("Info6"),
    TEXT("Info7"),
    TEXT("Info8"),
    TEXT("Info9"),
    TEXT("Info10"),
    TEXT("Info11"),
    TEXT("Info12"),
    TEXT("Info13")
}; // pL

const TCHAR *szRegistryColour[] =     { TEXT("Colour0"),
				  TEXT("Colour1"),
				  TEXT("Colour2"),
				  TEXT("Colour3"),
				  TEXT("Colour4"),
				  TEXT("Colour5"),
				  TEXT("Colour6"),
				  TEXT("Colour7"),
				  TEXT("Colour8"),
				  TEXT("Colour9"),
				  TEXT("Colour10"),
				  TEXT("Colour11"),
				  TEXT("Colour12"),
				  TEXT("Colour13"),
				  TEXT("Colour14")
}; // pL


const TCHAR *szRegistryBrush[] =     {  TEXT("Brush0"),
				  TEXT("Brush1"),
				  TEXT("Brush2"),
				  TEXT("Brush3"),
				  TEXT("Brush4"),
				  TEXT("Brush5"),
				  TEXT("Brush6"),
				  TEXT("Brush7"),
				  TEXT("Brush8"),
				  TEXT("Brush9"),
				  TEXT("Brush10"),
				  TEXT("Brush11"),
				  TEXT("Brush12"),
				  TEXT("Brush13"),
				  TEXT("Brush14")
}; // pL

const TCHAR *szRegistryAirspaceMode[] =     {  TEXT("AirspaceMode0"),
					       TEXT("AirspaceMode1"),
					       TEXT("AirspaceMode2"),
					       TEXT("AirspaceMode3"),
					       TEXT("AirspaceMode4"),
					       TEXT("AirspaceMode5"),
					       TEXT("AirspaceMode6"),
					       TEXT("AirspaceMode7"),
					       TEXT("AirspaceMode8"),
					       TEXT("AirspaceMode9"),
					       TEXT("AirspaceMode10"),
					       TEXT("AirspaceMode11"),
					       TEXT("AirspaceMode12"),
					       TEXT("AirspaceMode13"),
					       TEXT("AirspaceMode14")
}; // pL


const TCHAR *szRegistryAirspacePriority[] = {  TEXT("AirspacePriority0"),
					 TEXT("AirspacePriority1"),
					 TEXT("AirspacePriority2"),
					 TEXT("AirspacePriority3"),
					 TEXT("AirspacePriority4"),
					 TEXT("AirspacePriority5"),
					 TEXT("AirspacePriority6"),
					 TEXT("AirspacePriority7"),
					 TEXT("AirspacePriority8"),
					 TEXT("AirspacePriority9"),
					 TEXT("AirspacePriority10"),
					 TEXT("AirspacePriority11"),
					 TEXT("AirspacePriority12"),
					 TEXT("AirspacePriority13"),
					 TEXT("AirspacePriority14")
}; // pL


const TCHAR szRegistryAirspaceWarning[]= TEXT("AirspaceWarn");
const TCHAR szRegistryAirspaceBlackOutline[]= TEXT("AirspaceBlackOutline");
const TCHAR szRegistryAltMargin[]=	   TEXT("AltMargin");
const TCHAR szRegistryAltMode[]=  TEXT("AltitudeMode");
const TCHAR szRegistryAltitudeUnitsValue[] = TEXT("Altitude");
const TCHAR szRegistryCircleZoom[]= TEXT("CircleZoom");
const TCHAR szRegistryClipAlt[]= TEXT("ClipAlt");
const TCHAR szRegistryDisplayText[] = TEXT("DisplayText");
const TCHAR szRegistryDisplayUpValue[] = TEXT("DisplayUp");
const TCHAR szRegistryDistanceUnitsValue[] = TEXT("Distance");
const TCHAR szRegistryDrawTerrain[]= TEXT("DrawTerrain");
const TCHAR szRegistryDrawTopology[]= TEXT("DrawTopology");
const TCHAR szRegistryFAISector[] = TEXT("FAISector");
const TCHAR szRegistryFinalGlideTerrain[]= TEXT("FinalGlideTerrain");
const TCHAR szRegistryAutoWind[]= TEXT("AutoWind");
const TCHAR szRegistryHomeWaypoint[]= TEXT("HomeWaypoint");
const TCHAR szRegistryAlternate1[]= TEXT("Alternate1"); // VENTA3
const TCHAR szRegistryAlternate2[]= TEXT("Alternate2");
const TCHAR szRegistryLiftUnitsValue[] = TEXT("Lift");
const TCHAR szRegistryLatLonUnits[] = TEXT("LatLonUnits");
const TCHAR szRegistryPolarID[] = TEXT("Polar"); // pL
const TCHAR szRegistryPort1Index[]= TEXT("PortIndex");
const TCHAR szRegistryPort2Index[]= TEXT("Port2Index");
const TCHAR szRegistryPort3Index[]= TEXT("Port3Index");
const TCHAR szRegistryRegKey[]=				 TEXT("RegKey");
const TCHAR szRegistrySafetyAltitudeArrival[] =     TEXT("SafetyAltitudeArrival");
const TCHAR szRegistrySafetyAltitudeBreakOff[] =     TEXT("SafetyAltitudeBreakOff");
const TCHAR szRegistrySafetyAltitudeTerrain[] =     TEXT("SafetyAltitudeTerrain");
const TCHAR szRegistrySafteySpeed[] =          TEXT("SafteySpeed");
const TCHAR szRegistrySectorRadius[]=          TEXT("Radius");
const TCHAR szRegistrySnailTrail[]=		 TEXT("SnailTrail");
const TCHAR szRegistryTrailDrift[]=		 TEXT("TrailDrift");
const TCHAR szRegistryThermalLocator[]=	 TEXT("ThermalLocator");
const TCHAR szRegistryAnimation[]=		 TEXT("Animation");
const TCHAR szRegistrySpeed1Index[]=		 TEXT("SpeedIndex");
const TCHAR szRegistrySpeed2Index[]=		 TEXT("Speed2Index");
const TCHAR szRegistrySpeed3Index[]=		 TEXT("Speed3Index");
const TCHAR szRegistrySpeedUnitsValue[] =      TEXT("Speed");
const TCHAR szRegistryTaskSpeedUnitsValue[] =      TEXT("TaskSpeed");
const TCHAR szRegistryStartLine[]=		 TEXT("StartLine");
const TCHAR szRegistryStartRadius[]=		 TEXT("StartRadius");
const TCHAR szRegistryFinishLine[]=		 TEXT("FinishLine");
const TCHAR szRegistryFinishRadius[]=		 TEXT("FinishRadius");
const TCHAR szRegistryWarningTime[]=		 TEXT("WarnTime");
const TCHAR szRegistryAcknowledgementTime[]=	 TEXT("AcknowledgementTime");
const TCHAR szRegistryWindSpeed[] =            TEXT("WindSpeed");
const TCHAR szRegistryWindBearing[] =          TEXT("WindBearing");
const TCHAR szRegistryAirfieldFile[]=  TEXT("AirfieldFile"); // pL
const TCHAR szRegistryAirspaceFile[]=  TEXT("AirspaceFile"); // pL
const TCHAR szRegistryAdditionalAirspaceFile[]=  TEXT("AdditionalAirspaceFile"); // pL
const TCHAR szRegistryPolarFile[] = TEXT("PolarFile"); // pL
const TCHAR szRegistryTerrainFile[]=	 TEXT("TerrainFile"); // pL
const TCHAR szRegistryTopologyFile[]=  TEXT("TopologyFile"); // pL
const TCHAR szRegistryWayPointFile[]=  TEXT("WPFile"); // pL
const TCHAR szRegistryAdditionalWayPointFile[]=  TEXT("AdditionalWPFile"); // pL
const TCHAR szRegistryLanguageFile[]=  TEXT("LanguageFile"); // pL
const TCHAR szRegistryStatusFile[]=  TEXT("StatusFile"); // pL
const TCHAR szRegistryInputFile[]=  TEXT("InputFile"); // pL
const TCHAR szRegistryPilotName[]=  TEXT("PilotName");
const TCHAR szRegistryAircraftType[]=  TEXT("AircraftType");
const TCHAR szRegistryAircraftRego[]=  TEXT("AircraftRego");
const TCHAR szRegistryLoggerID[]=  TEXT("LoggerID");
const TCHAR szRegistryLoggerShort[]=  TEXT("LoggerShortName");
const TCHAR szRegistrySoundVolume[]=  TEXT("SoundVolume");
const TCHAR szRegistrySoundDeadband[]=  TEXT("SoundDeadband");
const TCHAR szRegistrySoundAudioVario[]=  TEXT("AudioVario");
const TCHAR szRegistrySoundTask[]=  TEXT("SoundTask");
const TCHAR szRegistrySoundModes[]=  TEXT("SoundModes");
const TCHAR szRegistryNettoSpeed[]= TEXT("NettoSpeed");
const TCHAR szRegistryAccelerometerZero[]= TEXT("AccelerometerZero");
const TCHAR szRegistryCDICruise[]= TEXT("CDICruise");
const TCHAR szRegistryCDICircling[]= TEXT("CDICircling");

const TCHAR szRegistryDeviceA[]= TEXT("DeviceA");
const TCHAR szRegistryDeviceB[]= TEXT("DeviceB");
const TCHAR szRegistryDeviceC[]= TEXT("DeviceC");

const TCHAR szRegistryAutoBlank[]= TEXT("AutoBlank");
const TCHAR szRegistryAutoBacklight[]= TEXT("AutoBacklight");
const TCHAR szRegistryAutoSoundVolume[]= TEXT("AutoSoundVolume");
const TCHAR szRegistryExtendedVisualGlide[]= TEXT("ExtVisualGlide");
const TCHAR szRegistryVirtualKeys[]= TEXT("VirtualKeys");
const TCHAR szRegistryAverEffTime[]= TEXT("AverEffTime");
const TCHAR szRegistryVarioGauge[]= TEXT("VarioGauge");

const TCHAR szRegistryDebounceTimeout[]= TEXT("DebounceTimeout");

const TCHAR szRegistryAppIndFinalGlide[] = TEXT("AppIndFinalGlide");
const TCHAR szRegistryAppIndLandable[] = TEXT("AppIndLandable");
const TCHAR szRegistryAppInverseInfoBox[] = TEXT("AppInverseInfoBox");
const TCHAR szRegistryAppGaugeVarioSpeedToFly[] = TEXT("AppGaugeVarioSpeedToFly");
const TCHAR szRegistryAppGaugeVarioAvgText[] = TEXT("AppGaugeVarioAvgText");
const TCHAR szRegistryAppGaugeVarioMc[] = TEXT("AppGaugeVarioMc");
const TCHAR szRegistryAppGaugeVarioBugs[] = TEXT("AppGaugeVarioBugs");
const TCHAR szRegistryAppGaugeVarioBallast[] = TEXT("AppGaugeVarioBallast");
const TCHAR szRegistryAppGaugeVarioGross[] = TEXT("AppGaugeVarioGross");
const TCHAR szRegistryAppCompassAppearance[] = TEXT("AppCompassAppearance");
const TCHAR szRegistryAppStatusMessageAlignment[] = TEXT("AppStatusMessageAlignment");
const TCHAR szRegistryAppTextInputStyle[] = TEXT("AppTextInputStyle");
const TCHAR szRegistryAppInfoBoxColors[] = TEXT("AppInfoBoxColors");
const TCHAR szRegistryAppDefaultMapWidth[] = TEXT("AppDefaultMapWidth");
const TCHAR szRegistryTeamcodeRefWaypoint[] = TEXT("TeamcodeRefWaypoint");
const TCHAR szRegistryAppInfoBoxBorder[] = TEXT("AppInfoBoxBorder");

#if defined(PNA) || defined(FIVV)
const TCHAR szRegistryAppInfoBoxGeom[] = TEXT("AppInfoBoxGeom"); // VENTA-ADDON GEOMETRY CONFIG
const TCHAR szRegistryAppInfoBoxModel[] = TEXT("AppInfoBoxModel"); // VENTA-ADDON MODEL CONFIG
#endif

const TCHAR szRegistryAppAveNeedle[] = TEXT("AppAveNeedle");

const TCHAR szRegistryAutoAdvance[] = TEXT("AutoAdvance");
const TCHAR szRegistryUTCOffset[] = TEXT("UTCOffset");
const TCHAR szRegistryBlockSTF[] = TEXT("BlockSpeedToFly");
const TCHAR szRegistryAutoZoom[] = TEXT("AutoZoom");
const TCHAR szRegistryMenuTimeout[] = TEXT("MenuTimeout");
const TCHAR szRegistryLockSettingsInFlight[] = TEXT("LockSettingsInFlight");
const TCHAR szRegistryTerrainContrast[] = TEXT("TerrainContrast");
const TCHAR szRegistryTerrainBrightness[] = TEXT("TerrainBrightness");
const TCHAR szRegistryTerrainRamp[] = TEXT("TerrainRamp");
const TCHAR szRegistryEnableFLARMMap[] = TEXT("EnableFLARMDisplay");
const TCHAR szRegistryEnableFLARMGauge[] = TEXT("EnableFLARMGauge");
const TCHAR szRegistryFLARMGaugeBearing[] = TEXT("FLARMGaugeBearing");
const TCHAR szRegistryGliderScreenPosition[] = TEXT("GliderScreenPosition");
const TCHAR szRegistrySetSystemTimeFromGPS[] = TEXT("SetSystemTimeFromGPS");
const TCHAR szRegistryAutoForceFinalGlide[] = TEXT("AutoForceFinalGlide");

const TCHAR szRegistryVoiceClimbRate[]= TEXT("VoiceClimbRate");
const TCHAR szRegistryVoiceTerrain[]= TEXT("VoiceTerrain");
const TCHAR szRegistryVoiceWaypointDistance[]= TEXT("VoiceWaypointDistance");
const TCHAR szRegistryVoiceTaskAltitudeDifference[]= TEXT("VoiceTaskAltitudeDifference");
const TCHAR szRegistryVoiceMacCready[]= TEXT("VoiceMacCready");
const TCHAR szRegistryVoiceNewWaypoint[]= TEXT("VoiceNewWaypoint");
const TCHAR szRegistryVoiceInSector[]= TEXT("VoiceInSector");
const TCHAR szRegistryVoiceAirspace[]= TEXT("VoiceAirspace");

const TCHAR szRegistryFinishMinHeight[]= TEXT("FinishMinHeight");
const TCHAR szRegistryStartMaxHeight[]= TEXT("StartMaxHeight");
const TCHAR szRegistryStartMaxSpeed[]= TEXT("StartMaxSpeed");
const TCHAR szRegistryStartMaxHeightMargin[]= TEXT("StartMaxHeightMargin");
const TCHAR szRegistryStartMaxSpeedMargin[]= TEXT("StartMaxSpeedMargin");
const TCHAR szRegistryStartHeightRef[] = TEXT("StartHeightRef");
const TCHAR szRegistryEnableNavBaroAltitude[] = TEXT("EnableNavBaroAltitude");

const TCHAR szRegistryLoggerTimeStepCruise[]= TEXT("LoggerTimeStepCruise");
const TCHAR szRegistryLoggerTimeStepCircling[]= TEXT("LoggerTimeStepCircling");

const TCHAR szRegistrySafetyMacCready[] = TEXT("SafetyMacCready");
const TCHAR szRegistryAbortSafetyUseCurrent[] = TEXT("AbortSafetyUseCurrent");
const TCHAR szRegistryAutoMcMode[] = TEXT("AutoMcMode");
const TCHAR szRegistryWaypointsOutOfRange[] = TEXT("WaypointsOutOfRange");
const TCHAR szRegistryEnableExternalTriggerCruise[] = TEXT("EnableExternalTriggerCruise");
const TCHAR szRegistryFAIFinishHeight[] = TEXT("FAIFinishHeight");
const TCHAR szRegistryOLCRules[] = TEXT("OLCRules");
const TCHAR szRegistryHandicap[] = TEXT("Handicap");
const TCHAR szRegistrySnailWidthScale[] = TEXT("SnailWidthScale");
const TCHAR szRegistryUserLevel[] = TEXT("UserLevel");
const TCHAR szRegistryRiskGamma[] = TEXT("RiskGamma");
const TCHAR szRegistryWindArrowStyle[] = TEXT("WindArrowStyle");
const TCHAR szRegistryDisableAutoLogger[] = TEXT("DisableAutoLogger");
const TCHAR szRegistryMapFile[]=	 TEXT("MapFile"); // pL
const TCHAR szRegistryBallastSecsToEmpty[]=	 TEXT("BallastSecsToEmpty");
const TCHAR szRegistryUseCustomFonts[]=	 TEXT("UseCustomFonts");
const TCHAR szRegistryFontInfoWindowFont[]=	 TEXT("InfoWindowFont");
const TCHAR szRegistryFontTitleWindowFont[]=	 TEXT("TitleWindowFont");
const TCHAR szRegistryFontMapWindowFont[]=	 TEXT("MapWindowFont");
const TCHAR szRegistryFontTitleSmallWindowFont[]=	 TEXT("TeamCodeFont");
const TCHAR szRegistryFontMapWindowBoldFont[]=	 TEXT("MapWindowBoldFont");
const TCHAR szRegistryFontCDIWindowFont[]=	 TEXT("CDIWindowFont");
const TCHAR szRegistryFontMapLabelFont[]=	 TEXT("MapLabelFont");
const TCHAR szRegistryFontStatisticsFont[]=	 TEXT("StatisticsFont");

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
  return 0;
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS) {
    pPos = Temp;
  }
  return res;
}


// Implement your code to save value to the registry

HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos)
{
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

#ifndef __WINE__ /* DWORD==unsigned on WINE, would be duplicate */
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
}

/**
 * Writes a value to the registry
 * @param szRegValue Name of the value that should be written
 * @param Pos Value that should be written
 */
HRESULT SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos)
{
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
  if (_tcslen(szFile)>0)
#ifndef __MINGW32__
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

#ifdef __MINGW32__
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
	  if (_tcslen(wname)>0) {
	    SetRegistryString(wname, wvalue);
	    found = true;
	  }
        } else if (_stscanf(winval, TEXT("%[^#=\r\n ]=%d[\r\n]"), wname, &j) == 2) {
	  if (_tcslen(wname)>0) {
	    SetToRegistry(wname, j);
	    found = true;
	  }
        } else if (_stscanf(winval, TEXT("%[^#=\r\n ]=\"\"[\r\n]"), wname) == 1) {
	  if (_tcslen(wname)>0) {
	    SetRegistryString(wname, TEXT(""));
	    found = true;
	  }
        } else {
	  //		assert(false);	// Invalid line reached
        }
      }

#ifdef __MINGW32__
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
#ifndef __MINGW32__
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
  TCHAR lpstrName[nMaxKeyNameSize+1];
  //  TCHAR lpstrClass[nMaxClassSize+1];
#ifdef __MINGW32__
  union {
    BYTE pValue[nMaxValueValueSize+4];
    DWORD dValue;
  } uValue;
#else
  BYTE pValue[nMaxValueValueSize+1];
#endif

  HKEY hkFrom;
  LONG res = ::RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey,
			    0, KEY_ALL_ACCESS, &hkFrom);

  if (ERROR_SUCCESS != res) {
    return;
  }

  FILE *fp=NULL;
  if (_tcslen(szFile)>0)
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
#ifdef __MINGW32__
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
#ifdef __MINGW32__
	fprintf(fp,"%S=%d\r\n", lpstrName, uValue.dValue);
#else
	wcstombs(sName,lpstrName,nMaxKeyNameSize+1);
	fprintf(fp,"%s=%d\r\n", sName, *((DWORD*)pValue));
#endif
      } else
      // XXX SCOTT - Check that the output data (lpstrName and pValue) do not contain \r or \n
      if (nType==1) { // text
	if (nValueSize>0) {
#ifdef __MINGW32__
	  uValue.pValue[nValueSize]= 0; // null terminate, just in case
	  uValue.pValue[nValueSize+1]= 0; // null terminate, just in case
	  if (_tcslen((TCHAR*)uValue.pValue)>0) {
	    fprintf(fp,"%S=\"%S\"\r\n", lpstrName, uValue.pValue);
	  } else {
	    fprintf(fp,"%S=\"\"\r\n", lpstrName);
	  }
#else
	  if (_tcslen((TCHAR*)pValue)>0) {
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
#ifdef __MINGW32__
	  fprintf(fp,"%S=\"\"\r\n", lpstrName);
#else
	  fprintf(fp,"%s=\"\"\r\n", lpstrName);
#endif
	}
      }
    }

  }
#ifdef __MINGW32__
  // JMW why flush agressively?
  fflush(fp);
#endif

#ifdef __MINGW32__
  fprintf(fp,"\r\n"); // end of file
#endif

  fclose(fp);

  ::RegCloseKey(hkFrom);
}
