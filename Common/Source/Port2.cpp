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

#include "stdafx.h"
#include "Port.h"
#include "externs.h"
#include "XCSoar.h"

#include <windows.h>
#include <tchar.h>


HANDLE hRead2Thread = NULL;              // Handle to the read thread
static TCHAR sPortName[8];
static BOOL  fRxThreadTerminated=TRUE;
static BOOL  Port2CloseThread;

BOOL Port2Initialize (LPTSTR lpszPortName, DWORD dwPortSpeed )
{
  DWORD dwError,
        dwThreadID;
  DCB PortDCB;
  COMMTIMEOUTS CommTimeouts;

  _tcscpy(sPortName, lpszPortName);

  // Open the serial port.
  hPort2 = CreateFile (lpszPortName, // Pointer to the name of the port
                      GENERIC_READ | GENERIC_WRITE,
                                    // Access (read-write) mode
                      0,            // Share mode
                      NULL,         // Pointer to the security attribute
                      OPEN_EXISTING,// How to open the serial port
                      0,            // Port attributes
                      NULL);        // Handle to port with attribute
                                    // to copy

  // If it fails to open the port, return FALSE.
  if ( hPort2 == INVALID_HANDLE_VALUE ) 
  {
    TCHAR sTmp[127];
    
    dwError = GetLastError ();

    // Could not open the port.
    _stprintf(sTmp, TEXT("Unable to Open\r\nPort %s"), sPortName),
    MessageBoxX (hWndMainWindow, sTmp,
		 gettext(TEXT("Error")), MB_OK|MB_ICONINFORMATION);
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
    MessageBoxX (hWndMainWindow, gettext(TEXT("Unable to Change the Vario Serial Port Settings")), 
		 gettext(TEXT("Error")), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }

  // Retrieve the time-out parameters for all read and write operations
  // on the port. 
  GetCommTimeouts (hPort2, &CommTimeouts);

  // Change the COMMTIMEOUTS structure settings.
  CommTimeouts.ReadIntervalTimeout = MAXDWORD;  
  CommTimeouts.ReadTotalTimeoutMultiplier = 0;  
  CommTimeouts.ReadTotalTimeoutConstant = 0;    
  CommTimeouts.WriteTotalTimeoutMultiplier = 10;  
  CommTimeouts.WriteTotalTimeoutConstant = 1000;    

  // Set the time-out parameters for all read and write operations
  // on the port. 
  if (!SetCommTimeouts (hPort2, &CommTimeouts))
  {
    // Could not create the read thread.
		CloseHandle (hPort2);
    MessageBoxX (hWndMainWindow, gettext(TEXT("Unable to Set Serial Port Timers")),
                gettext(TEXT("Error")), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  // SETRTS: Sends the RTS (request-to-send) signal. 
  EscapeCommFunction (hPort2, SETDTR);
  EscapeCommFunction (hPort2, SETRTS);

  Port2CloseThread = FALSE;

  // Create a read thread for reading data from the communication port.
  if (hRead2Thread = CreateThread (NULL, 0, 
				   (LPTHREAD_START_ROUTINE )Port2ReadThread, 0, 0, 
				   &dwThreadID))
  {
    SetThreadPriority(hRead2Thread,
		      THREAD_PRIORITY_NORMAL);

    CloseHandle (hRead2Thread);
  }
  else
  {
    // Could not create the read thread.
    CloseHandle (hPort2);
    MessageBoxX (hWndMainWindow, 
		 gettext(TEXT("Unable to Start Vario Serial Port Handler")),
		 gettext(TEXT("Error")), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }

  return TRUE;
}


/***********************************************************************

  PortWrite (BYTE Byte)

***********************************************************************/
void Port2Write (BYTE Byte)
{

  if (hPort2 == INVALID_HANDLE_VALUE) return;

  DWORD dwError,
        dwNumBytesWritten;

  if (!WriteFile (hPort2,              // Port handle
                  &Byte,              // Pointer to the data to write 
                  1,                  // Number of bytes to write
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

DWORD Port2ReadThread (LPVOID lpvoid)
{
  DWORD dwCommModemStatus, dwBytesTransferred;
  BYTE inbuf[1024];
  
  // JMW added purging of port on open to prevent overflow
  PurgeComm(hPort2, 
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  // Specify a set of events to be monitored for the port.

  dwMask2 = EV_RXFLAG | EV_CTS | EV_DSR | EV_RING | EV_RXCHAR;
  SetCommMask(hPort2, dwMask2);

  fRxThreadTerminated = FALSE;

  while ((hPort2 != INVALID_HANDLE_VALUE)
	 &&(!MapWindow::CLOSETHREAD)
	 && (!Port2CloseThread)) 
  {
    int i=0;

    // Wait for an event to occur for the port.
    if (!WaitCommEvent (hPort2, &dwCommModemStatus, 0)) {
      // error reading from port
      Sleep(100);
    }

    // Re-specify the set of events to be monitored for the port.
    //    SetCommMask (hPort2, dwMask2);

    if (
	(dwCommModemStatus & EV_RXFLAG)
	||(dwCommModemStatus & EV_RXCHAR)
	)
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
	if (Port2CloseThread)
	  dwBytesTransferred = 0;
      } while (dwBytesTransferred != 0);

    }

    Sleep(5); // give port some time to fill
    // Retrieve modem control-register values.
    GetCommModemStatus (hPort2, &dwCommModemStatus);

  }
  fRxThreadTerminated = TRUE;
  PurgeComm(hPort2, 
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
  CloseHandle (hPort2);
  hPort2 = INVALID_HANDLE_VALUE;

  return 0;
}


// Stop Rx Thread
// return: TRUE on success, FALSE on error
BOOL Port2StopRxThread(void){
  
  if (hPort2 == INVALID_HANDLE_VALUE) return(FALSE);

  Port2CloseThread = TRUE;

  DWORD tm = GetTickCount()+20000l;
#if (WINDOWSPC>0)
  /*
  dwMask2 = 0;
  while (!fRxThreadTerminated && (long)(tm-GetTickCount()) > 0){
    Sleep(10);
  }
  */
  LockFlightData();
  TerminateThread(hRead2Thread, 0);
  CloseHandle(hRead2Thread);
  UnlockFlightData();
  
  if (hPort2 != INVALID_HANDLE_VALUE)
    CloseHandle (hPort2);
  
#else
  PurgeComm(hPort2, 
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
  // setting the comm event mask with the same value
  //  GetCommMask(hPort2, &dwMask2);
  SetCommMask(hPort2, dwMask2);          // will cancel any
                                         // WaitCommEvent!  this is a
                                        // documented CE trick to
                                        // cancel the WaitCommEvent
  while (!fRxThreadTerminated && (long)(tm-GetTickCount()) > 0){
    Sleep(10);
  }
  //  #if COMMDEBUG > 0
  if (!fRxThreadTerminated)
    MessageBoxX (hWndMainWindow, 
		 gettext(TEXT("Port2 RX Thread not Terminated!")), 
		 TEXT("Error"), MB_OK);
  //  #endif
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

#if (WINDOWSPC>0)
    dwError = 0;
    hPort2 = INVALID_HANDLE_VALUE;
    return TRUE;
#else
    // Close the communication port.
    if (!CloseHandle (hPort2))
    {
      dwError = GetLastError ();
      return FALSE;
    }
    else
    {
      hPort2 = INVALID_HANDLE_VALUE;
      return TRUE;
    }
#endif
  }
  return FALSE;
}


void Port2WriteString(TCHAR *Text)
{
  LockComm();

	int i,len;
	len = _tcslen(Text);

	for(i=0;i<len;i++)
		Port2Write ((BYTE)Text[i]);
  UnlockComm();
}


void Port2WriteNMEA(TCHAR *Text) 
{
  LockComm();

  int i,len;
  len = _tcslen(Text);
  Port2Write((BYTE)_T('$'));
  unsigned char chk=0;
  for (i=0;i<len; i++) {
    chk ^= (BYTE)Text[i];
    Port2Write((BYTE)Text[i]);
  }
  Port2Write((BYTE)_T('*'));

  TCHAR tbuf[3];
  wsprintf(tbuf,TEXT("%02X"),chk);
  Port2Write((BYTE)tbuf[0]);
  Port2Write((BYTE)tbuf[1]);
  Port2Write((BYTE)_T('\r'));
  Port2Write((BYTE)_T('\n'));
  UnlockComm();
}
