/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Device/SerialPort.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Asset.hpp"
#include "OS/Sleep.h"

#ifdef _WIN32_WCE
#include "Device/Widcomm.hpp"
#else
#include "OS/OverlappedEvent.hpp"
#endif

#include <windows.h>

#include <assert.h>
#include <tchar.h>
#include <stdio.h>

static void
SerialPort_StatusMessage(unsigned type, const TCHAR *caption,
                      const TCHAR *fmt, ...)
{
  TCHAR tmp[127];
  va_list ap;

  va_start(ap, fmt);
  _vsntprintf(tmp, 127, fmt, ap);
  va_end(ap);

  if (caption)
    MessageBoxX(tmp, caption, type);
  else
    Message::AddMessage(tmp);
}

SerialPort::SerialPort(const TCHAR *path, unsigned _baud_rate, Handler &_handler)
  :Port(_handler), baud_rate(_baud_rate),
   hPort(INVALID_HANDLE_VALUE),
   buffer(NMEA_BUF_SIZE)
#ifndef _WIN32_WCE
   , rx_timeout(0)
#endif
{
  assert(path != NULL);

  _tcscpy(sPortName, path);
}

SerialPort::~SerialPort()
{
  Close();
}

bool
SerialPort::Open()
{
#ifdef _WIN32_WCE
  is_widcomm = IsWidcommDevice(sPortName);
#endif

  DCB PortDCB;

  buffer.clear();

  // Open the serial port.
  hPort = CreateFile(sPortName,    // Pointer to the name of the port
                     GENERIC_READ | GENERIC_WRITE, // Access (read-write) mode
                     0,            // Share mode
                     NULL,         // Pointer to the security attribute
                     OPEN_EXISTING,// How to open the serial port
#ifdef _WIN32_WCE
                     FILE_ATTRIBUTE_NORMAL, // Port attributes
#else
                     FILE_FLAG_OVERLAPPED, // Overlapped I/O
#endif
                     NULL);        // Handle to port with attribute to copy

  // If it fails to open the port, return false.
  if (hPort == INVALID_HANDLE_VALUE) {
    // Could not open the port.
    SerialPort_StatusMessage(MB_OK | MB_ICONINFORMATION, NULL,
                          _("Unable to open port %s"), sPortName);

    return false;
  }

  PortDCB.DCBlength = sizeof(DCB);

  // Get the default port setting information.
  GetCommState(hPort, &PortDCB);

  // Change the DCB structure settings.
  PortDCB.BaudRate = baud_rate; // Current baud
  PortDCB.fBinary = true;               // Binary mode; no EOF check
  PortDCB.fParity = true;               // Enable parity checking
  PortDCB.fOutxCtsFlow = false;         // No CTS output flow control
  PortDCB.fOutxDsrFlow = false;         // No DSR output flow control
  PortDCB.fDtrControl = DTR_CONTROL_ENABLE; // DTR flow control type
  PortDCB.fDsrSensitivity = false;      // DSR sensitivity
  PortDCB.fTXContinueOnXoff = true;     // XOFF continues Tx
  PortDCB.fOutX = false;                // No XON/XOFF out flow control
  PortDCB.fInX = false;                 // No XON/XOFF in flow control
  PortDCB.fErrorChar = false;           // Disable error replacement
  PortDCB.fNull = false;                // Disable null removal
  PortDCB.fRtsControl = RTS_CONTROL_ENABLE; // RTS flow control
  PortDCB.fAbortOnError = true;         // JMW abort reads/writes on error
  PortDCB.ByteSize = 8;                 // Number of bits/byte, 4-8
  PortDCB.Parity = NOPARITY;            // 0-4=no,odd,even,mark,space
  PortDCB.StopBits = ONESTOPBIT;        // 0,1,2 = 1, 1.5, 2
  PortDCB.EvtChar = '\n';               // wait for end of line

  // Configure the port according to the specifications of the DCB structure.
  if (!SetCommState(hPort, &PortDCB)) {
    // Could not create the read thread.
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;

    // TODO code: SCOTT I18N - Fix this to sep the TEXT from PORT, TEXT can be
    // gettext(), port added on new line
    SerialPort_StatusMessage(MB_OK, _("Error"),
                          _("Unable to change settings on port %s"), sPortName);
    return false;
  }

  //  SetRxTimeout(10); // JMW20070515 wait a maximum of 10ms
  SetRxTimeout(0);

  SetupComm(hPort, 1024, 1024);

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  EscapeCommFunction(hPort, SETDTR);
  // SETRTS: Sends the RTS (request-to-send) signal.
  EscapeCommFunction(hPort, SETRTS);

  if (!StartRxThread()){
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;

    return false;
  }

  return true;
}

