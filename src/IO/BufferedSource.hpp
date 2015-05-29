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

#ifndef XCSOAR_IO_BUFFERED_SOURCE_HPP
#define XCSOAR_IO_BUFFERED_SOURCE_HPP

#include "Util/StaticFifoBuffer.hpp"
#include "Source.hpp"

template<class T, unsigned size>
class BufferedSource : public Source<T> {
private:
  StaticFifoBuffer<T, size> buffer;
  long position;

public:
  BufferedSource():position(0) {}

protected:
  virtual unsigned Read(T *p, unsigned n) = 0;

public:
  typename Source<T>::Range Read() override {
    buffer.Shift();
    auto r = buffer.Write();
    if (!r.IsEmpty()) {
      unsigned n = Read(r.data, r.size);
      buffer.Append(n);
    }

    return buffer.Read();
  }

  void Consume(unsigned n) override {
    buffer.Consume(n);
    position += n;
  }

  long Tell() const override {
    return position;
  }
};

#endif
