/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "SerialPort.hpp"
#include "Asset.hpp"
#include "OS/LogError.hpp"
#include "OS/Sleep.h"

#ifdef _WIN32_WCE
#include "Widcomm.hpp"
#else
#include "OS/OverlappedEvent.hpp"
#endif

#include <windows.h>

#include <algorithm>
#include <assert.h>
#include <tchar.h>
#include <stdio.h>

SerialPort::SerialPort(DataHandler &_handler)
  :BufferedPort(_handler)
{
}

SerialPort::~SerialPort()
{
  BufferedPort::BeginClose();

  // Close the communication port.
  if (hPort != INVALID_HANDLE_VALUE) {
    StoppableThread::BeginStop();

    if (CloseHandle(hPort) && !IsEmbedded())
      Sleep(2000); // needed for windows bug

    Thread::Join();
  }

  BufferedPort::EndClose();
}

bool
SerialPort::Open(const TCHAR *path, unsigned _baud_rate)
{
  assert(!Thread::IsInside());

#ifdef _WIN32_WCE
  is_widcomm = IsWidcommDevice(path);
#endif

  DCB PortDCB;

  // Open the serial port.
  hPort = CreateFile(path,
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
    LogLastError(_T("Failed to open port '%s'"), path);
    return false;
  }

  baud_rate = _baud_rate;

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
    LogLastError(_T("Failed to configure port '%s'"), path);
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;
    return false;
  }

  SetupComm(hPort, 1024, 1024);

  // Direct the port to perform extended functions SETDTR and SETRTS
  // SETDTR: Sends the DTR (data-terminal-ready) signal.
  EscapeCommFunction(hPort, SETDTR);
  // SETRTS: Sends the RTS (request-to-send) signal.
  EscapeCommFunction(hPort, SETRTS);

  StoppableThread::Start();

  return true;
}

bool
SerialPort::IsValid() const
{
  return hPort != INVALID_HANDLE_VALUE;
}

bool
SerialPort::Drain()
{
  int nbytes = GetDataQueued();
  if (nbytes < 0)
    return false;
  else if (nbytes == 0)
    return true;

  ::SetCommMask(hPort, EV_ERR|EV_TXEMPTY);
  DWORD events;
  return WaitCommEvent(hPort, &events, NULL) &&
    (events & EV_TXEMPTY) != 0;
}

void
SerialPort::Flush()
{
  PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
  BufferedPort::Flush();
}

int
SerialPort::GetDataQueued() const
{
  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

  COMSTAT com_stat;
  DWORD errors;
  return ::ClearCommError(hPort, &errors, &com_stat)
    ? (int)com_stat.cbOutQue
    : -1;
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

Port::WaitResult
SerialPort::WaitDataPending(OverlappedEvent &overlapped,
                            unsigned timeout_ms) const
{
  int nbytes = GetDataPending();
  if (nbytes > 0)
    return WaitResult::READY;
  else if (nbytes < 0)
    return WaitResult::FAILED;

  ::SetCommMask(hPort, EV_RXCHAR);

  DWORD dwCommModemStatus;
  if (!::WaitCommEvent(hPort, &dwCommModemStatus, overlapped.GetPointer())) {
    if (::GetLastError() != ERROR_IO_PENDING)
      return WaitResult::FAILED;

    switch (overlapped.Wait(timeout_ms)) {
    case OverlappedEvent::FINISHED:
      break;

    case OverlappedEvent::TIMEOUT:
      /* the operation may still be running, we have to cancel it */
      ::CancelIo(hPort);
      ::SetCommMask(hPort, 0);
      overlapped.Wait();
      return WaitResult::TIMEOUT;

    case OverlappedEvent::CANCELED:
      /* the operation may still be running, we have to cancel it */
      ::CancelIo(hPort);
      ::SetCommMask(hPort, 0);
      overlapped.Wait();
      return WaitResult::CANCELLED;
    }

    DWORD result;
    if (!::GetOverlappedResult(hPort, overlapped.GetPointer(), &result, FALSE))
      return WaitResult::FAILED;
  }

  if ((dwCommModemStatus & EV_RXCHAR) == 0)
      return WaitResult::FAILED;

  return GetDataPending() > 0
    ? WaitResult::READY
    : WaitResult::FAILED;
}

#endif

void
SerialPort::Run()
{
  assert(Thread::IsInside());

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
  else {
    ::SetCommMask(hPort, EV_RXCHAR);
    SetRxTimeout(0);
  }

  while (!CheckStopped()) {

#ifndef _WIN32_WCE

    WaitResult result = WaitDataPending(osStatus, INFINITE);
    switch (result) {
    case WaitResult::READY:
      break;

    case WaitResult::TIMEOUT:
      continue;

    case WaitResult::FAILED:
    case WaitResult::CANCELLED:
      ::Sleep(100);
      continue;
    }

    int nbytes = GetDataPending();
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
        ::SetCommMask(hPort, 0);
        osReader.Wait();
        continue;
      }

      if (!::GetOverlappedResult(hPort, osReader.GetPointer(),
                                 &dwBytesTransferred, FALSE))
        continue;
    }

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
#endif

    DataReceived(inbuf, dwBytesTransferred);
  }

  Flush();
}

