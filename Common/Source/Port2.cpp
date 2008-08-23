/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "StdAfx.h"
#include "Port.h"
#include "externs.h"
#include "XCSoar.h"

#include <windows.h>
#include <tchar.h>


HANDLE hRead2Thread = NULL;              // Handle to the read thread
static TCHAR sPortName[8];
static BOOL  fRxThreadTerminated=TRUE;
static BOOL  Port2CloseThread;

BOOL Port2Initialize (LPTSTR lpszPortName, DWORD dwPortSpeed)
{
  DWORD dwError;
  DCB PortDCB;
  TCHAR sTmp[127];

  if (lpszPortName) {
    _tcscpy(sPortName, lpszPortName);
  }

  // Open the serial port.
  hPort2 = CreateFile (sPortName, // Pointer to the name of the port
                      GENERIC_READ | GENERIC_WRITE,
                                    // Access (read-write) mode
                      0,            // Share mode
                      NULL,         // Pointer to the security attribute
                      OPEN_EXISTING,// How to open the serial port
                      FILE_ATTRIBUTE_NORMAL,            // Port attributes
                      NULL);        // Handle to port with attribute
                                    // to copy

  // If it fails to open the port, return FALSE.
  if ( hPort2 == INVALID_HANDLE_VALUE )
  {

    dwError = GetLastError ();

    // Could not open the port.
    _stprintf(sTmp, TEXT("%s %s"), gettext(TEXT("Unable to open port")),
              sPortName),
      //    MessageBoxX (hWndMainWindow, sTmp,
      //		 gettext(TEXT("Error")), MB_OK|MB_ICONINFORMATION);
    DoStatusMessage(sTmp);
    return FALSE;
  }

  PortDCB.DCBlength = sizeof (DCB);

  // Get the default port setting information.
  GetCommState (hPort2, &PortDCB);

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
  PortDCB.fNull = FALSE;                // Disable null stripping
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
  if (!SetCommState (hPort2, &PortDCB))
  {
    // Could not create the read thread.
    CloseHandle (hPort2);
#if (WINDOWSPC>0)
    Sleep(2000); // needed for windows bug
#endif

    _stprintf(sTmp, TEXT("%s %s"),
              gettext(TEXT("Unable to Change Settings on Port")), sPortName);
    MessageBoxX (hWndMainWindow, sTmp,
                 gettext(TEXT("Error")), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }

  //  Port2SetRxTimeout(10); // JMW20070515 wait a maximum of 10ms
  Port2SetRxTimeout(0);

  SetupComm(hPort2, 1024, 1024);

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  // SETRTS: Sends the RTS (request-to-send) signal.
  EscapeCommFunction (hPort2, SETDTR);
  EscapeCommFunction (hPort2, SETRTS);

  if (!Port2StartRxThread()){
    CloseHandle (hPort2);
#if (WINDOWSPC>0)
    Sleep(2000); // needed for windows bug
#endif
    return(FALSE);
  }
  return TRUE;
}


/***********************************************************************

  PortWrite (BYTE Byte)
  20060514:sgi change to block write, writting byte by byte is very slow

***********************************************************************/
void Port2Write(void *Buffer, size_t Size){

  if (hPort2 == INVALID_HANDLE_VALUE) return;

  DWORD dwError,
        dwNumBytesWritten;

  if (!WriteFile (hPort2,             // Port handle
                  Buffer,             // Pointer to the data to write
                  Size,               // Number of bytes to write
                  &dwNumBytesWritten, // Pointer to the number of bytes
                                      // written
                  (OVERLAPPED *)NULL))              // Must be NULL for Windows CE
  {
    // WriteFile failed. Report error.
    dwError = GetLastError ();
  }
}

/***********************************************************************

  PortReadThread (LPVOID lpvoid)

***********************************************************************/
static DWORD dwMask2;

/***********************************************************************

  PortWrite (BYTE Byte)

***********************************************************************/
void Port2Write (BYTE Byte)
{

  if (hPort2 == INVALID_HANDLE_VALUE) return;

  DWORD dwError,
        dwNumBytesWritten;

  if (!WriteFile (hPort2,              // Port handle
                  &Byte,               // Pointer to the data to write
                  1,                   // Number of bytes to write
                  &dwNumBytesWritten,  // Pointer to the number of bytes
                                       // written
                  (OVERLAPPED *)NULL)) // Must be NULL for Windows CE
  {
    // WriteFile failed. Report error.
    dwError = GetLastError ();
  }
}

void Port2Flush(void) {
  PurgeComm(hPort2,
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}


DWORD Port2ReadThread (LPVOID lpvoid)
{
  DWORD dwCommModemStatus, dwBytesTransferred;
  BYTE inbuf[1024];
  (void)lpvoid;

  // JMW added purging of port on open to prevent overflow
  Port2Flush();

  // Specify a set of events to be monitored for the port.

  dwMask2 = EV_RXFLAG | EV_CTS | EV_DSR | EV_RING | EV_RXCHAR;
  SetCommMask(hPort2, dwMask2);

  fRxThreadTerminated = FALSE;

  while ((hPort2 != INVALID_HANDLE_VALUE)
	 &&(!MapWindow::CLOSETHREAD)
	 && (!Port2CloseThread))
  {
//    int i=0;
    #if (WINDOWSPC>0)
    Sleep(50);  // ToDo rewrite the whole driver to use overlaped IO on W2K or higher
    #else
    // Wait for an event to occur for the port.
    if (!WaitCommEvent (hPort2, &dwCommModemStatus, 0)) {
      // error reading from port
      Sleep(100);
    }
    #endif

    // Re-specify the set of events to be monitored for the port.
    //    SetCommMask (hPort2, dwMask2);

    #if !defined(WINDOWSPC) || (WINDOWSPC==0)
    if (
      (dwCommModemStatus & EV_RXFLAG)
      ||(dwCommModemStatus & EV_RXCHAR)
      )
    #endif
    {

      // Loop for waiting for the data.
      do
      {
	dwBytesTransferred = 0;
        // Read the data from the serial port.
	if (ReadFile (hPort2, inbuf, 1024, &dwBytesTransferred,
		      (OVERLAPPED *)NULL)) {

	  for (unsigned int j=0; j<dwBytesTransferred; j++) {
	    ProcessChar2 (inbuf[j]);
	  }
	} else {
	  dwBytesTransferred = 0;
	}
        Sleep(50); // JMW20070515: give port some time to fill... prevents ReadFile
                  // from causing the thread to take up too much CPU
	if (Port2CloseThread)
	  dwBytesTransferred = 0;
      } while (dwBytesTransferred != 0);

    }

    Sleep(5); // give port some time to fill
    // Retrieve modem control-register values.
    GetCommModemStatus (hPort2, &dwCommModemStatus);

  }
  Port2Flush();

  fRxThreadTerminated = TRUE;

  return 0;
}


// Restart Rx Thread
// return: TRUE on success, FALSE on error
BOOL Port2StartRxThread(void){

  DWORD dwThreadID, dwError;
  TCHAR sTmp[127];

  Port2CloseThread = FALSE;
  // Create a read thread for reading data from the communication port.
  if ((hRead2Thread = CreateThread
      (NULL, 0, (LPTHREAD_START_ROUTINE )Port2ReadThread,
       0, 0, &dwThreadID)) != NULL)
  {
    SetThreadPriority(hRead2Thread,
                      THREAD_PRIORITY_NORMAL); //THREAD_PRIORITY_ABOVE_NORMAL
    /////???? Why close this handle here?    CloseHandle (hRead2Thread);
  }
  else                                  // Could not create the read thread.
  {
    _stprintf(sTmp, TEXT("%s %s"),
              gettext(TEXT("Unable to Start RX Thread on Port")), sPortName);
    MessageBoxX (hWndMainWindow, sTmp,
                 gettext(TEXT("Error")), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }
  return TRUE;
}


// Stop Rx Thread
// return: TRUE on success, FALSE on error
BOOL Port2StopRxThread(void){

  if (hPort2 == INVALID_HANDLE_VALUE) return(FALSE);
  if (fRxThreadTerminated) return (TRUE);

  Port2CloseThread = TRUE;

  DWORD tm = GetTickCount()+20000l;
#if (WINDOWSPC>0)
  //  dwMask2 = 0;
  while (!fRxThreadTerminated && (long)(tm-GetTickCount()) > 0){
    Sleep(10);
  }
  if (!fRxThreadTerminated) {
    TerminateThread(hRead2Thread, 0);
  } else {
    CloseHandle(hRead2Thread);
  }
#else
  Port2Flush();
  // setting the comm event mask with the same value
  //  GetCommMask(hPort2, &dwMask2);
  SetCommMask(hPort2, dwMask2);          // will cancel any
                                         // WaitCommEvent!  this is a
                                        // documented CE trick to
                                        // cancel the WaitCommEvent
  while (!fRxThreadTerminated && (long)(GetTickCount()-tm) < 1000){
    Sleep(10);
  }
  if (!fRxThreadTerminated) {
  //  #if COMMDEBUG > 0
    MessageBoxX (hWndMainWindow,
		 gettext(TEXT("Port2 RX Thread not Terminated!")),
		 gettext(TEXT("Error")), MB_OK);
  //  #endif
  } else {
    CloseHandle(hRead2Thread);
  }

#endif

  return(fRxThreadTerminated);

}


/***********************************************************************

  PortClose ()

***********************************************************************/
BOOL Port2Close ()
{
  DWORD dwError;

  if (hPort2 != INVALID_HANDLE_VALUE)
  {

    Port2StopRxThread();
    Sleep(100);  // todo ...

    dwError = 0;

    // Close the communication port.
    if (!CloseHandle (hPort2)) {
      dwError = GetLastError ();
      return FALSE;
    } else {
#if (WINDOWSPC>0)
      Sleep(2000); // needed for windows bug
#endif
      hPort2 = INVALID_HANDLE_VALUE;
      return TRUE;
    }
  }
  return FALSE;
}


void Port2WriteString(const TCHAR *Text){
  char sTmp[512];
  LockComm();

  int len = _tcslen(Text);

  // 20060514: sgi change to block write
  WideCharToMultiByte( CP_ACP, 0, Text,
			 len+1,
			 sTmp,
			 512, NULL, NULL);

  sTmp[512-1] = '\0';
  Port2Write(sTmp, len);
  UnlockComm();
}


// Set Rx Timeout in ms
// Timeout: Rx receive timeout in ms
// return: last set Rx timeout or -1 on error
int Port2SetRxTimeout(int Timeout){

  COMMTIMEOUTS CommTimeouts;
  int result;
  DWORD dwError;

  if (hPort2 == INVALID_HANDLE_VALUE) return(-1);

  GetCommTimeouts(hPort2, &CommTimeouts);

  result = CommTimeouts.ReadTotalTimeoutConstant;

  // Change the COMMTIMEOUTS structure settings.
  CommTimeouts.ReadIntervalTimeout = MAXDWORD;

  // JMW 20070515
  if (Timeout==0) {
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
  if (!SetCommTimeouts (hPort2, &CommTimeouts))
  {
                                        // Could not create the read thread.
    CloseHandle (hPort2);
#if (WINDOWSPC>0)
    Sleep(2000); // needed for windows bug
#endif
    MessageBoxX (hWndMainWindow,
                 gettext(TEXT("Unable to Set Serial Port Timers")),
		 gettext(TEXT("Error")), MB_OK);
    dwError = GetLastError ();
    return(-1);
  }

  return(result);

}


//////////

                                        // Get a single Byte
                                        // return: char readed or EOF on error
int Port2GetChar(void){

  BYTE  inbuf[2];
  DWORD dwBytesTransferred;

  if (hPort2 == INVALID_HANDLE_VALUE || !Port2CloseThread) return(EOF);

  if (ReadFile(hPort2, inbuf, 1, &dwBytesTransferred, (OVERLAPPED *)NULL)) {

    if (dwBytesTransferred == 1)
      return(inbuf[0]);

  }

  return(EOF);

}



unsigned long Port2SetBaudrate(unsigned long BaudRate){

  COMSTAT ComStat;
  DCB     PortDCB;
  DWORD   dwErrors;
  unsigned long result = 0;

  do{
    ClearCommError(hPort2, &dwErrors, &ComStat);
  } while(ComStat.cbOutQue > 0);

  Sleep(10);

  GetCommState(hPort2, &PortDCB);

  result = PortDCB.BaudRate;

  PortDCB.BaudRate = BaudRate;

  if (!SetCommState(hPort2, &PortDCB))
    return(0);

  return(result);

}



int Port2Read(void *Buffer, size_t Size) {

  DWORD dwBytesTransferred;

  if (hPort2 == INVALID_HANDLE_VALUE) return(-1);

  if (ReadFile (hPort2, Buffer, Size, &dwBytesTransferred,
                (OVERLAPPED *)NULL)){

    return(dwBytesTransferred);

  }

  return(-1);

}
