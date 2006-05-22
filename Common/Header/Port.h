#if !defined(AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <windows.h>


BOOL Port1Initialize (LPTSTR,DWORD);
BOOL Port1Close (void);
void Port1Write (BYTE);
DWORD Port1ReadThread (LPVOID);
void Port1WriteString(TCHAR *Text);

BOOL Port2Initialize (LPTSTR,DWORD);
BOOL Port2Close (void);
void Port2Write (BYTE);
DWORD Port2ReadThread (LPVOID);
void Port2WriteString(TCHAR *Text);
void Port2WriteNMEA(TCHAR *Text);

BOOL Port1StopRxThread(void);
BOOL Port1StartRxThread(void);
int  Port1GetChar(void);
int  Port1SetRxTimeout(int Timeout);
unsigned long Port1SetBaudrate(unsigned long BaudRate);
int Port1Read(void *Buffer, size_t Size);
void Port1WriteNMEA(TCHAR *Text);

BOOL Port2StopRxThread(void);


void VarioWriteNMEA(TCHAR *Text);
void VarioWriteSettings(void);

#endif
