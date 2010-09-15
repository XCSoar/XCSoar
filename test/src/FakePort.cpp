/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Device/Port.hpp"

#include <stdio.h>

ComPort::ComPort(const TCHAR *path, unsigned _baud_rate, Handler &_handler)
  :handler(_handler),
   stop_trigger(_T("ComPort::stop_trigger"), true),
   buffer(NMEA_BUF_SIZE) {}

void ComPort::run() {}

bool ComPort::Close() { return true; }

int ComPort::SetRxTimeout(int) { return 0; }
unsigned long ComPort::SetBaudrate(unsigned long baud) { return baud; }

bool ComPort::StopRxThread() { return true; }
bool ComPort::StartRxThread() { return true; }

void
ComPort::Write(const void *data, unsigned length)
{
  fwrite(data, length, 1, stdout);
}

void
ComPort::Write(const char *text)
{
  fputs(text, stdout);
}

void
ComPort::Flush()
{
  fflush(stdout);
}

int
ComPort::GetChar()
{
  return getchar();
}

int
ComPort::Read(void *Buffer, size_t Size)
{
  return 0;
}

bool
ComPort::ExpectString(const char *token)
{
  return true;
}
