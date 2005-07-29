#if !defined(AFX_UTILS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_UTILS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "task.h"
#include "mapwindow.h"

BOOL GetFromRegistry(const TCHAR *szRegValue, DWORD *pPos);
HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos);
BOOL GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize);
HRESULT SetRegistryString(const TCHAR *szRegValue, TCHAR *Pos);
void ReadRegistrySettings(void);
void SetRegistryColour(int i, DWORD c);
void SetRegistryBrush(int i, DWORD c);
void StoreType(int Index,int InfoType);
void rotate(double *xin, double *yin, double angle);
void frotate(float *xin, float *yin, float angle);
double Distance(double lat1, double lon1, double lat2, double lon2);
double Bearing(double lat1, double lon1, double lat2, double lon2);
double Reciprocal(double InBound);
double BiSector(double InBound, double OutBound);
void SectorEndPoint(double StartLat, double StartLon, double  Radial, double Dist, double *EndLat, double *EndLon);
void CalculateNewPolarCoef(void);
int Registered(void);
void CheckRegistration(void);
void RefreshTaskWaypoint(int i);
void CalculateTaskSectors(void);
void CalculateAATTaskSectors(void);
double FindLattitude(double Lat, double Lon, double Bearing, double Distance);
double FindLongditude(double Lat, double Lon, double Bearing, double Distance);
void ConvertFlightLevels(void);
BOOL PolygonVisible(const POINT *lpPoints, int nCount, RECT rc);
void ReadPort1Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void ReadPort2Settings(DWORD *PortIndex, DWORD *SpeedIndex);
void WritePort1Settings(DWORD PortIndex, DWORD SpeedIndex);
void WritePort2Settings(DWORD PortIndex, DWORD SpeedIndex);
int  Circle(HDC hdc, long x, long y, int radius, RECT rc);
void ReadAssetNumber(void);
void WriteProfile(HWND hwnd, TCHAR *szFile);
void ReadProfile(HWND hwnd, TCHAR *szFile);
double ScreenAngle(int x1, int y1, int x2, int y2);
void ReadCompaqID(void);
void ReadUUID(void);
void FormatWarningString(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top, TCHAR *szMessageBuffer, TCHAR *TileBuffer );
BOOL ReadString(HANDLE hFile, int Max, TCHAR *String);
BOOL ReadStringX(FILE *fp, int Max, TCHAR *String);
void InitSineTable(void);
double fastcosine(double x);
double fastsine(double x);
float ffastcosine(float x);
float ffastsine(float x);
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


#endif
