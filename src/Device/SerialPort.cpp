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

#include "Device/SerialPort.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Message.hpp"
#include "Asset.hpp"

#ifdef HAVE_POSIX
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#else /* !HAVE_POSIX */
#include <windows.h>
#endif /* !HAVE_POSIX */

#include <assert.h>
#include <tchar.h>
#include <stdio.h>

#define COMMDEBUG 0

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
#ifdef HAVE_POSIX
   fd(-1),
#else
   hPort(INVALID_HANDLE_VALUE),
   dwMask(0),
#endif
   buffer(NMEA_BUF_SIZE)
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
#ifdef HAVE_POSIX
  fd = open(sPortName, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    SerialPort_StatusMessage(MB_OK | MB_ICONINFORMATION, NULL,
                          _("Unable to open port %s"), sPortName);
    return false;
  }
#else /* !HAVE_POSIX */
  DCB PortDCB;

  buffer.clear();

  // Open the serial port.
  hPort = CreateFile(sPortName,    // Pointer to the name of the port
                     GENERIC_READ | GENERIC_WRITE, // Access (read-write) mode
                     0,            // Share mode
                     NULL,         // Pointer to the security attribute
                     OPEN_EXISTING,// How to open the serial port
                     FILE_ATTRIBUTE_NORMAL, // Port attributes
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

    if (!is_embedded())
      Sleep(2000); // needed for windows bug

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
#endif /* !HAVE_POSIX */

  if (!StartRxThread()){
#ifdef HAVE_POSIX
    close(fd);
    fd = -1;
#else /* !HAVE_POSIX */
    CloseHandle(hPort);
    hPort = INVALID_HANDLE_VALUE;
#endif /* !HAVE_POSIX */

    if (!is_embedded())
      Sleep(2000); // needed for windows bug

    return false;
  }

  return true;
}

void
SerialPort::Flush(void)
{
#ifdef HAVE_POSIX
  // XXX
#else /* !HAVE_POSIX */
  PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
#endif /* !HAVE_POSIX */
}

void
SerialPort::run()
{
#ifdef HAVE_POSIX
  char buffer[1024];

  // XXX use poll()
  while (!wait_stopped(50)) {
    ssize_t nbytes = read(fd, buffer, sizeof(buffer));
    for (ssize_t i = 0; i < nbytes; ++i)
      ProcessChar(buffer[i]);
  }
#else /* !HAVE_POSIX */
  DWORD dwCommModemStatus, dwBytesTransferred;
  BYTE inbuf[1024];

  // JMW added purging of port on open to prevent overflow
  Flush();

  // Specify a set of events to be monitored for the port.
  dwMask = EV_RXFLAG | EV_CTS | EV_DSR | EV_RING | EV_RXCHAR;

  if (is_embedded())
    SetCommMask(hPort, dwMask);

  while (!is_stopped()) {

    if (is_embedded()) {
      // Wait for an event to occur for the port.
      if (!WaitCommEvent(hPort, &dwCommModemStatus, 0)) {
        // error reading from port
        Sleep(100);
        continue;
      }

      if ((dwCommModemStatus & EV_RXCHAR) == 0)
        /* no data available */
        continue;
    } else {
      Sleep(50); // ToDo rewrite the whole driver to use overlaped IO
      // on W2K or higher
    }

    // Read the data from the serial port.
    if (!ReadFile(hPort, inbuf, 1024, &dwBytesTransferred, NULL)) {
      // ignore everything until started
      Sleep(100);
      continue;
    }

    for (unsigned int j = 0; j < dwBytesTransferred; j++)
      ProcessChar(inbuf[j]);

    Sleep(50); // JMW20070515: give port some time to
    // fill... prevents ReadFile from causing the
    // thread to take up too much CPU
  }
#endif /* !HAVE_POSIX */

  Flush();
}

