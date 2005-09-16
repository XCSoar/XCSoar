// XCSExamplePlugin.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "XCSExamplePlugin.h"
#include <Mmsystem.h>
// #include "Message.h"

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	/*
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
	*/
    return TRUE;
}

typedef void (CALLBACK *DLLFUNC)(TCHAR*, TCHAR*);
HMODULE hinst;

XCSEXAMPLEPLUGIN_API void DemoSound(TCHAR *misc) {
	sndPlaySound(misc, SND_ASYNC | SND_FILENAME);

	DLLFUNC lpfnDLLProc = (DLLFUNC)GetProcAddress(hinst, TEXT("DoStatusMessage"));
	if (lpfnDLLProc)
		lpfnDLLProc(TEXT("Testing from DLL"), TEXT(""));
}

XCSEXAMPLEPLUGIN_API void XCSAPI_SetHInst(HMODULE h) {
	hinst = h;
}
