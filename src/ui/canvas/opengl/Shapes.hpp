// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class GLArrayBuffer;

namespace OpenGL {

static constexpr unsigned CIRCLE_SIZE = 32;
static constexpr unsigned SMALL_CIRCLE_SIZE = 8;

extern GLArrayBuffer *circle_buffer, *small_circle_buffer;

void InitShapes() noexcept;
void DeinitShapes() noexcept;

} // namespace OpenGL
