/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "XCSoar.h"
#include "Screen/Animation.hpp"
#include "Screen/Blank.hpp"
#include "Airspace.h"
#include "Interface.hpp"
#include "MapWindow.h"
#include "GaugeFLARM.h"
#include "TerrainRenderer.h"
#include "Audio/VegaVoice.h"
#include "McReady.h"
#include "WayPoint.hpp"
#include "Screen/Fonts.hpp"
#include "Device/device.h"
#include "Math/FastMath.h"
#include "Math/Units.h"
#include "Logger.h"
#include "Utils.h"
#include "Device/Parser.h"
#include "externs.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include "LogFile.hpp"
#include "InfoBoxManager.h"
#include "Asset.hpp"
#include "GlideRatio.hpp"

#include <assert.h>
#include <stdlib.h>

const TCHAR szRegistryKey[] = TEXT(REGKEYNAME);
const TCHAR *szRegistryDisplayType[MAXINFOWINDOWS] =     { TEXT("Info0"),
				       TEXT("Info1"),
				       TEXT("Info2"),
				       TEXT("Info3"),
				       TEXT("Info4"),
				       TEXT("Info5"),
				       TEXT("Info6"),
				       TEXT("Info7"),
				       TEXT("Info8"),
				       TEXT("Info9")
				       TEXT("Info10")
				       TEXT("Info11")
				       TEXT("Info12")
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


extern int UserLevel; // used by dlgConfiguration

void StoreType(int Index,int InfoType)
{
  SetToRegistry(szRegistryDisplayType[Index],(DWORD)InfoType);
}

// This function checks to see if Final Glide mode infoboxes have been
// initialised.  If all are zero, then the current configuration was
// using XCSoarV3 infoboxes, so copy settings from cruise mode.
void CheckInfoTypes() {
  int i;
  char iszero=0;
  for (i=0; i<MAXINFOWINDOWS; i++) {
    iszero |= (InfoType[i] >> 16) & 0xff;
  }
  if (iszero==0) {
    for (i=0; i<MAXINFOWINDOWS; i++) {
      InfoType[i] += (InfoType[i] & 0xff)<<16;
      StoreType(i,InfoType[i]);
    }
  }
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

void DefaultRegistrySettingsAltair(void)
{  // RLD left GNAV Altair settings untouched.
   // these are redundant b/c they're also added to "InitialiseFontsHardCoded"
  SetRegistryStringIfAbsent(TEXT("InfoWindowFont"),
   TEXT("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  SetRegistryStringIfAbsent(TEXT("TitleWindowFont"),
   TEXT("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"));
  SetRegistryStringIfAbsent(TEXT("CDIWindowFont"),
   TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"));
  SetRegistryStringIfAbsent(TEXT("MapLabelFont"),
   TEXT("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"));
  SetRegistryStringIfAbsent(TEXT("StatisticsFont"),
   TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  SetRegistryStringIfAbsent(TEXT("MapWindowFont"),
   TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  SetRegistryStringIfAbsent(TEXT("MapWindowBoldFont"),
   TEXT("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"));
  SetRegistryStringIfAbsent(TEXT("BugsBallastFont"),
   TEXT("24,0,0,0,750,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  SetRegistryStringIfAbsent(TEXT("AirspacePressFont"),
   TEXT("24,0,0,0,750,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"));
  SetRegistryStringIfAbsent(TEXT("AirspaceColourDlgFont"),
   TEXT("14,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"));
  SetRegistryStringIfAbsent(TEXT("TeamCodeFont"),
   TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"));
#if 0
  SetRegistryStringIfAbsent(TEXT("ScaleList"),
   TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));
#endif
}


void SaveRegistryAirspacePriority() {
  for (int i=0; i<AIRSPACECLASSCOUNT; i++) {
    SetToRegistry(szRegistryAirspacePriority[i], AirspacePriority[i]);
  }
}


void ReadRegistrySettings(void)
{
  DWORD Speed = 0;
  DWORD Distance = 0;
  DWORD TaskSpeed = 0;
  DWORD Lift = 0;
  DWORD Altitude = 0;
//  DWORD DisplayUp = 0;
  DWORD Temp = 0;
  int i;

  StartupStore(TEXT("Read registry settings\n"));

#if defined(GNAV) || defined(PCGNAV) || defined(GNAV_FONTEST)
  DefaultRegistrySettingsAltair();
#endif

  for (i=0; i<AIRSPACECLASSCOUNT; i++) {
    Temp=0;
    GetFromRegistry(szRegistryAirspacePriority[i], &Temp);
    AirspacePriority[i] = Temp;
  }

  Temp = 0;
  GetFromRegistry(szRegistryLatLonUnits, &Temp);
  Units::CoordinateFormat = (CoordinateFormats_t)Temp;

  GetFromRegistry(szRegistrySpeedUnitsValue,&Speed);
  switch(Speed)
    {
    case 0 :
      SPEEDMODIFY = TOMPH;
      break;
    case 1 :
      SPEEDMODIFY = TOKNOTS;
      break;
    case 2 :
      SPEEDMODIFY = TOKPH;
      break;
    }

  TaskSpeed = 2;
  GetFromRegistry(szRegistryTaskSpeedUnitsValue,&TaskSpeed);
  switch(TaskSpeed)
    {
    case 0 :
      TASKSPEEDMODIFY = TOMPH;
      break;
    case 1 :
      TASKSPEEDMODIFY = TOKNOTS;
      break;
    case 2 :
      TASKSPEEDMODIFY = TOKPH;
      break;
    }

  GetFromRegistry(szRegistryDistanceUnitsValue,&Distance);
  switch(Distance)
    {
    case 0 : DISTANCEMODIFY = TOMILES; break;
    case 1 : DISTANCEMODIFY = TONAUTICALMILES; break;
    case 2 : DISTANCEMODIFY = TOKILOMETER; break;
    }

  GetFromRegistry(szRegistryAltitudeUnitsValue,&Altitude);
  switch(Altitude)
    {
    case 0 : ALTITUDEMODIFY = TOFEET; break;
    case 1 : ALTITUDEMODIFY = TOMETER; break;
    }

  GetFromRegistry(szRegistryLiftUnitsValue,&Lift);
  switch(Lift)
    {
    case 0 : LIFTMODIFY = TOKNOTS; break;
    case 1 : LIFTMODIFY = TOMETER; break;
    }

  Units::NotifyUnitChanged();

  for (i=0;i<MAXINFOWINDOWS;i++)
    {
      Temp = InfoType[i];
      GetFromRegistry(szRegistryDisplayType[i],&Temp);
      InfoType[i] = Temp;
    }

  // check against V3 infotypes
  CheckInfoTypes();

  Temp=0;
  GetFromRegistry(szRegistryDisplayUpValue,&Temp);
  switch(Temp)
    {
    case TRACKUP : DisplayOrientation = TRACKUP; break;
    case NORTHUP : DisplayOrientation = NORTHUP;break;
    case NORTHCIRCLE : DisplayOrientation = NORTHCIRCLE;break;
    case TRACKCIRCLE : DisplayOrientation = TRACKCIRCLE;break;
    case NORTHTRACK : DisplayOrientation = NORTHTRACK;break;
    }

  Temp=0;
  GetFromRegistry(szRegistryDisplayText,&Temp);
  switch(Temp)
    {
    case 0 : DisplayTextType = DISPLAYNAME; break;
    case 1 : DisplayTextType = DISPLAYNUMBER;break;
    case 2 : DisplayTextType = DISPLAYFIRSTFIVE; break;
    case 3 : DisplayTextType = DISPLAYNONE;break;
    case 4 : DisplayTextType = DISPLAYFIRSTTHREE; break;
    case 5 : DisplayTextType = DISPLAYNAMEIFINTASK; break;
    }

  Temp=AltitudeMode;
  if(GetFromRegistry(szRegistryAltMode,&Temp)==ERROR_SUCCESS)
    AltitudeMode = Temp;

  Temp=ClipAltitude;
  if(GetFromRegistry(szRegistryClipAlt,&Temp)==ERROR_SUCCESS)
    ClipAltitude = Temp;

  Temp=AltWarningMargin;
  if(GetFromRegistry(szRegistryAltMargin,&Temp)==ERROR_SUCCESS)
    AltWarningMargin = Temp;

  Temp=(DWORD)SAFETYALTITUDEARRIVAL;
  GetFromRegistry(szRegistrySafetyAltitudeArrival,&Temp);
  SAFETYALTITUDEARRIVAL = (double)Temp;

  Temp=(DWORD)SAFETYALTITUDEBREAKOFF;
  GetFromRegistry(szRegistrySafetyAltitudeBreakOff,&Temp);
  SAFETYALTITUDEBREAKOFF = (double)Temp;

  Temp=(DWORD)SAFETYALTITUDETERRAIN;
  GetFromRegistry(szRegistrySafetyAltitudeTerrain,&Temp);
  SAFETYALTITUDETERRAIN = (double)Temp;

  Temp=(DWORD)SAFTEYSPEED;
  GetFromRegistry(szRegistrySafteySpeed,&Temp);
  SAFTEYSPEED = (double)Temp;

  Temp = SectorType;
  GetFromRegistry(szRegistryFAISector,&Temp);
  SectorType = Temp;

  SectorRadius = 10000;
  GetFromRegistry(szRegistrySectorRadius,
		  &SectorRadius);

  Temp = POLARID;
  GetFromRegistry(szRegistryPolarID,
		  &Temp);
  POLARID = (int)Temp;

  GetRegistryString(szRegistryRegKey, strRegKey, 65);

  for(i=0;i<AIRSPACECLASSCOUNT;i++)
    {
      MapWindow::iAirspaceMode[i] = GetRegistryAirspaceMode(i);

      Temp= MapWindow::iAirspaceBrush[i];
      if(GetFromRegistry(szRegistryBrush[i],&Temp)==ERROR_SUCCESS)
        MapWindow::iAirspaceBrush[i] =			(int)Temp;

      Temp= MapWindow::iAirspaceColour[i];
      if(GetFromRegistry(szRegistryColour[i],&Temp)==ERROR_SUCCESS)
        MapWindow::iAirspaceColour[i] =			(int)Temp;

      if (MapWindow::iAirspaceColour[i]>= NUMAIRSPACECOLORS) {
        MapWindow::iAirspaceColour[i]= 0;
      }

      if (MapWindow::iAirspaceBrush[i]>= NUMAIRSPACEBRUSHES) {
        MapWindow::iAirspaceBrush[i]= 0;
      }

    }

  Temp = MapWindow::bAirspaceBlackOutline;
  GetFromRegistry(szRegistryAirspaceBlackOutline,&Temp);
  MapWindow::bAirspaceBlackOutline = (Temp == 1);

  Temp = TrailActive;
  GetFromRegistry(szRegistrySnailTrail,&Temp);
  TrailActive = Temp;

  Temp = MapWindow::EnableTrailDrift;
  GetFromRegistry(szRegistryTrailDrift,&Temp);
  MapWindow::EnableTrailDrift = (Temp==1);

  Temp = EnableThermalLocator;
  GetFromRegistry(szRegistryThermalLocator,&Temp);
  EnableThermalLocator = Temp;

  //Temp = EnableAnimation;
  Temp=0; // disabled by default VNT9
  GetFromRegistry(szRegistryAnimation,&Temp);
  EnableAnimation = (Temp==1);

  Temp  = EnableTopology;
  GetFromRegistry(szRegistryDrawTopology,&Temp);
  EnableTopology = (Temp == 1);

  Temp  = EnableTerrain;
  GetFromRegistry(szRegistryDrawTerrain,&Temp);
  EnableTerrain = (Temp == 1);

  Temp  = FinalGlideTerrain;
  GetFromRegistry(szRegistryFinalGlideTerrain,&Temp);
  FinalGlideTerrain = Temp;

  Temp  = AutoWindMode;
  GetFromRegistry(szRegistryAutoWind,&Temp);
  AutoWindMode = Temp;

  Temp  = CircleZoom;
  GetFromRegistry(szRegistryCircleZoom,&Temp);
  CircleZoom = (Temp == 1);

  Temp = HomeWaypoint;
  if (GetFromRegistry(szRegistryHomeWaypoint,&Temp)==ERROR_SUCCESS) {
    HomeWaypoint = Temp;
  } else {
    HomeWaypoint = -1;
  }

// VENTA3
  Temp = Alternate1;
  if (GetFromRegistry(szRegistryAlternate1,&Temp)==ERROR_SUCCESS) {
    Alternate1 = Temp;
    // TODO: for portrait no need to force alternate calculations here.
    // Infobox will trigger them on if visible..
    OnAlternate1=true;
  } else {
    Alternate1 = -1;
    OnAlternate1=false;
  }

  Temp = Alternate2;
  if (GetFromRegistry(szRegistryAlternate2,&Temp)==ERROR_SUCCESS) {
    Alternate2 = Temp;
    OnAlternate2=true;
  } else {
    Alternate2 = -1;
    OnAlternate2=false;
  }


  Temp = MapWindow::SnailWidthScale;
  GetFromRegistry(szRegistrySnailWidthScale,&Temp);
  MapWindow::SnailWidthScale = Temp;

  Temp = TeamCodeRefWaypoint;
  GetFromRegistry(szRegistryTeamcodeRefWaypoint,&Temp);
  TeamCodeRefWaypoint = Temp;

  Temp = 1;
  GetFromRegistry(szRegistryStartLine,&Temp);
  StartLine = Temp;

  Temp = 1000;
  GetFromRegistry(szRegistryStartRadius,&Temp);
  StartRadius = Temp;

  Temp = 1;
  GetFromRegistry(szRegistryFinishLine,&Temp);
  FinishLine = Temp;

  Temp = 1000;
  GetFromRegistry(szRegistryFinishRadius,&Temp);
  FinishRadius = Temp;

  Temp = 0;
  GetFromRegistry(szRegistryAirspaceWarning,&Temp);
  AIRSPACEWARNINGS = Temp;

  Temp = 30;
  GetFromRegistry(szRegistryWarningTime,&Temp);
  WarningTime = max(10,Temp);

  Temp = 30;
  GetFromRegistry(szRegistryAcknowledgementTime,&Temp);
  AcknowledgementTime = max(10,Temp);

  Temp = 80;
  GetFromRegistry(szRegistrySoundVolume,&Temp);
  SoundVolume = Temp;

  Temp = 4;
  GetFromRegistry(szRegistrySoundDeadband,&Temp);
  SoundDeadband = Temp;

  Temp = 1;
  GetFromRegistry(szRegistrySoundAudioVario,&Temp);
  EnableSoundVario = (Temp == 1);

  Temp = 1;
  GetFromRegistry(szRegistrySoundTask,&Temp);
  EnableSoundTask = (Temp == 1);

  Temp = 1;
  GetFromRegistry(szRegistrySoundModes,&Temp);
  EnableSoundModes = (Temp == 1);

  /* no longer used
  Temp = 500;
  GetFromRegistry(szRegistryNettoSpeed,&Temp);
  NettoSpeed = Temp;
  */

  EnableCDICruise = 0;
  EnableCDICircling = 0;

  /* JMW temporarily disabled these because they are not updated for 4.7+
  Temp = 0;
  GetFromRegistry(szRegistryCDICruise,&Temp);
  EnableCDICruise = (Temp == 1);

  Temp = 0;
  GetFromRegistry(szRegistryCDICircling,&Temp);
  EnableCDICircling = (Temp == 1);
  */

#ifdef HAVE_BLANK
  Temp = 0;
  GetFromRegistry(szRegistryAutoBlank,&Temp);
  EnableAutoBlank = (Temp == 1);
#endif

  Temp = 1;
  GetFromRegistry(szRegistryAutoBacklight,&Temp); // VENTA4
  EnableAutoBacklight = (Temp == 1);

  Temp = 1;
  GetFromRegistry(szRegistryAutoSoundVolume,&Temp); // VENTA4
  EnableAutoSoundVolume = (Temp == 1);

  Temp = 0;
  GetFromRegistry(szRegistryExtendedVisualGlide,&Temp); // VENTA4
  ExtendedVisualGlide = Temp;


#ifdef PNA
  Temp = 1;
#else
  Temp = 0;
#endif
  GetFromRegistry(szRegistryVirtualKeys,&Temp);
  VirtualKeys = Temp;

  Temp = (AverEffTime_t)ae2minutes;
  GetFromRegistry(szRegistryAverEffTime,&Temp);
  AverEffTime = Temp;

  /*
  Temp = 0;
  GetFromRegistry(szRegistryVarioGauge,&Temp);
  EnableVarioGauge = (Temp == 1);
  */

#if defined(GNAV) || defined(PCGNAV)
  Temp = 0;
#else
  Temp = 250;
#endif
  GetFromRegistry(szRegistryDebounceTimeout, &Temp);
  debounceTimeout = Temp;

  Temp = 100;
  GetFromRegistry(szRegistryAccelerometerZero,&Temp);
  AccelerometerZero = Temp;
  if (AccelerometerZero==0.0) {
    AccelerometerZero= 100.0;
    Temp = 100;
    SetToRegistry(szRegistryAccelerometerZero,Temp);
  }

  // new appearance variables

  //Temp = Appearance.IndFinalGlide;
  Temp=(IndFinalGlide_t)fgFinalGlideDefault; // VNT9 default
  GetFromRegistry(szRegistryAppIndFinalGlide, &Temp);
  Appearance.IndFinalGlide = (IndFinalGlide_t)Temp;

  Temp = Appearance.IndLandable;
  GetFromRegistry(szRegistryAppIndLandable, &Temp);
  Appearance.IndLandable = (IndLandable_t)Temp;

  Temp = Appearance.InverseInfoBox;
  GetFromRegistry(szRegistryAppInverseInfoBox, &Temp);
  Appearance.InverseInfoBox = (Temp != 0);

  Temp = Appearance.GaugeVarioSpeedToFly;
  GetFromRegistry(szRegistryAppGaugeVarioSpeedToFly, &Temp);
  Appearance.GaugeVarioSpeedToFly = (Temp != 0);

  Temp = Appearance.GaugeVarioAvgText;
  GetFromRegistry(szRegistryAppGaugeVarioAvgText, &Temp);
  Appearance.GaugeVarioAvgText = (Temp != 0);

  Temp = Appearance.GaugeVarioMc;
  GetFromRegistry(szRegistryAppGaugeVarioMc, &Temp);
  Appearance.GaugeVarioMc = (Temp != 0);

  Temp = Appearance.GaugeVarioBugs;
  GetFromRegistry(szRegistryAppGaugeVarioBugs, &Temp);
  Appearance.GaugeVarioBugs = (Temp != 0);

  Temp = Appearance.GaugeVarioBallast;
  GetFromRegistry(szRegistryAppGaugeVarioBallast, &Temp);
  Appearance.GaugeVarioBallast = (Temp != 0);

  Temp = Appearance.GaugeVarioGross;
  GetFromRegistry(szRegistryAppGaugeVarioGross, &Temp);
  Appearance.GaugeVarioGross = (Temp != 0);

  Temp = Appearance.CompassAppearance;
  GetFromRegistry(szRegistryAppCompassAppearance, &Temp);
  Appearance.CompassAppearance = (CompassAppearance_t)Temp;

  //Temp = Appearance.InfoBoxBorder;
  Temp=(InfoBoxBorderAppearance_t)apIbBox; // VNT9 default
  GetFromRegistry(szRegistryAppInfoBoxBorder, &Temp);
  Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)Temp;

// VENTA2-ADDON Geometry change and PNA custom font settings
// depending on infobox geometry and model type
// I had to move here the font setting because I needed first to
// know the screen geometry, in the registry!
#if defined(PNA) || defined(FIVV)
  Temp = Appearance.InfoBoxGeom;
  GetFromRegistry(szRegistryAppInfoBoxGeom, &Temp);
  Appearance.InfoBoxGeom = (InfoBoxGeomAppearance_t)Temp;

  if (GlobalModelType == MODELTYPE_PNA_HP31X ) {
			needclipping=true;
			// key transcoding for this one
			StartupStore(TEXT("Loading HP31X settings\n"));
	}
	else
	if (GlobalModelType == MODELTYPE_PNA_PN6000 ) {
			StartupStore(TEXT("Loading PN6000 settings\n"));
			// key transcoding for this one
	}
	else
	if (GlobalModelType == MODELTYPE_PNA_MIO ) {
			StartupStore(TEXT("Loading MIO settings\n"));
			// currently no special settings from MIO but need to handle hw keys
	}
	else
	if (GlobalModelType == MODELTYPE_PNA_NOKIA_500 ) {
			StartupStore(TEXT("Loading Nokia500 settings\n"));
			// key transcoding is made
	}
	else
	if (GlobalModelType == MODELTYPE_PNA_MEDION_P5 ) {
			StartupStore(TEXT("Loading Medion settings\n"));
			needclipping=true;
	}
	else
	if (GlobalModelType == MODELTYPE_PNA_PNA ) {
		StartupStore(TEXT("Loading default PNA settings\n"));
	}
	else
		StartupStore(TEXT("No special regsets for this PDA\n")); // VENTA2

// VENTA-ADDON Model change
  Temp = Appearance.InfoBoxModel;
  GetFromRegistry(szRegistryAppInfoBoxModel, &Temp);
  Appearance.InfoBoxModel = (InfoBoxModelAppearance_t)Temp;
#endif

  Temp = Appearance.StateMessageAlligne;
  GetFromRegistry(szRegistryAppStatusMessageAlignment, &Temp);
  Appearance.StateMessageAlligne = (StateMessageAlligne_t)Temp;

  Temp = Appearance.TextInputStyle;
  GetFromRegistry(szRegistryAppTextInputStyle, &Temp);
  Appearance.TextInputStyle = (TextInputStyle_t)Temp;

  Temp = Appearance.DefaultMapWidth;
  GetFromRegistry(szRegistryAppDefaultMapWidth, &Temp);
  Appearance.DefaultMapWidth = Temp;

  //Temp = Appearance.InfoBoxColors;
  Temp=1; // true as default VNT9
  GetFromRegistry(szRegistryAppInfoBoxColors, &Temp);
  Appearance.InfoBoxColors = (Temp != 0);

  Temp = Appearance.GaugeVarioAveNeedle;
  GetFromRegistry(szRegistryAppAveNeedle, &Temp);
  Appearance.GaugeVarioAveNeedle = (Temp != 0);

  // StateMessageAlligne : center, topleft
  // DefaultMapWidth: 206?
  // CompassAppearance (north arrow)
  //
  // DontShowLoggerIndicator
  // FlightModeIcon
  // DontShowAutoMacCready
  // MapScale
  // MapScale2
  // BestCruiseTrack
  // Aircraft
  // IndFinalGlide
  // IndLandable

  Temp = 1;
  GetFromRegistry(szRegistryAutoAdvance,&Temp);
  AutoAdvance = (Temp == 1);

  Temp = AutoMcMode;
  GetFromRegistry(szRegistryAutoMcMode,&Temp);
  AutoMcMode = Temp;

  Temp = WaypointsOutOfRange;
  GetFromRegistry(szRegistryWaypointsOutOfRange,&Temp);
  WaypointsOutOfRange = Temp;

  Temp = OLCRules;
  GetFromRegistry(szRegistryOLCRules,&Temp);
  OLCRules = Temp;

  Temp = EnableFAIFinishHeight;
  GetFromRegistry(szRegistryFAIFinishHeight,&Temp);
  EnableFAIFinishHeight = (Temp==1);

  Temp = Handicap;
  GetFromRegistry(szRegistryHandicap,&Temp);
  Handicap = Temp;

  Temp = EnableExternalTriggerCruise;
  GetFromRegistry(szRegistryEnableExternalTriggerCruise,&Temp);
  EnableExternalTriggerCruise = Temp;

  Temp = 0;
  GetFromRegistry(szRegistryUTCOffset,&Temp);
  UTCOffset = Temp;
  if (UTCOffset>12*3600) {
    UTCOffset-= 24*3600;
  }

  Temp = 0;
  GetFromRegistry(szRegistryBlockSTF,&Temp);
  EnableBlockSTF = (Temp == 1);

  Temp = 0;
  GetFromRegistry(szRegistryAutoZoom,&Temp);
  MapWindow::AutoZoom = (Temp == 1);

  Temp = MenuTimeoutMax;
  GetFromRegistry(szRegistryMenuTimeout,&Temp);
  MenuTimeoutMax = Temp;

  Temp = 1;
  GetFromRegistry(szRegistryLockSettingsInFlight,&Temp);
  LockSettingsInFlight = (Temp == 1);

  Temp = 0;
  GetFromRegistry(szRegistryLoggerShort,&Temp);
  LoggerShortName = (Temp == 1);

  Temp = EnableFLARMMap;
  GetFromRegistry(szRegistryEnableFLARMMap,&Temp);
  EnableFLARMMap = Temp;

  Temp = EnableFLARMGauge;
  GetFromRegistry(szRegistryEnableFLARMGauge,&Temp);
  EnableFLARMGauge = (Temp==1);

  Temp = TerrainContrast;
  GetFromRegistry(szRegistryTerrainContrast,&Temp);
  TerrainContrast = (short)Temp;

  Temp = TerrainBrightness;
  GetFromRegistry(szRegistryTerrainBrightness,&Temp);
  TerrainBrightness = (short)Temp;

  Temp = TerrainRamp;
  GetFromRegistry(szRegistryTerrainRamp,&Temp);
  TerrainRamp = (short)Temp;

  Temp = MapWindow::GliderScreenPosition;
  GetFromRegistry(szRegistryGliderScreenPosition,&Temp);
  MapWindow::GliderScreenPosition = (int)Temp;

  Temp = BallastSecsToEmpty;
  GetFromRegistry(szRegistryBallastSecsToEmpty,&Temp);
  BallastSecsToEmpty = Temp;

  Temp = SetSystemTimeFromGPS;
  GetFromRegistry(szRegistrySetSystemTimeFromGPS,&Temp);
  SetSystemTimeFromGPS = (Temp!=0);

  Temp = AutoForceFinalGlide;
  GetFromRegistry(szRegistryAutoForceFinalGlide,&Temp);
  AutoForceFinalGlide = (Temp!=0);


  Temp = 0; // fonts
  GetFromRegistry(szRegistryUseCustomFonts,&Temp);
  UseCustomFonts = (Temp == 0 ? 0 : 1);
  SetToRegistry(szRegistryUseCustomFonts, UseCustomFonts);


  ////

  Temp = EnableVoiceClimbRate;
  GetFromRegistry(szRegistryVoiceClimbRate,&Temp);
  EnableVoiceClimbRate = (Temp!=0);

  Temp = EnableVoiceTerrain;
  GetFromRegistry(szRegistryVoiceTerrain,&Temp);
  EnableVoiceTerrain = (Temp!=0);

  Temp = EnableVoiceWaypointDistance;
  GetFromRegistry(szRegistryVoiceWaypointDistance,&Temp);
  EnableVoiceWaypointDistance = (Temp!=0);

  Temp = EnableVoiceTaskAltitudeDifference;
  GetFromRegistry(szRegistryVoiceTaskAltitudeDifference,&Temp);
  EnableVoiceTaskAltitudeDifference = (Temp!=0);

  Temp = EnableVoiceMacCready;
  GetFromRegistry(szRegistryVoiceMacCready,&Temp);
  EnableVoiceMacCready = (Temp!=0);

  Temp = EnableVoiceNewWaypoint;
  GetFromRegistry(szRegistryVoiceNewWaypoint,&Temp);
  EnableVoiceNewWaypoint = (Temp!=0);

  Temp = EnableVoiceInSector;
  GetFromRegistry(szRegistryVoiceInSector,&Temp);
  EnableVoiceInSector = (Temp!=0);

  Temp = EnableVoiceAirspace;
  GetFromRegistry(szRegistryVoiceAirspace,&Temp);
  EnableVoiceAirspace = (Temp!=0);

  Temp = FinishMinHeight;
  GetFromRegistry(szRegistryFinishMinHeight,&Temp);
  FinishMinHeight = Temp;

  Temp = StartHeightRef;
  GetFromRegistry(szRegistryStartHeightRef,&Temp);
  StartHeightRef = Temp;

  Temp = StartMaxHeight;
  GetFromRegistry(szRegistryStartMaxHeight,&Temp);
  StartMaxHeight = Temp;

  Temp = StartMaxHeightMargin;
  GetFromRegistry(szRegistryStartMaxHeightMargin,&Temp);
  StartMaxHeightMargin = Temp;

  Temp = StartMaxSpeed;
  GetFromRegistry(szRegistryStartMaxSpeed,&Temp);
  StartMaxSpeed = Temp;

  Temp = StartMaxSpeedMargin;
  GetFromRegistry(szRegistryStartMaxSpeedMargin,&Temp);
  StartMaxSpeedMargin = Temp;

  Temp = EnableNavBaroAltitude;
  GetFromRegistry(szRegistryEnableNavBaroAltitude,&Temp);
  EnableNavBaroAltitude = (Temp!=0);


  Temp = LoggerTimeStepCruise;
  GetFromRegistry(szRegistryLoggerTimeStepCruise,&Temp);
  LoggerTimeStepCruise = Temp;

  Temp = LoggerTimeStepCircling;
  GetFromRegistry(szRegistryLoggerTimeStepCircling,&Temp);
  LoggerTimeStepCircling = Temp;

  Temp = GlidePolar::AbortSafetyUseCurrent;
  GetFromRegistry(szRegistryAbortSafetyUseCurrent, &Temp);
  GlidePolar::AbortSafetyUseCurrent = (Temp != 0);

  Temp = iround(GlidePolar::SafetyMacCready*10);
  GetFromRegistry(szRegistrySafetyMacCready,&Temp);
  GlidePolar::SafetyMacCready = Temp/10.0;

  Temp  = UserLevel;
  GetFromRegistry(szRegistryUserLevel,&Temp);
  UserLevel = Temp;

  Temp  = iround(GlidePolar::RiskGamma*10);
  GetFromRegistry(szRegistryRiskGamma,&Temp);
  GlidePolar::RiskGamma = Temp/10.0;

  //Temp = MapWindow::WindArrowStyle;
  Temp=(CompassAppearance_t)apCompassAltA; // VNT9 default
  GetFromRegistry(szRegistryWindArrowStyle,&Temp);
  MapWindow::WindArrowStyle = Temp;

  Temp = DisableAutoLogger;
  GetFromRegistry(szRegistryDisableAutoLogger,&Temp);
  if (Temp)
    DisableAutoLogger = true;
  else
    DisableAutoLogger = false;
}

//
// NOTE: all registry variables are unsigned!
//
BOOL GetFromRegistry(const TCHAR *szRegValue, DWORD *pPos)
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

  defaultVal = *pPos;
  dwSize = sizeof(DWORD);
  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);
  if (hRes != ERROR_SUCCESS) {
    *pPos = defaultVal;
  }

  RegCloseKey(hKey);
  return hRes;
}


// Implement your code to save value to the registry

HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos)
{
  HKEY    hKey;
  DWORD    Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS)
    {
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
      return FALSE;
    }

  dwSize *= 2;

  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);

  RegCloseKey(hKey);
  return hRes;
}

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

  if(GetFromRegistry(szRegistryPort1Index,&Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistry(szRegistrySpeed1Index,&Temp)==ERROR_SUCCESS)
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

  if(GetFromRegistry(szRegistryPort2Index,&Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistry(szRegistrySpeed2Index,&Temp)==ERROR_SUCCESS)
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

  if(GetFromRegistry(szRegistryPort3Index,&Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistry(szRegistrySpeed3Index,&Temp)==ERROR_SUCCESS)
    (*SpeedIndex) = Temp;
}


void WritePort3Settings(DWORD PortIndex, DWORD SpeedIndex)
{
  SetToRegistry(szRegistryPort3Index, PortIndex);
  SetToRegistry(szRegistrySpeed3Index, SpeedIndex);
}

#define CheckIndex(x, i)    assert((i>=0) && (sizeof(x)/sizeof(x[0]) > i));

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



void SetRegistryAirspaceMode(int i)
{

  CheckIndex(MapWindow::iAirspaceMode, i);
  CheckIndex(szRegistryAirspaceMode, i);

  DWORD val = MapWindow::iAirspaceMode[i];
  SetToRegistry(szRegistryAirspaceMode[i], val);
}

int GetRegistryAirspaceMode(int i) {
  DWORD Temp= 3; // display + warnings
  CheckIndex(szRegistryAirspaceMode, i);
  GetFromRegistry(szRegistryAirspaceMode[i],&Temp);
  return Temp;
}

void SaveSoundSettings()
{
  SetToRegistry(szRegistrySoundVolume, (DWORD)SoundVolume);
  SetToRegistry(szRegistrySoundDeadband, (DWORD)SoundDeadband);
  SetToRegistry(szRegistrySoundAudioVario, EnableSoundVario);
  SetToRegistry(szRegistrySoundTask, EnableSoundTask);
  SetToRegistry(szRegistrySoundModes, EnableSoundModes);
}

void SaveWindToRegistry() {
  DWORD Temp;
  Temp = iround(CALCULATED_INFO.WindSpeed);
  SetToRegistry(szRegistryWindSpeed,Temp);
  Temp = iround(CALCULATED_INFO.WindBearing);
  SetToRegistry(szRegistryWindBearing,Temp);
  SetWindEstimate(CALCULATED_INFO.WindSpeed, CALCULATED_INFO.WindBearing);

}

void LoadWindFromRegistry() {
  StartupStore(TEXT("Load wind from registry\n"));

  DWORD Temp;
  Temp=0;
  GetFromRegistry(szRegistryWindSpeed,&Temp);
  CALCULATED_INFO.WindSpeed = Temp;
  Temp=0;
  GetFromRegistry(szRegistryWindBearing,&Temp);
  CALCULATED_INFO.WindBearing = Temp;
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

//////////////////////////
// Registry file handling
/////////////////

const static int nMaxValueNameSize = MAX_PATH + 6; //255 + 1 + /r/n
const static int nMaxValueValueSize = MAX_PATH * 2 + 6; // max regkey name is 256 chars + " = "
const static int nMaxClassSize = MAX_PATH + 6;
const static int nMaxKeyNameSize = MAX_PATH + 6;

static bool LoadRegistryFromFile_inner(const TCHAR *szFile, bool wide=true)
{
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
        if (winval[0] > 255) { // not reading corectly, probably narrow file.
          break;
        }
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
  char sName[nMaxKeyNameSize+1];
  char sValue[nMaxValueValueSize+1];
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

    if (_tcslen(lpstrName)>0) {

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
