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
void Port1WriteString(const TCHAR *Text);
int  Port1SetRxTimeout(int Timeout);
unsigned long Port1SetBaudrate(unsigned long BaudRate);
BOOL Port1StopRxThread(void);
BOOL Port1StartRxThread(void);
void Port1Flush(void);

int  Port1GetChar(void);
int Port1Read(void *Buffer, size_t Size);

BOOL Port2Initialize (LPTSTR,DWORD);
BOOL Port2Close (void);
void Port2Write (BYTE);
DWORD Port2ReadThread (LPVOID);
void Port2WriteString(const TCHAR *Text);
int  Port2SetRxTimeout(int Timeout);
unsigned long Port2SetBaudrate(unsigned long BaudRate);
BOOL Port2StartRxThread(void);
BOOL Port2StopRxThread(void);

int  Port2GetChar(void);
int Port2Read(void *Buffer, size_t Size);
void Port2Flush(void);

#endif
