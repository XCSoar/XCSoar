/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include <stddef.h>

class MD5
{
public:
  static constexpr size_t DIGEST_LENGTH = 32;

  struct State {
    uint32_t a, b, c, d;
  };

private:
  uint8_t buff512bits[64];
  State state;
  uint64_t message_length;

  void Process512(const uint8_t *in);

public:
  /**
   * Initialise with a custom key.
   */
  void Initialise(const State &_state) {
    state = _state;
    message_length = 0;
  }

  /**
   * Initialise with the default key.
   */
  void Initialise();

  void Append(uint8_t ch);
  void Append(const void *data, size_t length);

  void Finalize();

  /**
   * @param buffer a buffer of at least #DIGEST_LENGTH+1 bytes
   * @return a pointer to the null terminator
   */
  char *GetDigest(char *buffer) const;
};

#endif
