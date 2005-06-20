#if !defined(AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <windows.h>


BOOL PortInitialize (LPTSTR,DWORD);
BOOL PortClose (HANDLE);
void PortWrite (BYTE);
DWORD PortReadThread (LPVOID);
void PortWriteString(TCHAR *Text);




#endif