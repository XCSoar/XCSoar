#if !defined(AFX_UTILS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_UTILS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <shlobj.h>
#include "task.h"
#include "mapwindow.h"

extern TCHAR szRegistryKey[];
extern TCHAR szRegistrySpeedUnitsValue[];
extern TCHAR szRegistryDistanceUnitsValue[];
extern TCHAR szRegistryAltitudeUnitsValue[];
extern TCHAR szRegistryLiftUnitsValue[];
extern TCHAR szRegistryDisplayUpValue[];
extern TCHAR szRegistryDisplayText[];   
extern TCHAR szRegistrySafetyAltitudeArrival[];
extern TCHAR szRegistrySafetyAltitudeBreakOff[];
extern TCHAR szRegistrySafetyAltitudeTerrain[];
extern TCHAR szRegistrySafteySpeed[];
extern TCHAR szRegistryFAISector[];
extern TCHAR szRegistrySectorRadius[];
extern TCHAR szRegistryPolarID[];
extern TCHAR szRegistryWayPointFile[];
extern TCHAR szRegistryAdditionalWayPointFile[];
extern TCHAR szRegistryAirspaceFile[];
extern TCHAR szRegistryAdditionalAirspaceFile[];
extern TCHAR szRegistryAirfieldFile[];
extern TCHAR szRegistryTopologyFile[];
extern TCHAR szRegistryPolarFile[];
extern TCHAR szRegistryTerrainFile[];
extern TCHAR szRegistryLanguageFile[];
extern TCHAR szRegistryStatusFile[];
extern TCHAR szRegistryInputFile[];
extern TCHAR szRegistryAltMode[];
extern TCHAR szRegistryClipAlt[];
extern TCHAR szRegistryAltMargin[];
extern TCHAR szRegistryRegKey[];
extern TCHAR szRegistrySnailTrail[];
extern TCHAR szRegistryDrawTopology[];
extern TCHAR szRegistryDrawTerrain[];
extern TCHAR szRegistryFinalGlideTerrain[];
extern TCHAR szRegistryStartLine[];
extern TCHAR szRegistryStartRadius[];
extern TCHAR szRegistryFinishLine[];
extern TCHAR szRegistryFinishRadius[];
extern TCHAR szRegistryAirspaceWarning[];
extern TCHAR szRegistryAirspaceBlackOutline[];
extern TCHAR szRegistryWarningTime[];
extern TCHAR szRegistryAcknowledgementTime[];
extern TCHAR szRegistryCircleZoom[];
extern TCHAR szRegistryWindUpdateMode[];        
extern TCHAR szRegistryHomeWaypoint[];        
extern TCHAR szRegistryPilotName[];        
extern TCHAR szRegistryAircraftType[];        
extern TCHAR szRegistryAircraftRego[];        
extern TCHAR szRegistryNettoSpeed[];        
extern TCHAR szRegistryCDICruise[];
extern TCHAR szRegistryCDICircling[];
extern TCHAR szRegistryAutoBlank[];
extern TCHAR szRegistryVarioGauge[];
extern TCHAR szRegistryDebounceTimeout[];
extern TCHAR szRegistryAppDefaultMapWidth[];
extern TCHAR szRegistryAppIndFinalGlide[];
extern TCHAR szRegistryAppIndLandable[];
extern TCHAR szRegistryAppInverseInfoBox[];
extern TCHAR szRegistryAppInfoBoxColors[];
extern TCHAR szRegistryAppGaugeVarioSpeedToFly[];
extern TCHAR szRegistryAppGaugeVarioAvgText[];
extern TCHAR szRegistryAppGaugeVarioMc[];
extern TCHAR szRegistryAppGaugeVarioBugs[];
extern TCHAR szRegistryAppGaugeVarioBallast[];
extern TCHAR szRegistryAppCompassAppearance[];
extern TCHAR szRegistryAppStatusMessageAlignment[];
extern TCHAR szRegistryAutoAdvance[];
extern TCHAR szRegistryUTCOffset[];
extern TCHAR szRegistryBlockSTF[];
extern TCHAR szRegistryAutoZoom[];
extern TCHAR szRegistryMenuTimeout[];
extern TCHAR szRegistryLockSettingsInFlight[];
extern TCHAR szRegistryTerrainContrast[];
extern TCHAR szRegistryTerrainBrightness[];
extern TCHAR szRegistryEnableFLARMDisplay[];

extern bool LockSettingsInFlight;

