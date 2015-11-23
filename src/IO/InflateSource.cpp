/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "InflateSource.hpp"

InflateSource::InflateSource(Source<char> &_src)
  :src(_src)
{
  auto compressed = src.Read();
  /* we need the const_cast because Android NDK r10e contains ZLib
     1.2.3 headers without ZLIB_CONST support */
  strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(compressed.data));
  strm.avail_in = compressed.size;
  strm.zalloc = nullptr;
  strm.zfree = nullptr;
  strm.opaque = nullptr;

  int result = inflateInit2(&strm, 16 + MAX_WBITS);
  if (result != Z_OK)
    strm.opaque = this;
}

InflateSource::~InflateSource()
{
  if (strm.opaque == nullptr)
    inflateEnd(&strm);
}

unsigned
InflateSource::Read(char *p, unsigned n)
{
  while (true) {
    auto compressed = src.Read();
    if (compressed.IsEmpty())
      return 0;

    /* we need the const_cast because Android NDK r10e contains ZLib
       1.2.3 headers without ZLIB_CONST support */
    strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(compressed.data));
    strm.avail_in = compressed.size;
    strm.next_out = reinterpret_cast<Bytef *>(p);
    strm.avail_out = n;

    int result = inflate(&strm, Z_NO_FLUSH);
    if (result != Z_OK && result != Z_STREAM_END)
      return 0;

    src.Consume(compressed.size - strm.avail_in);

    if (strm.avail_out < n)
      return n - strm.avail_out;
  }
}
