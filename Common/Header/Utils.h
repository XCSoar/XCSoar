#if !defined(AFX_UTILS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_UTILS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <shlobj.h>
#include <math.h>
#include "task.h"
#include "Airspace.h"
#include <zzip/lib.h>

#define  POLARUSEWINPILOTFILE  6    // if this polat is selected use the winpilot file

extern TCHAR szRegistryKey[];
extern TCHAR szRegistrySpeedUnitsValue[];
extern TCHAR szRegistryDistanceUnitsValue[];
extern TCHAR szRegistryAltitudeUnitsValue[];
extern TCHAR szRegistryLiftUnitsValue[];
extern TCHAR szRegistryTaskSpeedUnitsValue[];
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
extern TCHAR szRegistryAutoWind[];
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
extern TCHAR szRegistryTeamcodeRefWaypoint[];
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
extern TCHAR szRegistryAppGaugeVarioGross[];
extern TCHAR szRegistryAppCompassAppearance[];
extern TCHAR szRegistryAppStatusMessageAlignment[];
extern TCHAR szRegistryAppInfoBoxBorder[];
extern TCHAR szRegistryAppAveNeedle[];
extern TCHAR szRegistryAutoAdvance[];
extern TCHAR szRegistryUTCOffset[];
extern TCHAR szRegistryBlockSTF[];
extern TCHAR szRegistryAutoZoom[];
extern TCHAR szRegistryMenuTimeout[];
extern TCHAR szRegistryLockSettingsInFlight[];
extern TCHAR szRegistryTerrainContrast[];
extern TCHAR szRegistryTerrainBrightness[];
extern TCHAR szRegistryTerrainRamp[];
extern TCHAR szRegistryEnableFLARMDisplay[];
extern TCHAR szRegistryFLARMGaugeBearing[];
extern TCHAR szRegistrySnailTrail[];
extern TCHAR szRegistryTrailDrift[];
extern TCHAR szRegistryThermalLocator[];
extern TCHAR szRegistryGliderScreenPosition[];
extern TCHAR szRegistryAnimation[];
extern TCHAR szRegistrySetSystemTimeFromGPS[];
extern TCHAR szRegistryAutoForceFinalGlide[];

extern TCHAR szRegistryVoiceClimbRate[];
extern TCHAR szRegistryVoiceTerrain[];
extern TCHAR szRegistryVoiceWaypointDistance[];
extern TCHAR szRegistryVoiceTaskAltitudeDifference[];
extern TCHAR szRegistryVoiceMacCready[];
extern TCHAR szRegistryVoiceNewWaypoint[];
extern TCHAR szRegistryVoiceInSector[];
extern TCHAR szRegistryVoiceAirspace[];

extern TCHAR szRegistryFinishMinHeight[];
extern TCHAR szRegistryStartMaxHeight[];
extern TCHAR szRegistryStartMaxSpeed[];

extern TCHAR szRegistryEnableNavBaroAltitude[];

extern TCHAR szRegistryLoggerTimeStepCruise[];
extern TCHAR szRegistryLoggerTimeStepCircling[];

extern TCHAR szRegistrySafetyMacCready[];
extern TCHAR szRegistryAbortSafetyUseCurrent[];
extern TCHAR szRegistryAutoMcMode[];
extern TCHAR szRegistryWaypointsOutOfRange[];
extern TCHAR szRegistryEnableExternalTriggerCruise[];
extern TCHAR szRegistryFAIFinishHeight[];
extern TCHAR szRegistryOLCRules[];
extern TCHAR szRegistryHandicap[];
extern TCHAR szRegistrySnailWidthScale[];
extern TCHAR szRegistryLatLonUnits[];
extern TCHAR szRegistryUserLevel[];
extern TCHAR szRegistryRiskGamma[];
extern TCHAR szRegistryWindArrowStyle[];
extern TCHAR szRegistryDisableAutoLogger[];
extern TCHAR szRegistryMapFile[];

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
void SetRegistryAirspacePriority(int i);
void SetRegistryAirspaceMode(int i);
int GetRegistryAirspaceMode(int i);
void StoreType(int Index,int InfoType);
void irotate(int &xin, int &yin, const double &angle);
void irotatescale(int &xin, int &yin, const double &angle, const double &scale,
                  double &x, double &y);
void protate(POINT &pin, const double &angle);
void protateshift(POINT &pin, const double &angle, const int &x, const int &y);
void rotate(double &xin, double &yin, const double &angle);
void frotate(float &xin, float &yin, const float &angle);
void rotatescale(double &xin, double &yin, const double &angle, const double &scale);
void frotatescale(float &xin, float &yin, const float &angle, const float &scale);

void DistanceBearing(double lat1, double lon1, double lat2, double lon2,
                     double *Distance, double *Bearing);

double Reciprocal(double InBound);
double BiSector(double InBound, double OutBound);
double HalfAngle(double Angle0, double Angle1);
void SectorEndPoint(double StartLat, double StartLon, double  Radial, double Dist, double *EndLat, double *EndLon);
void CalculateNewPolarCoef(void);
void FindLatitudeLongitude(double Lat, double Lon, 
                           double Bearing, double Distance, 
                           double *lat_out, double *lon_out);
