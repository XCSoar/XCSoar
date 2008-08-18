#include <stdlib.h>
#include <wchar.h>

int _wtoi(const wchar_t *ptr)
{
	return wcstol(ptr, NULL, 10);
}

