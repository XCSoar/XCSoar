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

#include "LibJPEG.hpp"
#include "UncompressedImage.hpp"
#include "OS/Path.hpp"
#include "Util/ScopeExit.hxx"

#include <algorithm>
#include <stdexcept>

#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>

extern "C" {
#include <jpeglib.h>
}

[[noreturn]]
static void
JpegErrorExit(j_common_ptr cinfo)
{
  char msg[JMSG_LENGTH_MAX];
  cinfo->err->format_message(cinfo, msg);
  throw std::runtime_error(msg);
}

UncompressedImage
LoadJPEGFile(Path path)
{
  FILE *file = _tfopen(path.c_str(), _T("rb"));
  if (file == nullptr)
    return UncompressedImage();

  AtScopeExit(file) { fclose(file); };

  struct jpeg_decompress_struct cinfo;

  struct jpeg_error_mgr err;
  cinfo.err = jpeg_std_error(&err);
  err.error_exit = JpegErrorExit;

  jpeg_create_decompress(&cinfo);

  AtScopeExit(&cinfo) { jpeg_destroy_decompress(&cinfo); };

  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, (boolean)true);

  if (cinfo.num_components != 3)
    return UncompressedImage();

  cinfo.out_color_space = JCS_RGB;
  cinfo.quantize_colors = (boolean)false;
  jpeg_calc_output_dimensions(&cinfo);

  jpeg_start_decompress(&cinfo);

  const unsigned width = cinfo.output_width;
  const unsigned height = cinfo.output_height;

  const size_t row_size = 3 * width;
  const size_t image_buffer_size = row_size * height;
  const size_t row_buffer_size = row_size;
  /* allocate a buffer that holds the uncompressed image plus a row
     buffer with packed 24 bit samples (for libjpeg) */
  std::unique_ptr<uint8_t[]> image_buffer(new uint8_t[image_buffer_size
                                                      + row_buffer_size]);

  uint8_t *const row = image_buffer.get() + image_buffer_size;
  JSAMPROW rowptr[1] = { row };

  uint8_t *p = image_buffer.get();
  while (cinfo.output_scanline < height) {
    jpeg_read_scanlines(&cinfo, rowptr, (JDIMENSION)1);

    p = std::copy_n(row, row_size, p);
  }

  assert(p == image_buffer.get() + image_buffer_size);

  jpeg_finish_decompress(&cinfo);

  return UncompressedImage(UncompressedImage::Format::RGB,
                           row_size, width, height,
                           std::move(image_buffer));
}
