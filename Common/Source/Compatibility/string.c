#include "Compatibility/string.h"

#ifdef HAVE_MSVCRT

#include <stdlib.h>
#include <wchar.h>

int _wtoi(const wchar_t *ptr)
{
	return wcstol(ptr, NULL, 10);
}

#endif
