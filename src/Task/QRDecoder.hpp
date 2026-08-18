// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

/**
 * Decode a QR code from a greyscale image.
 *
 * This is deliberately free of any platform types so that every port
 * can feed it whatever its camera produces: Android hands over the Y
 * plane of a Camera2 frame, and an iOS or desktop caller could pass
 * any 8 bit luminance buffer.
 *
 * @param luminance one byte per pixel
 * @param size the buffer's size in bytes, so the decoder can reject a
 * buffer too small for the geometry rather than reading past its end
 * @param row_stride bytes between the starts of two rows; camera
 * planes often pad these, so it is not necessarily equal to width
 * @param pixel_stride bytes between two horizontally adjacent pixels,
 * which semi-planar formats may set to more than one
 *
 * @return the decoded text, or an empty string if the image contains
 * no readable QR code
 */
[[gnu::pure]]
std::string
DecodeQRCode(const uint8_t *luminance, std::size_t size,
             unsigned width, unsigned height,
             unsigned row_stride, unsigned pixel_stride) noexcept;
