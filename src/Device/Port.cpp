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

#include "Device/Port.hpp"
#include "PeriodClock.hpp"

#include <assert.h>
#include <string.h>
#include <stdio.h>

Port::Port(Handler &_handler)
  :handler(_handler) {}

Port::~Port() {}

size_t
Port::Write(const char *s)
{
  return Write(s, strlen(s));
}

bool
Port::FullWrite(const void *buffer, size_t length, unsigned timeout_ms)
{
  PeriodClock timeout;
  timeout.update();

  const char *p = (const char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.check(timeout_ms))
      return false;

    size_t nbytes = Write(p, end - p);
    if (nbytes == 0)
      return false;

    p += nbytes;
  }

  return true;
}

int
Port::GetChar()
{
  unsigned char ch;
  return Read(&ch, sizeof(ch)) == sizeof(ch)
    ? ch
    : EOF;
}

bool
Port::FullRead(void *buffer, size_t length, unsigned timeout_ms)
{
  PeriodClock timeout;
  timeout.update();

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.check(timeout_ms))
      return false;

    int nbytes = Read(p, end - p);
    if (nbytes <= 0)
      return false;

    p += nbytes;
  }

  return true;
}

bool
Port::ExpectString(const char *token)
{
  assert(token != NULL);

  PeriodClock timeout;
  timeout.update();

  const char *p = token;
  while (*p != '\0') {
    int ch = GetChar();
    if (ch == EOF)
      return false;

    if (ch != *p++) {
      if (timeout.elapsed() > 2000)
        /* give up after 2 seconds (is that enough for all
           devices?) */
        return false;

      /* retry */
      p = token;
    }
  }

  return true;
}
