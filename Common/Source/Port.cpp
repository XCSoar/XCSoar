
#include "stdafx.h"
#include "Port.h"
#include "externs.h"
#include "XCSoar.h"

#include <windows.h>
#include <tchar.h>


extern BOOL CLOSETHREAD;
HANDLE				hRead1Thread = NULL;              // Handle to the read thread


BOOL Port1Initialize (LPTSTR lpszPortName, DWORD dwPortSpeed )
{
  DWORD dwError,
        dwThreadID;
  DCB PortDCB;
  COMMTIMEOUTS CommTimeouts;

  // Open the serial port.
  hPort1 = CreateFile (lpszPortName, // Pointer to the name of the port
                      GENERIC_READ | GENERIC_WRITE,
                                    // Access (read-write) mode
                      0,            // Share mode
                      NULL,         // Pointer to the security attribute
                      OPEN_EXISTING,// How to open the serial port
                      0,            // Port attributes
                      NULL);        // Handle to port with attribute
                                    // to copy

  // If it fails to open the port, return FALSE.
  if ( hPort1 == INVALID_HANDLE_VALUE )
  {
    // Could not open the port.
    MessageBox (hWndMainWindow, TEXT("GPS Serial Port Unavailable"),
                TEXT("Error"), MB_OK|MB_ICONINFORMATION);
    dwError = GetLastError ();
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
    MessageBox (hWndMainWindow, TEXT("Unable to Change the GPS Serial Port Settings"),
                TEXT("Error"), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }

  // Retrieve the time-out parameters for all read and write operations
  // on the port.
  GetCommTimeouts (hPort1, &CommTimeouts);

  // Change the COMMTIMEOUTS structure settings.
  CommTimeouts.ReadIntervalTimeout = MAXDWORD;
  CommTimeouts.ReadTotalTimeoutMultiplier = 0;
  CommTimeouts.ReadTotalTimeoutConstant = 0;
  CommTimeouts.WriteTotalTimeoutMultiplier = 10;
  CommTimeouts.WriteTotalTimeoutConstant = 1000;

  // Set the time-out parameters for all read and write operations
  // on the port.
  if (!SetCommTimeouts (hPort1, &CommTimeouts))
  {
    // Could not create the read thread.
		CloseHandle (hPort1);
    MessageBox (hWndMainWindow, TEXT("Unable to Set Serial Port Timers"),
                TEXT("Error"), MB_OK);
    dwError = GetLastError ();
    return FALSE;
  }

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  // SETRTS: Sends the RTS (request-to-send) signal.
  EscapeCommFunction (hPort1, SETDTR);
  EscapeCommFunction (hPort1, SETRTS);

  // Create a read thread for reading data from the communication port.
  if (hRead1Thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )Port1ReadThread, 0, 0,
                                  &dwThreadID))
  {
    SetThreadPriority(hRead1Thread,
		      THREAD_PRIORITY_NORMAL);
    //THREAD_PRIORITY_ABOVE_NORMAL
    CloseHandle (hRead1Thread);
  }
  else
  {
    // Could not create the read thread.
		CloseHandle (hPort1);
    MessageBox (hWndMainWindow, TEXT("Unable to Start GPS Serial Port Handler"),
                TEXT("Error"), MB_OK);
    dwError = GetLastError ();
    return FALSE;
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
                  NULL))              // Must be NULL for Windows CE
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

  // Specify a set of events to be monitored for the port.
  SetCommMask (hPort1,  EV_RXFLAG |  EV_CTS
	       | EV_DSR | EV_RLSD | EV_RING
	       // | EV_RXCHAR
	       );

  while ((hPort1 != INVALID_HANDLE_VALUE)&&(!CLOSETHREAD))
  {
    int i=0;

    // Wait for an event to occur for the port.
    if (!WaitCommEvent (hPort1, &dwCommModemStatus, 0)) {
      // error reading from port
      Sleep(10);
    }

    // Re-specify the set of events to be monitored for the port.
    SetCommMask (hPort1, EV_RXFLAG | EV_CTS | EV_DSR | EV_RING
    // | EV_RXCHAR
		 );

    if (
	(dwCommModemStatus & EV_RXFLAG)
	//	||(dwCommModemStatus & EV_RXCHAR)
	)
    {

      // Loop for waiting for the data.
      do
      {
	dwBytesTransferred = 0;
        // Read the data from the serial port.
	if (ReadFile (hPort1, inbuf, 1024, &dwBytesTransferred, 0)) {

	  for (int j=0; j<dwBytesTransferred; j++) {
	    ProcessChar1 (inbuf[j]);
	  }
	} else {
	  dwBytesTransferred = 0;
	}

      } while (dwBytesTransferred != 0);

    }

    // Retrieve modem control-register values.
    GetCommModemStatus (hPort1, &dwCommModemStatus);

  }

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