BOOL GetFromRegistry(const TCHAR *szRegValue, DWORD *pPos);
HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos);
HRESULT SetToRegistry(const TCHAR *szRegValue, bool bVal);	// JG
HRESULT SetToRegistry(const TCHAR *szRegValue, int nVal);	// JG
BOOL GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize);
HRESULT SetRegistryString(const TCHAR *szRegValue, TCHAR *Pos);
void ReadRegistrySettings(void);
void SetRegistryColour(int i, DWORD c);
void SetRegistryBrush(int i, DWORD c);
void SetRegistryAirspaceMode(int i);
int GetRegistryAirspaceMode(int i);
void StoreType(int Index,int InfoType);
void rotate(double &xin, double &yin, const double angle);
void frotate(float &xin, float &yin, const float angle);
double Distance(double lat1, double lon1, double lat2, double lon2);
double Bearing(double lat1, double lon1, double lat2, double lon2);
double Reciprocal(double InBound);
double BiSector(double InBound, double OutBound);
void SectorEndPoint(double StartLat, double StartLon, double  Radial, double Dist, double *EndLat, double *EndLon);
void CalculateNewPolarCoef(void);
double FindLatitude(double Lat, double Lon, double Bearing, double Distance);
double FindLongitude(double Lat, double Lon, double Bearing, double Distance);
void ConvertFlightLevels(void);
BOOL PolygonVisible(const POINT *lpPoints, int nCount, RECT rc);
void ReadPort1Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void ReadPort2Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void WritePort1Settings(DWORD PortIndex, DWORD SpeedIndex);
void WritePort2Settings(DWORD PortIndex, DWORD SpeedIndex);
int  Circle(HDC hdc, long x, long y, int radius, RECT rc);
int Segment(HDC hdc, long x, long y, int radius, RECT rc, 
	    double start,
	    double end);
void ReadAssetNumber(void);
void WriteProfile(TCHAR *szFile);
void ReadProfile(TCHAR *szFile);
double ScreenAngle(int x1, int y1, int x2, int y2);
void ReadCompaqID(void);
void ReadUUID(void);
void FormatWarningString(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top, TCHAR *szMessageBuffer, TCHAR *TileBuffer );
BOOL ReadString(HANDLE hFile, int Max, TCHAR *String);
BOOL ReadStringX(FILE *fp, int Max, TCHAR *String);
void InitSineTable(void);
double fastcosine(const double &x);
double fastsine(const double &x);
float ffastcosine(const float &x);
float ffastsine(const float &x);
double StrToDouble(TCHAR *Source, TCHAR **Stop);
void PExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);
void SaveWindToRegistry();
void LoadWindFromRegistry();
void SaveSoundSettings();
void ReadDeviceSettings(int devIdx, TCHAR *Name);
void WriteDeviceSettings(int devIdx, TCHAR *Name);

unsigned int isqrt4(unsigned long val);
int iround(double i);
long lround(double i);

WORD crcCalc(void *Buffer, size_t size);
void ExtractDirectory(TCHAR *Dest, TCHAR *Source);
double DoSunEphemeris(double lon, double lat);

void *bsearch(void *key, void *base0, size_t nmemb, size_t size, int (*compar)(const void *elem1, const void *elem2));
TCHAR *strtok_r(TCHAR *s, TCHAR *delim, TCHAR **lasts);


void ResetInfoBoxes(void);

void SaveRegistryToFile(TCHAR* szFile); 
void LoadRegistryFromFile(TCHAR* szFile); 

/* =====================================================
   Interface Files !
   ===================================================== */

void ReadLanguageFile(void);
void ReadStatusFile(void);
void StatusFileInit(void);
void _init_Status(int num);

typedef struct {
	TCHAR *key;		/* English key */
	TCHAR *sound;		/* What sound entry to play */
	TCHAR *nmea_gps;		/* NMEA Sentence - to GPS serial */
	TCHAR *nmea_vario;		/* NMEA Sentence - to Vario serial */
	bool doStatus;
	bool doSound;
	int delay_ms;		/* Delay for DoStatusMessage */
	int iFontHeightRatio;	// TODO - not yet used
	bool docenter;		// TODO - not yet used
	int *TabStops;		// TODO - not yet used
	int disabled;		/* Disabled - currently during run time */
} StatusMessageSTRUCT;

typedef struct {
	TCHAR *key;
	TCHAR *text;
} GetTextSTRUCT;


// Size of Status message cache - Note 1000 messages may not be enough...
// TODO If we continue with the reading one at a time - then consider using
// a pointer structure and build on the fly, thus no limit, but also only
// RAM used as required - easy to do with struct above - just point to next.
// (NOTE: This is used for all the caches for now - temporary)
#define MAXSTATUSMESSAGECACHE 1000

// Parse string (new lines etc) and malloc the string
TCHAR* StringMallocParse(TCHAR* old_string);

TCHAR* LocalPath(TCHAR* file = TEXT(""), int loc = CSIDL_PERSONAL);

void ConvertTToC(CHAR* pszDest, const TCHAR* pszSrc);
void ConvertCToT(TCHAR* pszDest, const CHAR* pszSrc);

void propGetFontSettings(TCHAR *Name, LOGFONT* lplf);
int propGetScaleList(double *List, size_t Size);

long GetUTCOffset(void);
int TextToLineOffsets(TCHAR* text, int* LineOffsets, int maxLines);
void RestoreRegistry(void);
void StoreRegistry(void);
void XCSoarGetOpts(LPTSTR CommandLine);

bool CheckRectOverlap(RECT rc1, RECT rc2);

#endif
