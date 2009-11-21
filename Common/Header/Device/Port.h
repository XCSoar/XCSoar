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

#if !defined(AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#define NMEA_BUF_SIZE 100

// Forward declaration
struct DeviceDescriptor;

class ComPort {
public:
  ComPort(struct DeviceDescriptor *d);
  ~ComPort() { };

  void PutChar(BYTE);
  void WriteString(const TCHAR *);
  void Flush();

  BOOL Initialize(LPCTSTR, DWORD);
  BOOL Close();

  int SetRxTimeout(int);
  unsigned long SetBaudrate(unsigned long);

  BOOL StopRxThread();
  BOOL StartRxThread();
  void ProcessChar(char);

  int GetChar();
  int Read(void *Buffer, size_t Size);

private:
  static DWORD WINAPI ThreadProc(LPVOID);
  DWORD ReadThread();

  HANDLE hPort;
  HANDLE hReadThread;
  DWORD dwMask;
  TCHAR sPortName[8];
  BOOL CloseThread;
  BOOL fRxThreadTerminated;

  TCHAR BuildingString[NMEA_BUF_SIZE];
  int bi;
  struct DeviceDescriptor *dev;
};

#endif
