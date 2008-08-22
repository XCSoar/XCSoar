// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_)
#define AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <math.h>
#include <malloc.h>

#define ASSERT(x) assert(x)

typedef unsigned int uint;
typedef unsigned char byte;

#define _tcsclen(x) _tcslen(x)

#ifdef __cplusplus
extern "C" {
#endif

_CRTIMP int __cdecl     _wtoi (const wchar_t *);
	void __cdecl	TransparentImage(HDC, ...);

  // JMW _CRTIMP int __cdecl	SHSetAppKeyWndAssoc(int, HWND);
_CRTIMP void __cdecl	SystemIdleTimerReset(void);

#ifdef __cplusplus
}
#endif

#define CLEARTYPE_COMPAT_QUALITY 6

#ifdef GNAV
#undef VK_APP1
#define VK_APP1 VK_F1 
#undef VK_APP2
#define VK_APP2 VK_F2 
#undef VK_APP3
#define VK_APP3 VK_F3 
#undef VK_APP4
#define VK_APP4 VK_F4
#undef VK_APP5
#define VK_APP5 VK_F5 
#undef VK_APP6
#define VK_APP6 VK_F6 
#else
#if (WINDOWSPC>0)
#define VK_APP1     0x31
#define VK_APP2     0x32
#define VK_APP3     0x33
#define VK_APP4     0x34
#define VK_APP5     0x35
#define VK_APP6     0x36
#else
#define VK_APP1     0xC1 
#define VK_APP2     0xC2 
#define VK_APP3     0xC3
#define VK_APP4     0xC4
#define VK_APP5     0xC5
#define VK_APP6     0xC6
#endif
#endif

// Local Header Files

#include "options.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft eMbedded Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_)
