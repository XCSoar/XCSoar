/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "XCSoar.h"
#include "Protection.hpp"
#include "Interface.hpp"
#include "Device/Port.h"
#include "DeviceBlackboard.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Device/device.h"
#include "Message.h"
#include <windows.h>
#include <tchar.h>

#define COMMDEBUG 0

static void ComPort_StatusMessage(UINT type, const TCHAR *caption, const TCHAR *fmt, ...)
{
  TCHAR tmp[127];
  va_list ap;

  va_start(ap, fmt);
  _vsntprintf(tmp, 127, fmt, ap);
  va_end(ap);

  tmp[126] = _T('\0');

  if (caption)
    MessageBoxX(tmp, gettext(caption), type);
  else
    Message::AddMessage(tmp);
}

ComPort::ComPort(struct DeviceDescriptor *d)
{
  hReadThread = NULL;
  CloseThread = 0;
  fRxThreadTerminated = TRUE;
  dwMask = 0;
  hPort = INVALID_HANDLE_VALUE;
  BuildingString[0] = 0;
  bi = 0;
  dev = d;
}

BOOL ComPort::Initialize(LPCTSTR lpszPortName, DWORD dwPortSpeed)
{
  DWORD dwError;
  DCB PortDCB;

  if (lpszPortName) {
    _tcscpy(sPortName, lpszPortName);
  }

  // Open the serial port.
  hPort = CreateFile(sPortName, // Pointer to the name of the port
                      GENERIC_READ | GENERIC_WRITE,
                                    // Access (read-write) mode
                      0,            // Share mode
                      NULL,         // Pointer to the security attribute
                      OPEN_EXISTING,// How to open the serial port
                      FILE_ATTRIBUTE_NORMAL,            // Port attributes
                      NULL);        // Handle to port with attribute
                                    // to copy

  // If it fails to open the port, return FALSE.
  if (hPort == INVALID_HANDLE_VALUE) {
    dwError = GetLastError();

    // Could not open the port.
    // TODO code: SCOTT I18N - Fix this to sep the TEXT from PORT, TEXT can be
    // gettext(), port added on new line
    ComPort_StatusMessage(MB_OK|MB_ICONINFORMATION, NULL, TEXT("%s %s"),
              gettext(TEXT("Unable to open port")), sPortName);
    return FALSE;
  }

  PortDCB.DCBlength = sizeof(DCB);

  // Get the default port setting information.
  GetCommState(hPort, &PortDCB);

  // Change the DCB structure settings.
  PortDCB.BaudRate = dwPortSpeed;       // Current baud
  PortDCB.fBinary = TRUE;               // Binary mode; no EOF check
  PortDCB.fParity = TRUE;               // Enable parity checking
  PortDCB.fOutxCtsFlow = FALSE;         // No CTS output flow control
  PortDCB.fOutxDsrFlow = FALSE;         // No DSR output flow control
  PortDCB.fDtrControl = DTR_CONTROL_ENABLE;
                                        // DTR flow control type
  PortDCB.fDsrSensitivity = FALSE;      // DSR sensitivity
  PortDCB.fTXContinueOnXoff = TRUE;     // XOFF continues Tx
  PortDCB.fOutX = FALSE;                // No XON/XOFF out flow control
  PortDCB.fInX = FALSE;                 // No XON/XOFF in flow control
  PortDCB.fErrorChar = FALSE;           // Disable error replacement
  PortDCB.fNull = FALSE;                // Disable null removal
  PortDCB.fRtsControl = RTS_CONTROL_ENABLE;
                                        // RTS flow control

  PortDCB.fAbortOnError = TRUE;         // JMW abort reads/writes on
                                        // error, was FALSE

  PortDCB.ByteSize = 8;                 // Number of bits/byte, 4-8
  PortDCB.Parity = NOPARITY;            // 0-4=no,odd,even,mark,space
  PortDCB.StopBits = ONESTOPBIT;        // 0,1,2 = 1, 1.5, 2

  PortDCB.EvtChar = '\n'; // wait for end of line

  // Configure the port according to the specifications of the DCB
  // structure.
  if (!SetCommState(hPort, &PortDCB)) {
    // Could not create the read thread.
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;
#ifdef WINDOWSPC
    Sleep(2000); // needed for windows bug
#endif
    // TODO code: SCOTT I18N - Fix this to sep the TEXT from PORT, TEXT can be
    // gettext(), port added on new line
    ComPort_StatusMessage(MB_OK, TEXT("Error"), TEXT("%s %s"),
              gettext(TEXT("Unable to Change Settings on Port")), sPortName);
    dwError = GetLastError();
    return FALSE;
  }

  //  SetRxTimeout(10); // JMW20070515 wait a maximum of 10ms
  SetRxTimeout(0);

  SetupComm(hPort, 1024, 1024);

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  // SETRTS: Sends the RTS (request-to-send) signal.
  EscapeCommFunction(hPort, SETDTR);
  EscapeCommFunction(hPort, SETRTS);

  if (!StartRxThread()){
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;
#ifdef WINDOWSPC
    Sleep(2000); // needed for windows bug
#endif
    return FALSE;
  }

  return TRUE;
}


