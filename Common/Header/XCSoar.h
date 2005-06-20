
#if !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "sizes.h"

typedef struct _SCREEN_INFO
{
	TCHAR Description[DESCRIPTION_SIZE +1];
	TCHAR Title[TITLE_SIZE + 1];
	TCHAR Format[FORMAT_SIZE + 1];
	double Value;
	void (*Process)(int UpDown);
} SCREEN_INFO;

void ProcessChar (char c);

#endif // !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
