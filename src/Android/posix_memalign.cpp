/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2019 The XCSoar Project
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

#include <stdlib.h>


extern "C" {

  /** \brief Mimic posix_memalign as a thin wrapper around memalign.
   * 
   * posix_memalign is not present in the API version 16 bionic library and before, but is required by the builtin "new" operator of clang in NDK19 or later.
   */
  int posix_memalign(void **memptr, size_t alignment, size_t size) {

      *memptr = memalign (alignment,size);

      if (!*memptr) {
          // Assume that CLang knows what it does in regards to aligmnent.
          // The only reasonable error source can only by lack of memory.
          return ENOMEM;
      } else {
          return 0;
      }
  } // posix_memalign

} // extern "C" {

