#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "XCSoar.h"

#include <stdio.h>
#include <stdarg.h>

void DebugStore(const char *Str, ...)
{
#if !defined(NDEBUG) && !defined(GNAV)
  char buf[MAX_PATH];
  va_list ap;
  int len;

  va_start(ap, Str);
  len = vsprintf(buf, Str, ap);
  va_end(ap);

  LockFlightData();
  FILE *stream;
  TCHAR szFileName[] = TEXT("xcsoar-debug.log");
  static bool initialised = false;
  if (!initialised) {
    initialised = true;
    stream = _wfopen(szFileName,TEXT("w"));
  } else {
    stream = _wfopen(szFileName,TEXT("a+"));
  }

  fwrite(buf,len,1,stream);

  fclose(stream);
  UnlockFlightData();
#endif
}

void StartupStore(const TCHAR *Str, ...)
{
  TCHAR buf[MAX_PATH];
  va_list ap;

  va_start(ap, Str);
  _vstprintf(buf, Str, ap);
  va_end(ap);

  if (csFlightDataInitialized) {
    LockFlightData();
  }
  FILE *startupStoreFile = NULL;
  static TCHAR szFileName[MAX_PATH];
  static bool initialised = false;
  if (!initialised) {
#ifdef GNAV
    LocalPath(szFileName, TEXT("persist/xcsoar-startup.log"));
#else
    LocalPath(szFileName, TEXT("xcsoar-startup.log"));
#endif
    startupStoreFile = _tfopen(szFileName, TEXT("wb"));
    if (startupStoreFile) {
      fclose(startupStoreFile);
    }
    initialised = true;
  }
  startupStoreFile = _tfopen(szFileName, TEXT("ab+"));
  if (startupStoreFile != NULL) {
    fprintf(startupStoreFile, "%S", buf);
    fclose(startupStoreFile);
  }
  if (csFlightDataInitialized) {
    UnlockFlightData();
  }
}
