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

#ifndef XCSOAR_IO_CONVERT_LINE_READER_HPP
#define XCSOAR_IO_CONVERT_LINE_READER_HPP

#include "LineReader.hpp"
#include "Util/ReusableArray.hpp"

/**
 * Adapter which converts data from LineReader<char> to
 * LineReader<TCHAR>.
 */
class ConvertLineReader : public TLineReader {
public:
  enum charset {
    UTF8,
    ISO_LATIN_1,
  };

protected:
  LineReader<char> &source;

  charset m_charset;

#ifdef _UNICODE
  unsigned code_page;
#endif

  ReusableArray<TCHAR> tbuffer;

public:
  ConvertLineReader(LineReader<char> &_source, charset cs=UTF8);

public:
  virtual TCHAR *read();
  virtual long size() const;
  virtual long tell() const;
};

#endif
