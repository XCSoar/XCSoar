/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_IMGDECMP_HPP
#define XCSOAR_IMGDECMP_HPP

#include "DynamicLibrary.hpp"

#include "imgdecmp.h"

class ImgDeCmpDLL : public DynamicLibrary {
protected:
  typedef HRESULT (*DecompressImageIndirect_t)(DecompressImageInfo *pParams);

  DecompressImageIndirect_t DecompressImageIndirect_p;

public:
  ImgDeCmpDLL()
    :DynamicLibrary(_T("imgdecmp")),
     DecompressImageIndirect_p((DecompressImageIndirect_t)
                               Lookup(_T("DecompressImageIndirect"))) {}

  HRESULT DecompressImageIndirect(DecompressImageInfo *pParams) const {
    return DecompressImageIndirect_p != NULL
      ? DecompressImageIndirect_p(pParams)
      : E_NOTIMPL;
  }
};

#endif
