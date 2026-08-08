// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Include this *after* Windows headers (`windows.h`, `fileapi.h`,
 * `shlobj.h`, …).
 *
 * XCSoar #Path is UTF-8. Without `UNICODE`, Win32 macros such as
 * `CreateFile` expand to `CreateFileA` (active code page). That is
 * incompatible with UTF-8 paths and caused #2824.
 *
 * Undefining the A/W selector macros forces call sites to use the
 * explicit `*W` APIs with `UTF8ToWide()`, or `OpenPathFile()`.
 */

#ifdef _WIN32

#ifdef CreateFile
#undef CreateFile
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef MoveFile
#undef MoveFile
#endif
#ifdef MoveFileEx
#undef MoveFileEx
#endif
#ifdef CopyFile
#undef CopyFile
#endif
#ifdef GetFileAttributes
#undef GetFileAttributes
#endif
#ifdef GetFileAttributesEx
#undef GetFileAttributesEx
#endif
#ifdef FindFirstFile
#undef FindFirstFile
#endif
#ifdef FindNextFile
#undef FindNextFile
#endif
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef RemoveDirectory
#undef RemoveDirectory
#endif
#ifdef GetModuleFileName
#undef GetModuleFileName
#endif
#ifdef SHGetSpecialFolderPath
#undef SHGetSpecialFolderPath
#endif
#ifdef GetTempPath
#undef GetTempPath
#endif
#ifdef GetTempFileName
#undef GetTempFileName
#endif

#endif /* _WIN32 */