/***********************************************************************

  PortWrite (BYTE Byte)

***********************************************************************/
void ComPort::PutChar(BYTE Byte)
{
  if (hPort == INVALID_HANDLE_VALUE)
    return;

  DWORD dwError, dwNumBytesWritten;

  if (!WriteFile(hPort,              // Port handle
                 &Byte,               // Pointer to the data to write
                 1,                   // Number of bytes to write
                 &dwNumBytesWritten,  // Pointer to the number of bytes
                                      // written
                 (OVERLAPPED *)NULL)) // Must be NULL for Windows CE
  {
    // WriteFile failed. Report error.
    dwError = GetLastError();
  }
}


void ComPort::Flush(void)
{
  PurgeComm(hPort,
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

DWORD WINAPI ComPort::ThreadProc(LPVOID prt)
{
  ComPort *port = (ComPort *)prt;
  port->ReadThread();
  return 0;
}

/***********************************************************************

  PortReadThread (LPVOID lpvoid)

***********************************************************************/
DWORD ComPort::ReadThread()
{
  DWORD dwCommModemStatus, dwBytesTransferred;
  BYTE inbuf[1024];

  // JMW added purging of port on open to prevent overflow
  Flush();

  // Specify a set of events to be monitored for the port.

  dwMask = EV_RXFLAG | EV_CTS | EV_DSR | EV_RING | EV_RXCHAR;

#if !defined(WINDOWSPC) || (WINDOWSPC == 0)
  SetCommMask(hPort, dwMask);
#endif

  fRxThreadTerminated = FALSE;

  while ((hPort != INVALID_HANDLE_VALUE) &&
	 (!closeTriggerEvent.test()) && (!CloseThread))
  {

#ifdef WINDOWSPC
    Sleep(50);  // ToDo rewrite the whole driver to use overlaped IO
                // on W2K or higher
#else
    // Wait for an event to occur for the port.
    if (!WaitCommEvent(hPort, &dwCommModemStatus, 0)) {
      // error reading from port
      Sleep(100);
    }
#endif

    // Re-specify the set of events to be monitored for the port.
    //    SetCommMask(hPort, dwMask1);

#if !defined(WINDOWSPC) || (WINDOWSPC == 0)
    if ((dwCommModemStatus & EV_RXFLAG) || (dwCommModemStatus & EV_RXCHAR))
#endif
    {

      // Loop for waiting for the data.
      do {
        dwBytesTransferred = 0;
              // Read the data from the serial port.
        if (ReadFile(hPort, inbuf, 1024, &dwBytesTransferred,
		     (OVERLAPPED *)NULL)) {
	  if (globalRunningEvent.test())  // ignore everything until started
	    for (unsigned int j = 0; j < dwBytesTransferred; j++) {
	      ProcessChar(inbuf[j]);
	    }
        } else {
          dwBytesTransferred = 0;
        }

        Sleep(50); // JMW20070515: give port some time to
                   // fill... prevents ReadFile from causing the
                   // thread to take up too much CPU

	if (CloseThread)
	  dwBytesTransferred = 0;
      } while (dwBytesTransferred != 0);
    }

    // give port some time to fill
    Sleep(5);

    // Retrieve modem control-register values.
    GetCommModemStatus(hPort, &dwCommModemStatus);
  }

  Flush();

  fRxThreadTerminated = TRUE;

  return 0;
}


/***********************************************************************

  PortClose()

***********************************************************************/
BOOL ComPort::Close()
{
  DWORD dwError;

  if (hPort != INVALID_HANDLE_VALUE) {
    StopRxThread();
    Sleep(100);  // todo ...

    dwError = 0;

    // Close the communication port.
    if (!CloseHandle(hPort)) {
      dwError = GetLastError();
      return FALSE;
    } else {
#ifdef WINDOWSPC
      Sleep(2000); // needed for windows bug
#endif
      hPort = INVALID_HANDLE_VALUE;
      return TRUE;
    }
  }

  return FALSE;
}



void ComPort::WriteString(const TCHAR *Text)
{
   char tmp[512];
   DWORD written, error;

   if (hPort == INVALID_HANDLE_VALUE)
     return;

   int len = _tcslen(Text);

#ifdef _UNICODE
   len = WideCharToMultiByte(CP_ACP, 0, Text, len + 1, tmp, sizeof(tmp), NULL, NULL);
#else
   strcpy(tmp, Text);
   len = strlen(tmp);
#endif

   // don't write trailing '\0' to device
   if (--len<=0 || !WriteFile(hPort, tmp, len, &written, NULL))
     // WriteFile failed, report error
     error = GetLastError();
}


// Stop Rx Thread
// return: TRUE on success, FALSE on error
BOOL ComPort::StopRxThread()
{
  if (hPort == INVALID_HANDLE_VALUE)
    return FALSE;
  if (fRxThreadTerminated)
    return TRUE;

  CloseThread = TRUE;

  DWORD tm = GetTickCount()+20000l;
#ifdef WINDOWSPC
  while (!fRxThreadTerminated && (long)(tm-GetTickCount()) > 0) {
    Sleep(10);
  }
  if (!fRxThreadTerminated) {
    TerminateThread(hReadThread, 0);
  } else {
    CloseHandle(hReadThread);
  }
#else
  Flush();
  // setting the comm event mask with the same value
  //  GetCommMask(hPort, &dwMask);
  SetCommMask(hPort, dwMask);          // will cancel any
                                        // WaitCommEvent!  this is a
                                        // documented CE trick to
                                        // cancel the WaitCommEvent
  while (!fRxThreadTerminated && (long)(GetTickCount()-tm) < 1000){
    Sleep(10);
  }
  if (!fRxThreadTerminated) {
//#if COMMDEBUG > 0
    ComPort_StatusMessage(MB_OK, TEXT("Error"), TEXT("%s %s"), sPortName,
		 gettext(TEXT("RX Thread not Terminated!")));
//#endif
  } else {
    CloseHandle(hReadThread);
  }
#endif

  return fRxThreadTerminated;
}

// Restart Rx Thread
// return: TRUE on success, FALSE on error
BOOL ComPort::StartRxThread(void)
{
  DWORD dwThreadID, dwError;

  if (hPort == INVALID_HANDLE_VALUE)
    return FALSE;

  CloseThread = FALSE;

  // Create a read thread for reading data from the communication port.
  if ((hReadThread = CreateThread
      (NULL, 0, ThreadProc, this, 0, &dwThreadID)) != NULL) {
    SetThreadPriority(hReadThread,
                      THREAD_PRIORITY_NORMAL); //THREAD_PRIORITY_ABOVE_NORMAL

    //???? JMW Why close it here?    CloseHandle(hReadThread);
  } else {
    // Could not create the read thread.
    ComPort_StatusMessage(MB_OK, TEXT("Error"), TEXT("%s %s"),
              gettext(TEXT("Unable to Start RX Thread on Port")), sPortName);
    dwError = GetLastError();
    return FALSE;
  }

  return TRUE;
}

                                        // Get a single Byte
                                        // return: char readed or EOF on error
int ComPort::GetChar(void)
{
  BYTE  inbuf[2];
  DWORD dwBytesTransferred;

  if (hPort == INVALID_HANDLE_VALUE || !CloseThread)
    return EOF;

  if (ReadFile(hPort, inbuf, 1, &dwBytesTransferred, (OVERLAPPED *)NULL)) {
    if (dwBytesTransferred == 1)
      return inbuf[0];
  }

  return EOF;
}

// Set Rx Timeout in ms
// Timeout: Rx receive timeout in ms
// return: last set Rx timeout or -1 on error
int ComPort::SetRxTimeout(int Timeout)
{
  COMMTIMEOUTS CommTimeouts;
  int result;
  DWORD dwError;

  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

  GetCommTimeouts(hPort, &CommTimeouts);

  result = CommTimeouts.ReadTotalTimeoutConstant;

  // Change the COMMTIMEOUTS structure settings.
  CommTimeouts.ReadIntervalTimeout = MAXDWORD;

  // JMW 20070515
  if (Timeout == 0) {
    // no total timeouts used
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.ReadTotalTimeoutConstant = 0;
  } else {
    // only total timeout used
    CommTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    CommTimeouts.ReadTotalTimeoutConstant = Timeout;
  }

  CommTimeouts.WriteTotalTimeoutMultiplier = 10;
  CommTimeouts.WriteTotalTimeoutConstant = 1000;

                                        // Set the time-out parameters
                                        // for all read and write
                                        // operations on the port.
  if (!SetCommTimeouts(hPort, &CommTimeouts)) {
     // Could not create the read thread.
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;
#ifdef WINDOWSPC
    Sleep(2000); // needed for windows bug
#endif
    ComPort_StatusMessage(MB_OK, TEXT("Error"), TEXT("%s %s"),
                 gettext(TEXT("Unable to Set Serial Port Timers")), sPortName);
    dwError = GetLastError();
    return -1;
  }

  return result;
}

unsigned long ComPort::SetBaudrate(unsigned long BaudRate)
{
  COMSTAT ComStat;
  DCB     PortDCB;
  DWORD   dwErrors;
  unsigned long result = 0;

  if (hPort == INVALID_HANDLE_VALUE)
    return result;

  do {
    ClearCommError(hPort, &dwErrors, &ComStat);
  } while (ComStat.cbOutQue > 0);

  Sleep(10);

  GetCommState(hPort, &PortDCB);

  result = PortDCB.BaudRate;
  PortDCB.BaudRate = BaudRate;

  if (!SetCommState(hPort, &PortDCB))
    return 0;

  return result;
}

int ComPort::Read(void *Buffer, size_t Size)
{
  DWORD dwBytesTransferred;

  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

  if (ReadFile(hPort, Buffer, Size, &dwBytesTransferred,
                (OVERLAPPED *)NULL)) {
    return dwBytesTransferred;
  }

  return -1;
}


void ComPort::ProcessChar(char c) {
  if (bi<NMEA_BUF_SIZE-1) {

    BuildingString[bi++] = c;

    if(c=='\n') {
      BuildingString[bi] = '\0';
      mutexBlackboard.Lock();
      devParseNMEA(dev, BuildingString, &device_blackboard.SetBasic());
      mutexBlackboard.Unlock();
    } else {
      return;
    }
  }
  // overflow, so reset buffer
  bi = 0;
}