size_t
SerialPort::Write(const void *data, size_t length)
{
  DWORD NumberOfBytesWritten;

  if (hPort == INVALID_HANDLE_VALUE)
    return 0;

#ifdef _WIN32_WCE

#if 0
  /* this workaround is currently disabled because it causes major
    problems with some of our device drivers, causing timeouts; this
    may be a regression on the bugged HP31x, but I prefer to support
    sane platforms any day */

  if (IsWindowsCE() && !IsAltair())
    /* this is needed to work around a driver bug on the HP31x -
       without it, the second consecutive write without a task switch
       will hang the whole PNA; this Sleep() call enforces a task
       switch */
    Sleep(100);
#endif

  // lpNumberOfBytesWritten : This parameter can be NULL only when the lpOverlapped parameter is not NULL.
  if (!::WriteFile(hPort, data, length, &NumberOfBytesWritten, NULL))
    return 0;

  return NumberOfBytesWritten;

#else

  OverlappedEvent osWriter;

  // Start reading data
  if (::WriteFile(hPort, data, length, &NumberOfBytesWritten, osWriter.GetPointer()))
    return NumberOfBytesWritten;

  if (::GetLastError() != ERROR_IO_PENDING)
    return 0;

  // Let's wait for ReadFile() to finish
  unsigned timeout_ms = 1000 + length * 10;
  switch (osWriter.Wait(timeout_ms)) {
  case OverlappedEvent::FINISHED:
    // Get results
    ::GetOverlappedResult(hPort, osWriter.GetPointer(), &NumberOfBytesWritten, FALSE);
    return NumberOfBytesWritten;

  default:
    ::CancelIo(hPort);
    ::SetCommMask(hPort, 0);
    osWriter.Wait();
    return 0;
  }
#endif
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

  CommTimeouts.WriteTotalTimeoutMultiplier = 0;
  CommTimeouts.WriteTotalTimeoutConstant = 1000;

  // Set the time-out parameters
  // for all read and write
  // operations on the port.
  return SetCommTimeouts(hPort, &CommTimeouts);
}

unsigned
SerialPort::GetBaudrate() const
{
  if (hPort == INVALID_HANDLE_VALUE)
    return 0;

  DCB PortDCB;
  GetCommState(hPort, &PortDCB);

  return PortDCB.BaudRate;
}

bool
SerialPort::SetBaudrate(unsigned BaudRate)
{
  COMSTAT ComStat;
  DCB PortDCB;
  DWORD dwErrors;

  if (hPort == INVALID_HANDLE_VALUE)
    return false;

  do {
    ClearCommError(hPort, &dwErrors, &ComStat);
  } while (ComStat.cbOutQue > 0);

  Sleep(10);

  GetCommState(hPort, &PortDCB);

  PortDCB.BaudRate = BaudRate;

  return SetCommState(hPort, &PortDCB);
}
