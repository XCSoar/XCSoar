// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

class Path;

/**
 * Copy a file.
 *
 * Throws on error.
 */
void
CopyFile(Path src, Path dest);

/**
 * Attempt to move a file, but if that does not work, copy and delete
 * the source file.
 *
 * Throws on error.
 */
void
MoveOrCopyFile(Path src, Path dest);
