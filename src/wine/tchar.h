#ifndef XCSOAR_TCHAR_H
#define XCSOAR_TCHAR_H

#include_next <tchar.h>

#undef _T
#define _T(x) x

#endif