bool
SerialPort::Close()
{
#ifdef HAVE_POSIX
  if (fd < 0)
    return true;

  StopRxThread();

  close(fd);
  fd = -1;
  return true;
#else /* !HAVE_POSIX */
  DWORD dwError;

  if (hPort != INVALID_HANDLE_VALUE) {
    StopRxThread();
    Sleep(100); // todo ...

    dwError = 0;

    // Close the communication port.
    if (!CloseHandle(hPort)) {
      dwError = GetLastError();
      return false;
    } else {
      if (!is_embedded())
        Sleep(2000); // needed for windows bug

      hPort = INVALID_HANDLE_VALUE;
      return true;
    }
  }

  return false;
#endif /* !HAVE_POSIX */
}

void
SerialPort::Write(const void *data, unsigned length)
{
#ifdef HAVE_POSIX
  if (fd < 0)
    return;

  write(fd, data, length);
#else /* !HAVE_POSIX */
  if (hPort == INVALID_HANDLE_VALUE)
    return;

  ::WriteFile(hPort, data, length, NULL, NULL);
#endif /* !HAVE_POSIX */
}

bool
SerialPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::inside());

  // Make sure the port is still open
#ifdef HAVE_POSIX
  if (fd < 0)
    return false;
#else /* !HAVE_POSIX */
  if (hPort == INVALID_HANDLE_VALUE)
    return false;
#endif /* !HAVE_POSIX */

  // If the thread is not running, cancel the rest of the function
  if (!Thread::defined())
    return true;

  stop();

#ifndef HAVE_POSIX
  if (is_embedded()) {
    Flush();

    // this will cancel any WaitCommEvent!
    // this is a documented CE trick to cancel the WaitCommEvent
    SetCommMask(hPort, dwMask);
  }
#endif /* !HAVE_POSIX */

  // Stop the thread
  Thread::join();
  return true;
}

bool
SerialPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::inside());

  // Make sure the port was opened correctly
#ifdef HAVE_POSIX
  if (fd < 0)
    return false;
#else /* !HAVE_POSIX */
  if (hPort == INVALID_HANDLE_VALUE)
    return false;
#endif /* !HAVE_POSIX */

  // Start the receive thread
  StoppableThread::start();
  return true;
}

int
SerialPort::GetChar(void)
{
#ifdef HAVE_POSIX
  if (fd < 0)
    return EOF;

  char buffer;
  if (read(fd, &buffer, sizeof(buffer)) != 1)
    return EOF;

  return buffer;
#else /* !HAVE_POSIX */
  BYTE  inbuf[2];
  DWORD dwBytesTransferred;

  if (hPort == INVALID_HANDLE_VALUE || Thread::defined())
    return EOF;

  if (ReadFile(hPort, inbuf, 1, &dwBytesTransferred, (OVERLAPPED *)NULL)) {
    if (dwBytesTransferred == 1)
      return inbuf[0];
  }
#endif /* !HAVE_POSIX */

  return EOF;
}

int
SerialPort::SetRxTimeout(int Timeout)
{
#ifdef HAVE_POSIX
  return Timeout; // XXX
#else /* !HAVE_POSIX */
  COMMTIMEOUTS CommTimeouts;
  int result;
  DWORD dwError;

  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

  GetCommTimeouts(hPort, &CommTimeouts);

  result = CommTimeouts.ReadTotalTimeoutConstant;

  // Change the COMMTIMEOUTS structure settings.
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
    dwError = GetLastError();
    return -1;
  }

  return result;
#endif /* !HAVE_POSIX */
}

unsigned long
SerialPort::SetBaudrate(unsigned long BaudRate)
{
#ifdef HAVE_POSIX
  return BaudRate; // XXX
#else /* !HAVE_POSIX */
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
#endif /* !HAVE_POSIX */
}

int
SerialPort::Read(void *Buffer, size_t Size)
{
#ifdef HAVE_POSIX
  if (fd < 0)
    return -1;

  return read(fd, Buffer, Size);
#else /* !HAVE_POSIX */
  DWORD dwBytesTransferred;

  if (hPort == INVALID_HANDLE_VALUE)
    return -1;

  if (ReadFile(hPort, Buffer, Size, &dwBytesTransferred, (OVERLAPPED *)NULL))
    return dwBytesTransferred;

  return -1;
#endif /* !HAVE_POSIX */
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
