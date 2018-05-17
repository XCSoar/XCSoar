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

#include "ZzipStream.hpp"

#include <zzip/util.h>

static int
jas_zzip_read(jas_stream_obj_t *obj, char *buf, int cnt)
{
  const auto f = (struct zzip_file *)obj;

  return zzip_fread(buf, 1, cnt, f);
}

static int
jas_zzip_write(jas_stream_obj_t *obj, char *buf, int cnt)
{
  return -1;
}

static long
jas_zzip_seek(jas_stream_obj_t *obj, long offset, int origin)
{
  const auto f = (struct zzip_file *)obj;

  return zzip_seek(f, offset, origin);
}

static int
jas_zzip_close(jas_stream_obj_t *obj)
{
  const auto f = (struct zzip_file *)obj;

  return zzip_file_close(f);
}

static constexpr jas_stream_ops_t zzip_stream_ops = {
  jas_zzip_read,
  jas_zzip_write,
  jas_zzip_seek,
  jas_zzip_close
};

jas_stream_t *
OpenJasperZzipStream(struct zzip_dir *dir, const char *path)
{
  const auto f = zzip_open_rb(dir, path);
  if (f == nullptr)
    return nullptr;

  jas_stream_t *stream = jas_stream_create();
  if (stream == nullptr) {
    zzip_file_close(f);
    return nullptr;
  }

  stream->openmode_ = JAS_STREAM_READ|JAS_STREAM_BINARY;
  stream->obj_ = f;
  stream->ops_ = const_cast<jas_stream_ops_t *>(&zzip_stream_ops);

  /* By default, use full buffering for this type of stream. */
  jas_stream_initbuf(stream, JAS_STREAM_FULLBUF, 0, 0);

  return stream;
}
