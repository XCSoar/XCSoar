/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#if !defined(ANDROID) && !defined(WIN32)

#include "PCMPlayer.hpp"

#include "PCMDataSource.hpp"
#include "AudioAlgorithms.hpp"

#include <assert.h>

#include <algorithm>

size_t
PCMPlayer::FillPCMBuffer(int16_t *buffer, size_t n)
{
  assert(n > 0);
  assert(nullptr != source);
  const size_t n_read = source->GetData(buffer, n);
  if (n_read > 0)
    UpmixMonoPCM(buffer, n_read, channels);
  if (n_read < n)
    std::fill(buffer + n_read * channels, buffer + n * channels, 0);
  return n_read;
}

#endif
