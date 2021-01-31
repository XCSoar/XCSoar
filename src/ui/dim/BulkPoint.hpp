/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_BULK_POINT_HPP
#define XCSOAR_SCREEN_BULK_POINT_HPP

// IWYU pragma: begin_exports

#ifdef ENABLE_OPENGL
#include "opengl/BulkPoint.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "memory/BulkPoint.hpp"
#elif defined(USE_GDI)
#include "gdi/BulkPoint.hpp"
#else
#error No Point implementation
#endif

// IWYU pragma: end_exports

#endif
