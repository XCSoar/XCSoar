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

#ifndef XCSOAR_IO_BUFFERED_SOURCE_HPP
#define XCSOAR_IO_BUFFERED_SOURCE_HPP

#include "FifoBuffer.hpp"
#include "Source.hpp"

template<class T, unsigned size>
class BufferedSource : public Source<T> {
public:
  typedef std::pair<T*, unsigned> Range;

private:
  FifoBuffer<T, size> buffer;
  long position;

public:
  BufferedSource():position(0) {}

protected:
  virtual unsigned read(T *p, unsigned n) = 0;

public:
  virtual Range read() {
    Range r = buffer.Write();
    if (r.second > 0) {
      unsigned n = read(r.first, r.second);
      buffer.Append(n);
    }

    return buffer.Read();
  }

  virtual void consume(unsigned n) {
    buffer.Consume(n);
    position += n;
  }

  virtual long tell() const {
    return position;
  }
};

#endif
