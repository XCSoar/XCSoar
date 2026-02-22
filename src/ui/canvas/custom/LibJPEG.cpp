// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LibJPEG.hpp"
#include "UncompressedImage.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <algorithm>
#include <stdexcept>

#include <cassert>
#include <stdio.h>
#include <cstddef>

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

static UncompressedImage
DecodeJPEG(struct jpeg_decompress_struct &cinfo)
{
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

UncompressedImage
LoadJPEGFile(Path path)
{
  FILE *file = fopen(path.c_str(), "rb");
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

  return DecodeJPEG(cinfo);
}

UncompressedImage
LoadJPEG(std::span<const std::byte> buffer)
{
  assert(buffer.data() != nullptr);

  struct jpeg_decompress_struct cinfo;

  struct jpeg_error_mgr err;
  cinfo.err = jpeg_std_error(&err);
  err.error_exit = JpegErrorExit;

  jpeg_create_decompress(&cinfo);
  AtScopeExit(&cinfo) { jpeg_destroy_decompress(&cinfo); };

  jpeg_mem_src(&cinfo,
               reinterpret_cast<const unsigned char *>(buffer.data()),
               buffer.size());
  jpeg_read_header(&cinfo, (boolean)true);

  return DecodeJPEG(cinfo);
}
