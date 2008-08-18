#include "stdafx.h"
#include "XCSoar.h"

void StartupStore(TCHAR *Str); //rmk

void debug(const TCHAR *fmt, ...)
{
  TCHAR buf[80];
  va_list ap;

  va_start(ap, fmt);
  _vstprintf(buf, fmt, ap);
  va_end(ap);

  StartupStore(buf);
}


