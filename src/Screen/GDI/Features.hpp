/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_GDI_FEATURES_HPP
#define XCSOAR_SCREEN_GDI_FEATURES_HPP

/**
 * This macro is defined when the Canvas implements clipping against
 * its siblings and children.
 */
#define HAVE_CLIPPING

#define HAVE_HATCHED_BRUSH

#ifdef _WIN32_WCE /* embedded Windows? */

/* AlphaBlend() is implemented since WM5, but we need to load it
   dynamically from coredll.dll */
#if _WIN32_WCE >= 0x500
#define HAVE_ALPHA_BLEND
#define HAVE_DYNAMIC_ALPHA_BLEND
#endif

#else /* !_WIN32_WCE */

/* AlphaBlend() is implemented since Windows 2000 */
#if _WIN32_WINDOWS >= 0x500
#define HAVE_ALPHA_BLEND
#define HAVE_BUILTIN_ALPHA_BLEND
#endif

#endif /* !_WIN32_WCE */

#endif
