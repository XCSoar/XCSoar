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

#include <memory>
#include <utility>

#include "CoreGraphics.hpp"
#include "UncompressedImage.hpp"
#include "OS/Path.hpp"

#import <CoreGraphics/CoreGraphics.h>

static UncompressedImage
CGImageToUncompressedImage(CGImageRef image)
{
  if (image == nullptr)
    return UncompressedImage();
  
  size_t width = CGImageGetWidth(image);
  size_t height = CGImageGetHeight(image);
  
  if ((0 == width) || (0 == height))
    return UncompressedImage();
  
  size_t bits_per_pixel = CGImageGetBitsPerPixel(image);
  size_t bits_per_component = CGImageGetBitsPerComponent(image);
  CGColorSpaceRef colorspace = CGImageGetColorSpace(image);
  
  size_t row_size;
  UncompressedImage::Format format;
  CGColorSpaceRef bitmap_colorspace;
  CGBitmapInfo bitmap_info;
  
  if ((8 == bits_per_pixel) &&
      (8 == bits_per_component) &&
      (CGColorSpaceGetModel(colorspace) == kCGColorSpaceModelMonochrome)) {
    row_size = width;
    format = UncompressedImage::Format::GRAY;
    static CGColorSpaceRef grey_colorspace = CGColorSpaceCreateDeviceGray();
    bitmap_colorspace = grey_colorspace;
    bitmap_info = 0;
  } else {
    static CGColorSpaceRef rgb_colorspace = CGColorSpaceCreateDeviceRGB();
    bitmap_colorspace = rgb_colorspace;
    if ((24 == bits_per_pixel) && (8 == bits_per_component)) {
      row_size = width * 3;
      format = UncompressedImage::Format::RGB;
      bitmap_info = kCGBitmapByteOrder32Big;
    } else {
      row_size = width * 4;
      format = UncompressedImage::Format::RGBA;
      bitmap_info = kCGImageAlphaPremultipliedLast |
                    kCGBitmapByteOrder32Big;
    }
  }
  
  std::unique_ptr<uint8_t[]> uncompressed(new uint8_t[height * row_size]);
  
  CGContextRef bitmap = CGBitmapContextCreate(uncompressed.get(), width, height,
                                              8, row_size, bitmap_colorspace,
                                              bitmap_info);
  if (nullptr == bitmap) {
    return UncompressedImage();
  }
  CGContextDrawImage(bitmap, CGRectMake(0, 0, width, height), image);
  
  CFRelease(bitmap);
  
  return UncompressedImage(format, row_size, width, height,
                           std::move(uncompressed));
}

UncompressedImage
LoadJPEGFile(Path path)
{
  CGDataProviderRef data_provider = CGDataProviderCreateWithFilename(path.c_str());
  if (nullptr == data_provider)
    return UncompressedImage();
  
  CGImageRef image =  CGImageCreateWithJPEGDataProvider(
      data_provider, nullptr, false, kCGRenderingIntentDefault);
  
  UncompressedImage result = CGImageToUncompressedImage(image);
  
  if (nullptr != image)
    CFRelease(image);
  CFRelease(data_provider);
  
  return result;
}

UncompressedImage
LoadPNG(const void *data, size_t size)
{
  CGDataProviderRef data_provider = CGDataProviderCreateWithData(
      nullptr, data, size, nullptr);
  if (nullptr == data_provider)
    return UncompressedImage();
  
  CGImageRef image = CGImageCreateWithPNGDataProvider(
      data_provider, nullptr, false, kCGRenderingIntentDefault);
  
  UncompressedImage result = CGImageToUncompressedImage(image);
  
  if (nullptr != image)
    CFRelease(image);
  CFRelease(data_provider);
  
  return result;
}

UncompressedImage
LoadPNG(Path path)
{
  CGDataProviderRef data_provider = CGDataProviderCreateWithFilename(path.c_str());
  if (nullptr == data_provider)
    return UncompressedImage();

  CGImageRef image =  CGImageCreateWithPNGDataProvider(
      data_provider, nullptr, false, kCGRenderingIntentDefault);

  UncompressedImage result = CGImageToUncompressedImage(image);

  if (nullptr != image)
    CFRelease(image);
  CFRelease(data_provider);

  return result;
}
