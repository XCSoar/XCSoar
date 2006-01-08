// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include "aygshellpc.h"

#include <windows.h>

#include "options.h"

#include <ctype.h>
#include <commctrl.h>

#include <windows.h>


#include <stdlib.h>


#include <tchar.h>


#include <math.h>


#include <string.h>


#include <assert.h>





#define ASSERT(x) {}




#define NOCLEARTYPE





// define dummy's for unsupported API calls





//#define sndPlaySound(x, y)                  (1==1)


#define CommandBar_Destroy(x)               ((void)0)


#define SystemIdleTimerReset()              ((void)0)


#define GetSystemPowerStatusEx2(x, y, z)    (1==1)


#define KernelIoControl(x, y, z, u, v, w)   (1==1)


#define _strdup(x)                          strdup(x)





// define dummy's for unsupported Macros





//#define PROCESSOR_STRONGARM     0x12345
//#define PROCESSOR_ARM720        0x54321





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





//DWORD   GetCurrentProcessId(void);


long WINAPI     MulDiv32(long, long, long);



#include <tchar.h>

//#define _tcsclen(x) _tcslen(x)





//#define min(x, y)   (x < y ? x : y)
//#define max(x, y)   (x > y ? x : y)





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





// BCpp does not know a wide char version, this is the proto of the wrapper (see xcsoarps.cpp)


#define GetProcAddress GetProcAddressW


FARPROC GetProcAddressW(HMODULE hModule, LPCWSTR lpProcName);





//BOOL SHGetSpecialFolderPathW(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate);





// redefine WinMain


int WINAPI xcsoarWinMain(     HINSTANCE hInstance,


  HINSTANCE hPrevInstance,  LPTSTR    lpCmdLine,  int       nCmdShow);


#define WinMain xcsoarWinMain





#ifdef __cplusplus


}

#endif


// this module does NOTHING!
// it just simulate the aygSH interface to keep the compiler happy

// some macros to keep the compiler happy

#define SHCMBF_EMPTYBAR   0  //???
#define SHCMBF_HIDDEN     0  //???

#define SHIDIM_FLAGS      0

#define SHIDIF_DONEBUTTON       0x0001
// puts the OK button on the navigation bar
#define SHIDIF_SIZEDLG         0x0002
// sizes the dialog box, noting the current position of the input panel
#define SHIDIF_SIZEDLGFULLSCREEN   0x0004
// sizes the dialog box FULL screen, regardless of the position of the input panel
#define SHIDIF_SIPDOWN         0x0008
// puts the input panel down
#define SHIDIF_FULLSCREENNOMENUBAR   0x0010
// sizes the dialog box FULL screen. Does not leave room at the bottom for a menu bar


// missing library function dummy's

#define SHFullScreen(x, y)                    (1==1)
#define SHSetAppKeyWndAssoc(x, y)             (1==1)
#define SHHandleWMSettingChange(x, y, z, v)   (1==1)
#define SHHandleWMActivate(x, y, z, v, w)     (1==1)
#define SHCreateMenuBar(x)                    (1==1)
#define SHInitDialog(x)                       ((void)0)
#define SHSipPreference(hDlg,SIP_FORCEDOWN)   ((void)0)

// some typedefs to keep the compiler happy

typedef struct tagSHINITDIALOG{
  DWORD dwMask;
  HWND hDlg;
  DWORD dwFlags;
} SHINITDLGINFO, *PSHINITDLGINFO;

typedef struct {
  DWORD cbSize;
  HWND hwndLastFocus;
  UINT fSipUp :1;
  UINT fSipOnDeactivation :1;
  UINT fActive :1;
  UINT fReserved :29;
} SHACTIVATEINFO, *PSHACTIVATEINFO;

typedef struct tagSHMENUBARINFO{
  DWORD cbSize;
  HWND hwndParent;
  DWORD dwFlags;
  UINT nToolBarId;
  HINSTANCE hInstRes;
  int nBmpId;
  int cBmpImages;
  HWND hwndMB;
} SHMENUBARINFO, *PSHMENUBARINFO;




// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
