/*! \file
 * \brief GDI compatibility functions
 *
 * This header provides GDI functions which might not be available on
 * all WIN32 platforms.
 */

#ifndef XCSOAR_COMPAT_GDI_H
#define XCSOAR_COMPAT_GDI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef HAVE_MSVCRT

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl	TransparentImage(HDC, ...);

#ifdef __cplusplus
}
#endif

#endif

#endif
