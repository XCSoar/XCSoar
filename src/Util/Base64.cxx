/*
 * Copyright (C) 2015-2016 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Base64.hxx"
#include "Util/ConstBuffer.hxx"

static char *
Base64(char *dest, uint8_t a, uint8_t b, uint8_t c)
{
  static constexpr char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  *dest++ = base64[a >> 2];
  *dest++ = base64[((a & 0x03) << 4) | (b >> 4)];
  *dest++ = base64[((b & 0x0f) << 2) | (c >> 6)];
  *dest++ = base64[c & 0x3f];
  return dest;
}

char *
Base64(char *dest, ConstBuffer<uint8_t> src)
{
  while (src.size >= 3) {
    dest = Base64(dest, src[0], src[1], src[2]);
    src.skip_front(3);
  }

  if (src.size == 2) {
    dest = Base64(dest, src[0], src[1], 0);
    dest[-1] = '=';
  } else if (src.size == 1) {
    dest = Base64(dest, src[0], 0, 0);
    dest[-2] = '=';
    dest[-1] = '=';
  }

  return dest;
}
