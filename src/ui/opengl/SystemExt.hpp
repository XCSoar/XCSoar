// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "System.hpp"

#ifndef ENABLE_SDL
#include <GLES2/gl2ext.h>
#endif

#if defined(GL_EXT_discard_framebuffer) && defined(__APPLE__) && TARGET_OS_IPHONE
/* this typedef is missing in the iOS SDK */
typedef void (GL_APIENTRYP PFNGLDISCARDFRAMEBUFFEREXTPROC) (GLenum target, GLsizei numAttachments, const GLenum *attachments);
#endif // TARGET_OS_IPHONE
