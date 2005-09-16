#ifndef XCSEXAMPLEPLUGIN_H
#define XCSEXAMPLEPLUGIN_H

extern "C" {

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the XCSEXAMPLEPLUGIN_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// XCSEXAMPLEPLUGIN_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef XCSEXAMPLEPLUGIN_EXPORTS
#define XCSEXAMPLEPLUGIN_API __declspec(dllexport)
#else
#define XCSEXAMPLEPLUGIN_API __declspec(dllimport)
#endif

// This class is exported from the XCSExamplePlugin.dll
XCSEXAMPLEPLUGIN_API void DemoSound(TCHAR* misc);

XCSEXAMPLEPLUGIN_API void XCSAPI_SetHInst(HMODULE);

}

#endif
