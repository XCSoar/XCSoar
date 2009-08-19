#ifndef XCSOAR_COMPATIBILITY_STRING_H
#define XCSOAR_COMPATIBILITY_STRING_H

#include <tchar.h>
#include <ctype.h>

#ifndef HAVE_MSVCRT

static inline char *
_strupr(char *p)
{
  char *q;

  for (q = p; *q != 0; ++q)
    *q = toupper(*q);

  return p;
}

#define _istalnum isalnum
#define towupper toupper
#define _strnicmp strncasecmp
#define _strdup strdup

#else /* !HAVE_MSVCRT */

#define _tcsclen(x) _tcslen(x)

#ifdef __cplusplus
extern "C" {
#endif

_CRTIMP int __cdecl     _wtoi (const wchar_t *);

#ifdef __cplusplus
}
#endif

#endif /* HAVE_MSVCRT */

#endif
