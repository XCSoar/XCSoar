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


#define COMMDEBUG 0

HANDLE hRead1Thread = NULL;              // Handle to the read thread
static BOOL  Port1CloseThread;
static BOOL  fRxThreadTerminated;
static TCHAR sPortName[8];

static DWORD dwMask;

BOOL Port1Initialize (LPTSTR lpszPortName, DWORD dwPortSpeed )
{
  DWORD dwError;
  DCB PortDCB;
  TCHAR sTmp[127];

  _tcscpy(sPortName, lpszPortName);

  // Open the serial port.
  hPort1 = CreateFile (lpszPortName, // Pointer to the name of the port
                      GENERIC_READ | GENERIC_WRITE,
                                    // Access (read-write) mode
                      0,            // Share mode
                      NULL,         // Pointer to the security attribute
                      OPEN_EXISTING,// How to open the serial port
                      FILE_ATTRIBUTE_NORMAL,            // Port attributes
                      NULL);        // Handle to port with attribute
                                    // to copy

  // If it fails to open the port, return FALSE.
  if ( hPort1 == INVALID_HANDLE_VALUE )
  {

    dwError = GetLastError ();

    // Could not open the port.
    // TODO SCOTT I18N - Fix this to sep the TEXT from PORT, TEXT can be
    // gettext(), port added on new line
    _stprintf(sTmp, TEXT("Unable to Open\r\nPort %s"), sPortName),
    MessageBox (hWndMainWindow, sTmp,
                TEXT("Error"), MB_OK|MB_ICONINFORMATION);
    return FALSE;
  }

  PortDCB.DCBlength = sizeof (DCB);

  // Get the default port setting information.
  GetCommState (hPort1, &PortDCB);

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
  if (!SetCommState (hPort1, &PortDCB))
  {
    // Could not create the read thread.
		CloseHandle (hPort1);
    // TODO SCOTT I18N - Fix this to sep the TEXT from PORT, TEXT can be
    // gettext(), port added on new line
    _stprintf(sTmp, TEXT("Unable to Change Settings on Port %s"), sPortName),
    MessageBox (hWndMainWindow, sTmp, TEXT("Error"), MB_OK);
    // dwError = GetLastError ();
    return FALSE;
  }

  Port1SetRxTimeout(0);

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  // SETRTS: Sends the RTS (request-to-send) signal.
  EscapeCommFunction (hPort1, SETDTR);
  EscapeCommFunction (hPort1, SETRTS);

  if (!Port1StartRxThread()){
    CloseHandle (hPort1);
    return(FALSE);
  }


  return TRUE;
}