void
SerialPort::Flush(void)
{
  PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

int
SerialPort::GetDataPending() const
{
  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

  COMSTAT com_stat;
  DWORD errors;
  return ::ClearCommError(hPort, &errors, &com_stat)
    ? (int)com_stat.cbInQue
    : -1;
}

#ifndef _WIN32_WCE

int
SerialPort::WaitDataPending(OverlappedEvent &overlapped,
                            unsigned timeout_ms) const
{
  int nbytes = GetDataPending();
  if (nbytes != 0)
    return nbytes;

  ::SetCommMask(hPort, EV_RXCHAR);

  DWORD dwCommModemStatus;
  if (!::WaitCommEvent(hPort, &dwCommModemStatus, overlapped.GetPointer())) {
    if (::GetLastError() != ERROR_IO_PENDING)
      return -1;

    if (overlapped.Wait(timeout_ms) != OverlappedEvent::FINISHED) {
      /* the operation may still be running, we have to cancel it */
      ::CancelIo(hPort);
      return -1;
    }

    DWORD result;
    if (!::GetOverlappedResult(hPort, overlapped.GetPointer(), &result, FALSE))
      return -1;
  }

  if ((dwCommModemStatus & EV_RXCHAR) == 0)
    return -1;

  return GetDataPending();
}

#endif

void
SerialPort::Run()
{
  DWORD dwBytesTransferred;
  BYTE inbuf[1024];

  // JMW added purging of port on open to prevent overflow
  Flush();

#ifndef _WIN32_WCE
  OverlappedEvent osStatus, osReader;
  if (!osStatus.Defined() || !osReader.Defined())
     // error creating event; abort
     return;
#endif

  // Specify a set of events to be monitored for the port.
  if (is_widcomm)
    SetRxTimeout(180);
  else
    ::SetCommMask(hPort, EV_RXCHAR);

  while (!CheckStopped()) {

#ifndef _WIN32_WCE

    int nbytes = WaitDataPending(osStatus, 500);
    if (nbytes <= 0) {
      ::Sleep(100);
      continue;
    }

    // Start reading data

    if ((size_t)nbytes > sizeof(inbuf))
      nbytes = sizeof(inbuf);

    if (!::ReadFile(hPort, inbuf, nbytes, &dwBytesTransferred,
                    osReader.GetPointer())) {
      if (::GetLastError() != ERROR_IO_PENDING) {
        // Error in ReadFile() occured
        ::Sleep(100);
        continue;
      }

      if (osReader.Wait() != OverlappedEvent::FINISHED) {
        ::CancelIo(hPort);
        continue;
      }

      if (!::GetOverlappedResult(hPort, osReader.GetPointer(),
                                 &dwBytesTransferred, FALSE))
        continue;
    }

    // Process data that was directly read by ReadFile()
    for (unsigned int j = 0; j < dwBytesTransferred; j++)
      ProcessChar(inbuf[j]);

#else

    if (is_widcomm) {
      /* WaitCommEvent() doesn't work with the Widcomm Bluetooth
         driver, it blocks for 11 seconds, regardless whether data is
         received.  This workaround polls for input manually.
         Observed on an iPaq hx4700 with WM6. */
    } else {
      // Wait for an event to occur for the port.
      DWORD dwCommModemStatus;
      if (!::WaitCommEvent(hPort, &dwCommModemStatus, 0)) {
        // error reading from port
        Sleep(100);
        continue;
      }

      if ((dwCommModemStatus & EV_RXCHAR) == 0)
        /* no data available */
        continue;
    }

    // Read the data from the serial port.
    if (!ReadFile(hPort, inbuf, 1024, &dwBytesTransferred, NULL) ||
        dwBytesTransferred == 0) {
      Sleep(100);
      continue;
    }

    for (unsigned int j = 0; j < dwBytesTransferred; j++)
      ProcessChar(inbuf[j]);
#endif

  }

  Flush();
}

bool
SerialPort::Close()
{
  if (hPort != INVALID_HANDLE_VALUE) {
    StopRxThread();
    Sleep(100); // todo ...

    // Close the communication port.
    if (!CloseHandle(hPort)) {
      return false;
    } else {
      if (!is_embedded())
        Sleep(2000); // needed for windows bug

      hPort = INVALID_HANDLE_VALUE;
      return true;
    }
  }

  return false;
}

size_t
SerialPort::Write(const void *data, size_t length)
{
  DWORD NumberOfBytesWritten;

  if (hPort == INVALID_HANDLE_VALUE)
    return 0;

#ifdef _WIN32_WCE

  if (is_embedded())
    /* this is needed to work around a driver bug on the HP31x -
       without it, the second consecutive write without a task switch
       will hang the whole PNA; this Sleep() call enforces a task
       switch */
    Sleep(100);

  // lpNumberOfBytesWritten : This parameter can be NULL only when the lpOverlapped parameter is not NULL.
  if (!::WriteFile(hPort, data, length, &NumberOfBytesWritten, NULL))
    return 0;

  return NumberOfBytesWritten;

#else

  OverlappedEvent osWriter;

  // Start reading data
  if (::WriteFile(hPort, data, length, &NumberOfBytesWritten, osWriter.GetPointer()))
    return 0;

  if (::GetLastError() != ERROR_IO_PENDING)
    return 0;

  // Let's wait for ReadFile() to finish
  // 1000ms like WriteTotalTimeoutConstant in SetRxTimeout()
  switch (osWriter.Wait(1000)) {
  case OverlappedEvent::FINISHED:
    // Get results
    ::GetOverlappedResult(hPort, osWriter.GetPointer(), &NumberOfBytesWritten, FALSE);
    return NumberOfBytesWritten;

  default:
    ::CancelIo(hPort);
    return 0;
  }
#endif
}

bool
SerialPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // Make sure the port is still open
  if (hPort == INVALID_HANDLE_VALUE)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Flush();

  /* this will cancel WaitCommEvent() */
  if (!is_widcomm)
    ::SetCommMask(hPort, 0);

  if (!Thread::Join(2000)) {
    /* On Dell Axim x51v, the Bluetooth RFCOMM driver seems to be
       bugged: when the peer gets disconnected (e.g. switched off),
       the function WaitCommEvent() does not get cancelled by
       SetCommMask(), but it should be according to MSDN.  As a
       workaround, we force WaitCommEvent() to abort by destroying the
       handle.  That seems to do the trick. */
    ::CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;

    Thread::Join();
  }

  return true;
}

