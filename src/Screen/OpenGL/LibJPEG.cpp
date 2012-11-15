/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Screen/Bitmap.hpp"
#include "Texture.hpp"

#include <jpeglib.h>

#include <setjmp.h>

struct JPEGErrorManager {
  struct jpeg_error_mgr base;

  jmp_buf setjmp_buffer;

  JPEGErrorManager() {
    base.error_exit = ErrorExit;
  }

  gcc_noreturn
  void ErrorExit() {
    longjmp(setjmp_buffer, 1);
  }

  gcc_noreturn
  static void ErrorExit(j_common_ptr cinfo) {
    JPEGErrorManager *err = reinterpret_cast<JPEGErrorManager *>(cinfo);
    err->ErrorExit();
  }
};

static GLTexture *
ImportJPEG(unsigned width, unsigned height, const void *data)
{
  GLint internal_format = GL_RGB;
  GLenum format = GL_RGB, type = GL_UNSIGNED_BYTE;

  return new GLTexture(internal_format, width, height,
                       format, type, data);
}

bool
Bitmap::LoadFile(const TCHAR *path)
{
  FILE *file = fopen(path, "rb");
  if (file == nullptr)
    return false;

  struct jpeg_decompress_struct cinfo;

  JPEGErrorManager err;
  cinfo.err = jpeg_std_error(&err.base);
  if (setjmp(err.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return false;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, true);

  if (cinfo.num_components != 3) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return false;
  }

  cinfo.out_color_space = JCS_RGB;
  cinfo.quantize_colors = false;
  jpeg_calc_output_dimensions(&cinfo);

  jpeg_start_decompress(&cinfo);

  const unsigned width = cinfo.output_width;
  const unsigned height = cinfo.output_height;

  const size_t row_size = 3 * width;
  const size_t image_buffer_size = row_size * height;
  const size_t row_buffer_size = row_size;
  /* allocate a buffer that holds the uncompressed image plus a row
     buffer with packed 24 bit samples (for libjpeg) */
  uint8_t *const image_buffer =
    new uint8_t[image_buffer_size + row_buffer_size];
  if (image_buffer == nullptr) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return nullptr;
  }

  uint8_t *const row = image_buffer + image_buffer_size;
  JSAMPROW rowptr[1] = { row };

  uint8_t *p = image_buffer;
  while (cinfo.output_scanline < height) {
    jpeg_read_scanlines(&cinfo, rowptr, (JDIMENSION)1);

    p = std::copy(row, row + row_size, p);
  }

  assert(p == image_buffer + image_buffer_size);

  texture = ImportJPEG(width, height, image_buffer);
  delete[] image_buffer;

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(file);

  size = texture->GetSize();
  return true;
}
