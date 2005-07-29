// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_)
#define AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_

#pragma option -WU
#define STRICT

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


// Windows Header Files:
#include <ctype.h>
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define ASSERT     assert

#define NOCLEARTYPE

// define dummy's for unsupported API calls

#define sndPlaySound(x, y)                  (1==1)
#define CommandBar_Destroy(x)               ((void)0)
#define SystemIdleTimerReset()              ((void)0)
#define GetSystemPowerStatusEx2(x, y, z)    (1==1)
#define KernelIoControl(x, y, z, u, v, w)   (1==1)
#define _strdup(x)                          strdup(x)

// define dummy's for unsupported Macros

#define PROCESSOR_STRONGARM     0x12345
#define PROCESSOR_ARM720        0x54321

#define IOCTL_HAL_GET_DEVICEID  1


// typedef's

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct _SYSTEM_POWER_STATUS_EX2 {
  BYTE ACLineStatus;
  BYTE BatteryFlag;
  BYTE BatteryLifePercent;
  BYTE Reserved1;
  DWORD BatteryLifeTime;
  DWORD BatteryFullLifeTime;
  BYTE Reserved2;
  BYTE BackupBatteryFlag;
  BYTE BackupBatteryLifePercent;
  BYTE Reserved3;
  DWORD BackupBatteryLifeTime;
  DWORD BackupBatteryFullLifeTime;
  WORD BatteryVoltage;
  DWORD BatteryCurrent;
  DWORD BatteryAverageCurrent;
  DWORD BatteryAverageInterval;
  DWORD BatterymAHourConsumed;
  DWORD BatteryTemperature;
  DWORD BackupBatteryVoltage;
  BYTE  BatteryChemistry;
} SYSTEM_POWER_STATUS_EX2, *PSYSTEM_POWER_STATUS_EX2, *LPSYSTEM_POWER_STATUS_EX2;


#include <fcntl.h>

DWORD WINAPI    GetCurrentProcessId(void);
long WINAPI     MulDiv32(long, long, long);

#define _tcsclen(x) _tcslen(x)

#define min(x, y)   (x < y ? x : y)
#define max(x, y)   (x > y ? x : y)

#define VK_APP1     0xC1
#define VK_APP2     0xC2
#define VK_APP3     0xC3
#define VK_APP4     0xC4
#define VK_APP5     0xC5
#define VK_APP6     0xC6

//#define WNDPROC     FARPROC

#ifdef __cplusplus
extern "C" {
#endif

// redefine WinMain
int WINAPI xcsoarWinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow);
#define WinMain xcsoarWinMain

#ifdef __cplusplus
}
#endif


#include "options.h"

#endif // !defined(AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_)
