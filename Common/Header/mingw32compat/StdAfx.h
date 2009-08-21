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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WINE__
	void __cdecl	TransparentImage(HDC, ...);

  // JMW _CRTIMP int __cdecl	SHSetAppKeyWndAssoc(int, HWND);
_CRTIMP void __cdecl	SystemIdleTimerReset(void);
#endif

#ifdef __cplusplus
}
#endif

#define CLEARTYPE_COMPAT_QUALITY 6

// Local Header Files

#include "options.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft eMbedded Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4F78E65A_F28C_412D_9648_C8F491327F80__INCLUDED_)
