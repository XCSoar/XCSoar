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

#ifndef XCSOAR_IO_ZIP_SOURCE_HPP
#define XCSOAR_IO_ZIP_SOURCE_HPP

#include "BufferedSource.hpp"

struct zzip_file;
struct zzip_dir;
class Error;

class ZipSource : public BufferedSource<char, 4096u> {
private:
  struct zzip_file *file;

public:
  ZipSource(struct zzip_dir *dir, const char *path, Error &error);

  virtual ~ZipSource();

  bool error() const {
    return file == nullptr;
  }

public:
  /* virtual methods from class Source */
  long GetSize() const override;

protected:
  /* virtual methods from class BufferedSource */
  unsigned Read(char *p, unsigned n) override;
};

#endif
