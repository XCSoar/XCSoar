// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

typedef struct FT_FaceRec_ *FT_Face;

namespace FreeType {

#ifdef KOBO
/**
 * Are we using monochrome font rendering mode on the Kobo?  This
 * can be disabled for some situations; see
 * TopCanvas::SetEnableDither().
 */
extern bool mono;
#endif

/**
 * Throws on error.
 */
void
Initialise();

void
Deinitialise();

/**
 * Throws on error.
 */
FT_Face
Load(const char *path);

} // namespace FreeType