void ConvertFlightLevels(void);
BOOL PolygonVisible(const POINT *lpPoints, int nCount, RECT rc);
void ReadPort1Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void ReadPort2Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void WritePort1Settings(DWORD PortIndex, DWORD SpeedIndex);
void WritePort2Settings(DWORD PortIndex, DWORD SpeedIndex);
int  Circle(HDC hdc, long x, long y, int radius, RECT rc, bool clip=false,
            bool fill=true);
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
BOOL ReadString(ZZIP_FILE* zFile, int Max, TCHAR *String);
BOOL ReadString(HANDLE hFile, int Max, TCHAR *String);
BOOL ReadStringX(FILE *fp, int Max, TCHAR *String);

// Fast trig functions
void InitSineTable(void);

extern double COSTABLE[4096];
extern double SINETABLE[4096];
extern double INVCOSINETABLE[4096];
extern int ISINETABLE[4096];
extern int ICOSTABLE[4096];

#define DEG_TO_INT(x) ((unsigned short)((x)*(65536.0/360.0)))>>4

#define invfastcosine(x) INVCOSINETABLE[DEG_TO_INT(x)]
#define ifastcosine(x) ICOSTABLE[DEG_TO_INT(x)]
#define ifastsine(x) ISINETABLE[DEG_TO_INT(x)]
#define fastcosine(x) COSTABLE[DEG_TO_INT(x)]
#define fastsine(x) SINETABLE[DEG_TO_INT(x)]

double StrToDouble(TCHAR *Source, TCHAR **Stop);
void PExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);
void SaveWindToRegistry();
void LoadWindFromRegistry();
void SaveSoundSettings();
void ReadDeviceSettings(int devIdx, TCHAR *Name);
void WriteDeviceSettings(int devIdx, TCHAR *Name);

unsigned int isqrt4(unsigned long val);


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



// Parse string (new lines etc) and malloc the string
TCHAR* StringMallocParse(TCHAR* old_string);

void LocalPath(TCHAR* buf, TCHAR* file = TEXT(""), int loc = CSIDL_PERSONAL);
void LocalPathS(char* buf, TCHAR* file = TEXT(""), int loc = CSIDL_PERSONAL);

void ExpandLocalPath(TCHAR* filein);
void ContractLocalPath(TCHAR* filein);

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
int MeasureCPULoad();

TCHAR* GetWinPilotPolarInternalName(int i);

void SetSourceRectangle(RECT fromRect);
RECT WINAPI DrawWireRects(LPRECT lprcTo, UINT nMilliSecSpeed);

void OpenFLARMDetails();
void CloseFLARMDetails();
TCHAR* LookupFLARMDetails(long id);

double FindQNH(double alt_raw, double alt_known);
double AltitudeToQNHAltitude(double alt);
double StaticPressureToAltitude(double ps);
double AirDensity(double altitude);
double AirDensityRatio(double altitude);

double HexStrToDouble(TCHAR *Source, TCHAR **Stop); 

long CheckFreeRam(void);

void MemCheckPoint();
void MemLeakCheck();
void MyCompactHeaps();
unsigned long FindFreeSpace(TCHAR *path);
bool MatchesExtension(TCHAR *filename, TCHAR* extension);
BOOL PlayResource (LPTSTR lpName);
void CreateDirectoryIfAbsent(TCHAR *filename);

void InterfaceTimeoutReset(void);
bool InterfaceTimeoutCheck(void);

#ifdef __cplusplus
extern "C"{
#endif

bool FileExistsW(TCHAR *FileName);
bool FileExistsA(char *FileName);

#ifdef _UNICODE
#define FileExists FileExistsW
#else
#define FileExists FileExistsA
#endif

#ifdef __cplusplus
}
#endif


//2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor
//16.16 fixed point representation,

// =================================================================================
// Real2Int
// =================================================================================
inline int Real2Int(double val)
{
#if (WINDOWS_PC>0)
  val += 68719476736.0*1.5;
  return *((long*)&val) >> 16; 
#else
  return (int)val;
#endif
}


// =================================================================================
// Real2Int
// =================================================================================
inline int Real2Int(float val)
{
#if (WINDOWS_PC>0)
  return Real2Int ((double)val);
#else
  return (int)val;
#endif
}


inline int iround(double i) {
    return Real2Int(floor(i+0.5));
}

inline long lround(double i) {
    return (long)(floor(i+0.5));
}

inline unsigned int CombinedDivAndMod(unsigned int &lx) {
  unsigned int ox = lx & 0xff;
  // JMW no need to check max since overflow will result in 
  // beyond max dimensions
  lx = lx>>8;
  return ox;
}

bool RotateScreen(void);

bool AngleInRange(double Angle0, double Angle1, double x);
double AngleLimit180(double theta);
double AngleLimit360(double theta);


#endif