bool
SerialPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::IsInside());

  // Make sure the port was opened correctly
  if (hPort == INVALID_HANDLE_VALUE)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
SerialPort::SetRxTimeout(unsigned Timeout)
{
  COMMTIMEOUTS CommTimeouts;

  if (hPort == INVALID_HANDLE_VALUE)
    return false;

  CommTimeouts.ReadIntervalTimeout = MAXDWORD;

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

    if (!is_embedded())
      Sleep(2000); // needed for windows bug

    SerialPort_StatusMessage(MB_OK, _("Error"),
                          _("Unable to set serial port timers %s"), sPortName);
    return false;
  }

#ifndef _WIN32_WCE
  rx_timeout = Timeout;
#endif

  return true;
}

unsigned
SerialPort::GetRxTimeout()
{
#ifdef _WIN32_WCE
  COMMTIMEOUTS CommTimeouts;
  return (::GetCommTimeouts(hPort, &CommTimeouts) ? CommTimeouts.ReadTotalTimeoutConstant : 0);
#else
  return rx_timeout;
#endif
}

unsigned long
SerialPort::SetBaudrate(unsigned long BaudRate)
{
  COMSTAT ComStat;
  DCB PortDCB;
  DWORD dwErrors;
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

int
SerialPort::Read(void *Buffer, size_t Size)
{
  DWORD dwBytesTransferred;

  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

#ifdef _WIN32_WCE

  if (ReadFile(hPort, Buffer, Size, &dwBytesTransferred, (OVERLAPPED *)NULL))
    return dwBytesTransferred;

  return -1;

#else
  OverlappedEvent osReader;

  int pending = WaitDataPending(osReader, rx_timeout);
  if (pending < 0)
    return -1;

  if (Size > (size_t)pending)
    /* don't request more bytes from the COMM buffer, because
       otherwise ReadFile() would block until the buffer has been
       filled completely */
    Size = pending;

  // Start reading data
  if (!::ReadFile(hPort, Buffer, Size, &dwBytesTransferred, osReader.GetPointer())) {
    if (::GetLastError() != ERROR_IO_PENDING)
      return -1;
  } else {
    // Process data that was directly read by ReadFile()
    return dwBytesTransferred;
  }

  // Let's wait for ReadFile() to finish
  switch (osReader.Wait(GetRxTimeout())) {
  case OverlappedEvent::FINISHED:
    // Get results
    if (!::GetOverlappedResult(hPort, osReader.GetPointer(), &dwBytesTransferred, FALSE))
      // Error occured while fetching results
      return -1;

    // Process data that was read with a delay by ReadFile()
    return dwBytesTransferred;

  default:
    return -1;
  }
#endif
}

void
SerialPort::ProcessChar(char c)
{
  FifoBuffer<char>::Range range = buffer.write();
  if (range.second == 0) {
    // overflow, so reset buffer
    buffer.clear();
    return;
  }

  if (c == '\n') {
    range.first[0] = _T('\0');
    buffer.append(1);

    range = buffer.read();
    handler.LineReceived(range.first);
    buffer.clear();
  } else if (c != '\r') {
    range.first[0] = c;
    buffer.append(1);
  }
}
