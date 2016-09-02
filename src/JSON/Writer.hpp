/*
Copyright_License {

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

#ifndef XCSOAR_JSON_WRITER_HPP
#define XCSOAR_JSON_WRITER_HPP

#include "IO/BufferedOutputStream.hxx"

#include <assert.h>

namespace JSON {
  /**
   * Writer for a JSON "null" value.
   */
  static inline void WriteNull(BufferedOutputStream &writer) {
    writer.Write("null");
  }

  /**
   * Writer for a JSON integer value.
   */
  static inline void WriteInteger(BufferedOutputStream &writer, int value) {
    writer.Format("%d", value);
  }

  /**
   * Writer for a JSON integer value.
   */
  static inline void WriteUnsigned(BufferedOutputStream &writer, unsigned value) {
    writer.Format("%u", value);
  }

  /**
   * Writer for a JSON integer value.
   */
  static inline void WriteLong(BufferedOutputStream &writer, long value) {
    writer.Format("%ld", value);
  }

  /**
   * Writer for a JSON string.
   */
  void WriteString(BufferedOutputStream &writer, const char *value);

  /**
   * Generate a JSON array.  The constructor/destructor
   * begins/finishes the array.
   *
   * To write an element manually, call BeginElement(), then write the
   * value, then call EndElement().  WriteElement() combines these
   * three steps in one method call.
   */
  class ArrayWriter {
    BufferedOutputStream &writer;
    bool first;

  public:
    ArrayWriter(BufferedOutputStream &_writer):writer(_writer), first(true) {
      writer.Write('[');
    }

    ~ArrayWriter() {
      writer.Write(']');
    }

    void BeginElement() {
      if (first)
        first = false;
      else
        writer.Write(',');
    }

    void EndElement() {
      assert(!first);
    }

    /**
     * Write an element using a callback.  The first callback argument
     * is a reference to the BufferedOutputStream.  Additional arguments are
     * passed to the callback.
     */
    template<typename T, typename... Args>
    void WriteElement(T callback, Args... args) {
      BeginElement();
      callback(writer, args...);
      EndElement();
    }
  };

  /**
   * Generate a JSON object (containing unordered name/value pairs).
   * The constructor/destructor begins/finishes the object.
   *
   * To write an element manually, call BeginElement(), then write the
   * value, then call EndElement().  WriteElement() combines these
   * three steps in one method call.
   */
  class ObjectWriter {
    BufferedOutputStream &writer;
    bool first;

  public:
    ObjectWriter(BufferedOutputStream &_writer):writer(_writer), first(true) {
      writer.Write('{');
    }

    ~ObjectWriter() {
      writer.Write('}');
    }

    void BeginElement(const char *name) {
      if (first)
        first = false;
      else
        writer.Write(',');

      WriteString(writer, name);
      writer.Write(':');
    }

    void EndElement() {
      assert(!first);
    }

    /**
     * Write an element using a callback.  The first callback argument
     * is a reference to the BufferedOutputStream.  Additional arguments are
     * passed to the callback.
     */
    template<typename T, typename... Args>
    void WriteElement(const char *name, T callback, Args... args) {
      BeginElement(name);
      callback(writer, args...);
      EndElement();
    }
  };
};

#endif

