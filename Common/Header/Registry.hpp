#ifndef XCSOAR_REGISTRY_HPP
#define XCSOAR_REGISTRY_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tchar.h>

extern const TCHAR szRegistryKey[];
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
extern const TCHAR szRegistryAirspaceWarning[];
extern const TCHAR szRegistryAirspaceBlackOutline[];
extern const TCHAR szRegistryWarningTime[];
extern const TCHAR szRegistryAcknowledgementTime[];
extern const TCHAR szRegistryCircleZoom[];
extern const TCHAR szRegistryWindUpdateMode[];
extern const TCHAR szRegistryHomeWaypoint[];
extern const TCHAR szRegistryAlternate1[];         // VENTA3
extern const TCHAR szRegistryAlternate2[];
extern const TCHAR szRegistryTeamcodeRefWaypoint[];
extern const TCHAR szRegistryPilotName[];
extern const TCHAR szRegistryAircraftType[];
extern const TCHAR szRegistryAircraftRego[];
extern const TCHAR szRegistryLoggerID[];
extern const TCHAR szRegistryLoggerShort[];
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

BOOL GetFromRegistry(const TCHAR *szRegValue, DWORD *pPos);

HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos);
HRESULT SetToRegistry(const TCHAR *szRegValue, bool bVal);	// JG
HRESULT SetToRegistry(const TCHAR *szRegValue, int nVal);	// JG
BOOL GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize);
HRESULT SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos);
void ReadRegistrySettings(void);
void SetRegistryColour(int i, DWORD c);
void SetRegistryBrush(int i, DWORD c);
void SetRegistryAirspacePriority(int i);
void SetRegistryAirspaceMode(int i);
int GetRegistryAirspaceMode(int i);
void StoreType(int Index,int InfoType);

void SaveSoundSettings();
void SaveWindToRegistry();
void LoadWindFromRegistry();
void ReadDeviceSettings(const int devIdx, TCHAR *Name);
void WriteDeviceSettings(const int devIdx, const TCHAR *Name);

void SaveRegistryToFile(const TCHAR* szFile);
void LoadRegistryFromFile(const TCHAR* szFile);

#endif
