// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/QRScanner.hpp"
#include "Task/QRDecoder.hpp"
#include "Main.hpp"
#include "NativeView.hpp"
#include "java/Global.hxx"
#include "util/Compiler.h"
#include "org_xcsoar_QRScannerActivity.h"

#include <cstddef>
#include <cstdint>

bool
HaveQRScanner() noexcept
{
  return native_view != nullptr;
}

void
ScanTaskQRCode() noexcept
{
  if (native_view != nullptr)
    native_view->ScanQRCode(Java::GetEnv());
}

/**
 * Decode one camera frame.  The plane is a direct ByteBuffer owned by
 * the Camera2 Image, so this borrows the camera's memory instead of
 * copying a megabyte per frame, and zxing-cpp is handed the strides
 * rather than a repacked buffer.
 */
gcc_visibility_default
JNIEXPORT jstring JNICALL
Java_org_xcsoar_QRScannerActivity_decodeQRCode(JNIEnv *env,
                                               [[maybe_unused]] jclass cls,
                                               jobject plane,
                                               jint width, jint height,
                                               jint row_stride,
                                               jint pixel_stride)
{
  if (plane == nullptr || width <= 0 || height <= 0 ||
      row_stride <= 0 || pixel_stride <= 0)
    return nullptr;

  const auto *data = (const uint8_t *)
    env->GetDirectBufferAddress(plane);
  if (data == nullptr)
    /* not a direct buffer; the caller checks for this, so getting here
       means the camera handed us something unexpected */
    return nullptr;

  const jlong capacity = env->GetDirectBufferCapacity(plane);
  if (capacity <= 0)
    return nullptr;

  /* DecodeQRCode() takes the capacity and lets zxing-cpp reject a
     buffer too small for the geometry */
  const auto text = DecodeQRCode(data, (std::size_t)capacity,
                                 width, height,
                                 row_stride, pixel_stride);
  if (text.empty())
    return nullptr;

  return env->NewStringUTF(text.c_str());
}
