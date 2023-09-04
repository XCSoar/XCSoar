// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ZzipStream.hpp"
#include "lib/fmt/RuntimeError.hxx"

#include <zzip/util.h>

static int
jas_zzip_read(jas_stream_obj_t *obj, char *buf, unsigned cnt)
{
  const auto f = (struct zzip_file *)obj;

  return zzip_fread(buf, 1, cnt, f);
}

static int
jas_zzip_write([[maybe_unused]] jas_stream_obj_t *obj, [[maybe_unused]] const char *buf, [[maybe_unused]] unsigned cnt)
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

  return zzip_close(f);
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
    throw FmtRuntimeError("Failed to open '{}' from map file", path);

  jas_stream_t *stream = jas_stream_create();
  if (stream == nullptr) {
    zzip_close(f);
    throw std::runtime_error("jas_stream_create() failed");
  }

  stream->openmode_ = JAS_STREAM_READ|JAS_STREAM_BINARY;
  stream->obj_ = f;
  stream->ops_ = const_cast<jas_stream_ops_t *>(&zzip_stream_ops);

  /* By default, use full buffering for this type of stream. */
  jas_stream_initbuf(stream, JAS_STREAM_FULLBUF, 0, 0);

  return stream;
}