/***********************************************************************

  PortWrite (BYTE Byte)

***********************************************************************/
void Port1Write (BYTE Byte)
{
  DWORD dwError,
        dwNumBytesWritten;

  if (!WriteFile (hPort1,              // Port handle
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
DWORD Port1ReadThread (LPVOID lpvoid)
{
  DWORD dwCommModemStatus, dwBytesTransferred;
  BYTE inbuf[1024];

  // JMW added purging of port on open to prevent overflow initially
  PurgeComm(hPort1,
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  // Specify a set of events to be monitored for the port.

  dwMask = EV_RXFLAG | EV_CTS | EV_DSR | EV_RING | EV_RXCHAR;

  SetCommMask(hPort1, dwMask);

  fRxThreadTerminated = FALSE;

  while ((hPort1 != INVALID_HANDLE_VALUE) && (!MapWindow::CLOSETHREAD) && (!Port1CloseThread))
  {
    int i=0;

    // Wait for an event to occur for the port.
    if (!WaitCommEvent (hPort1, &dwCommModemStatus, 0)) {
      // error reading from port
      Sleep(10);
    }

    // Re-specify the set of events to be monitored for the port.
    SetCommMask (hPort1, dwMask);

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
        if (ReadFile (hPort1, inbuf, 1024, &dwBytesTransferred, (OVERLAPPED *)NULL)) {

	        for (unsigned int j=0; j<dwBytesTransferred; j++) {
	          ProcessChar1 (inbuf[j]);
	        }

        } else {
	        dwBytesTransferred = 0;
        }

      } while (dwBytesTransferred != 0);

    }

    //    Sleep(5); // give port some time to fill

    // Retrieve modem control-register values.
    GetCommModemStatus (hPort1, &dwCommModemStatus);

  }
  fRxThreadTerminated = TRUE;

  return 0;
}


/***********************************************************************

  PortClose (HANDLE hCommPort)

***********************************************************************/
BOOL Port1Close (HANDLE hCommPort)
{
  DWORD dwError;

  if (hCommPort != INVALID_HANDLE_VALUE)
  {

    Port1StopRxThread();
    Sleep(20);  // todo ...

    // Close the communication port.
    if (!CloseHandle (hCommPort))
    {
      dwError = GetLastError ();
      return FALSE;
    }
    else
    {
      hCommPort = INVALID_HANDLE_VALUE;
      return TRUE;
    }
  }

  return FALSE;
}

void Port1WriteString(TCHAR *Text)
{
	int i,len;

	len = _tcslen(Text);

	for(i=0;i<len;i++)
		Port1Write ((BYTE)Text[i]);
}

                                        // Stop Rx Thread
                                        // return: TRUE on success, FALSE on error
BOOL Port1StopRxThread(void){

  if (hPort1 == INVALID_HANDLE_VALUE) return(FALSE);

  // JMW added purging of port on open to prevent overflow initially
  PurgeComm(hPort1,
            PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  Port1CloseThread = TRUE;

  //GetCommMask(hPort1, &dwMask);       // setting the comm event mask with the same value
  SetCommMask(hPort1, dwMask);          // will cancel any WaitCommEvent!
                                        // this is a documented CE trick to cancel the WaitCommEvent

  DWORD tm = GetTickCount()+2000l;

  while (!fRxThreadTerminated && (long)(tm-GetTickCount()) > 0){
    Sleep(10);
  }

  #if COMMDEBUG > 0
  if (!fRxThreadTerminated)
    MessageBox (hWndMainWindow, gettext(TEXT("Port1 RX Thread not Terminated!")), TEXT("Error"), MB_OK);
  #endif

  return(fRxThreadTerminated);

}


                                        // Restart Rx Thread
                                        // return: TRUE on success, FALSE on error
BOOL Port1StartRxThread(void){

  DWORD dwThreadID, dwError;
  TCHAR sTmp[127];

  Port1CloseThread = FALSE;
                                        // Create a read thread for reading data from the communication port.
  if (hRead1Thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )Port1ReadThread, 0, 0, &dwThreadID))
  {
    SetThreadPriority(hRead1Thread, THREAD_PRIORITY_NORMAL); //THREAD_PRIORITY_ABOVE_NORMAL
    CloseHandle (hRead1Thread);
  }
  else                                  // Could not create the read thread.
  {
    // TODO SCOTT I18N - Fix this to sep the TEXT from PORT, TEXT can be
    // gettext(), port added on new line
    _stprintf(sTmp, TEXT("Unable to Start RX Thread on Port %s"), sPortName),
    MessageBox (hWndMainWindow, sTmp, TEXT("Error"), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }
  return TRUE;
}

                                        // Get a single Byte
                                        // return: char readed or EOF on error
int Port1GetChar(void){

  char  inbuf[2];
  DWORD dwBytesTransferred;

  if (hPort1 == INVALID_HANDLE_VALUE || !Port1CloseThread) return(-1);

  if (ReadFile(hPort1, inbuf, 1, &dwBytesTransferred, (OVERLAPPED *)NULL)) {

    if (dwBytesTransferred == 1)
      return(inbuf[0]);

  }

  return(EOF);

}

                                        // Set Rx Timeout in ms
                                        // Timeout: Rx receive timeout in ms
                                        // return: last set Rx timeout or -1 on error
int Port1SetRxTimeout(int Timeout){

  COMMTIMEOUTS CommTimeouts;
  int result;
  DWORD dwError;

  if (hPort1 == INVALID_HANDLE_VALUE) return(-1);

  GetCommTimeouts(hPort1, &CommTimeouts);

  result = CommTimeouts.ReadTotalTimeoutConstant;

                                        // Change the COMMTIMEOUTS structure settings.
  CommTimeouts.ReadIntervalTimeout = MAXDWORD;
  CommTimeouts.ReadTotalTimeoutMultiplier = 0;
  CommTimeouts.ReadTotalTimeoutConstant = Timeout;
  CommTimeouts.WriteTotalTimeoutMultiplier = 10;
  CommTimeouts.WriteTotalTimeoutConstant = 1000;

                                        // Set the time-out parameters for all read and write operations
                                        // on the port.
  if (!SetCommTimeouts (hPort1, &CommTimeouts))
  {
                                        // Could not create the read thread.
		CloseHandle (hPort1);
    MessageBox (hWndMainWindow, gettext(TEXT("Unable to Set Serial Port Timers")),
                TEXT("Error"), MB_OK);
    dwError = GetLastError ();
    return(-1);
  }

  return(result);

}

unsigned long Port1SetBaudrate(unsigned long BaudRate){

  COMSTAT ComStat;
  DCB     PortDCB;
  DWORD   dwErrors;
  unsigned long result = 0;

  do{
    ClearCommError(hPort1, &dwErrors, &ComStat);
  } while(ComStat.cbOutQue > 0);

  Sleep(10);

  GetCommState(hPort1, &PortDCB);

  result = PortDCB.BaudRate;

  PortDCB.BaudRate = BaudRate;

  if (!SetCommState(hPort1, &PortDCB))
    return(0);

  return(result);

}

int Port1Read(void *Buffer, size_t Size){


  DWORD dwBytesTransferred;

  if (hPort1 == INVALID_HANDLE_VALUE) return(-1);

  if (ReadFile (hPort1, Buffer, Size, &dwBytesTransferred, (OVERLAPPED *)NULL)){

    return(dwBytesTransferred);

  }

  return(-1);

}

//////

void Port1WriteNMEA(TCHAR *Text)
{
  int i,len;
  len = _tcslen(Text);
  Port1Write((BYTE)_T('$'));
  unsigned char chk=0;
  for (i=0;i<len; i++) {
    chk ^= (BYTE)Text[i];
    Port1Write((BYTE)Text[i]);
  }
  Port1Write((BYTE)_T('*'));

  TCHAR tbuf[3];
  wsprintf(tbuf,TEXT("%02X"),chk);
  Port1Write((BYTE)tbuf[0]);
  Port1Write((BYTE)tbuf[1]);
  Port1Write((BYTE)_T('\r'));
  Port1Write((BYTE)_T('\n'));
}


void VarioWriteNMEA(TCHAR *Text) {
  if (!(GPS_INFO.VarioAvailable))
    return;

  if (Port2Available) {
    // assume vario is on port B
    Port2WriteNMEA(Text);
  } else {
    if (Port1Available) {
      Port1WriteNMEA(Text);
    }
  }
}
