/* Copyright_License {

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

#ifndef MD5_HPP
#define MD5_HPP

#include <stdint.h>

class MD5
{
public:
  enum {
    DIGEST_LENGTH = 16,
  };

private:
  unsigned char buff512bits[64];
  uint32_t a, b, c, d;
  uint32_t h0, h1, h2, h3;
  uint32_t f, g;
  // max message size = 536870912
  // because of 32-bit length tracking (MD5 standard is 64-bits)
  uint32_t message_length_bits;

  void Process512(const unsigned char *in);

public:
  void InitKey(uint32_t h0in, uint32_t h1in, uint32_t h2in, uint32_t h3in);

  void InitDigest(void);
  void AppendString(const unsigned char *in, int skip_invalid_igc_chars); // must be NULL-terminated string!
  void Finalize(void);
  int GetDigest(char *buffer);
  static bool IsValidIGCChar(char c);
};

#endif
