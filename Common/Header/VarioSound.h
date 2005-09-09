
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the VARIOSOUND_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// VARIOSOUND_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef VARIOSOUND_DLL
#ifdef VARIOSOUND_EXPORTS
#define VARIOSOUND_API __declspec(dllexport)
#else
#define VARIOSOUND_API __declspec(dllimport)
#endif
#else
#define VARIOSOUND_API
#endif

BOOL PlayResource (LPTSTR lpName);


extern "C" {
VARIOSOUND_API void VarioSound_Init(void);
VARIOSOUND_API void VarioSound_SetV(short v);
VARIOSOUND_API void VarioSound_SetSTFMode(bool);
VARIOSOUND_API void VarioSound_SetVAlt(short v);
VARIOSOUND_API void VarioSound_EnableSound(bool);
VARIOSOUND_API void VarioSound_SetVdead(short v);
VARIOSOUND_API void VarioSound_Close(void);  // added sgi
  void VarioSound_SetSoundVolume(int vpercent);
}



/*
[DllImport("VarioSound.dll",EntryPoint="VarioSound_Init")]
public static extern void VarioSound_Init();

[DllImport("VarioSound.dll",EntryPoint="VarioSound_EnableSound")]
public static extern void VarioSound_EnableSound(bool v);

[DllImport("VarioSound.dll",EntryPoint="VarioSound_SetV")]
public static extern void VarioSound_SetV(short i);

[DllImport("VarioSound.dll",EntryPoint="VarioSound_SetVdead")]
public static extern void VarioSound_SetVdead(short v);

*/
