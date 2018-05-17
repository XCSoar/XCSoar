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

#ifndef XCSOAR_DEBUG_REPLAY_VECTOR_HPP
#define XCSOAR_DEBUG_REPLAY_VECTOR_HPP

#include "DebugReplay.hpp"
#include "IGCFixEnhanced.hpp"
#include <assert.h>


class DebugReplayVector : public DebugReplay {
  const std::vector<IGCFixEnhanced> fixes;
  unsigned long position;

private:
  DebugReplayVector(const std::vector<IGCFixEnhanced> &_fixes)
    : fixes(_fixes), position(0) {
  }

  ~DebugReplayVector() {
  }

public:
  virtual bool Next();

  long Size() const {
    return fixes.size();
  }

  long Tell() const {
    return position;
  }

  int Level() const {
    assert(position > 0);
    return fixes[position - 1].level;
  }

  static DebugReplay* Create(const std::vector<IGCFixEnhanced> &fixes) {
    return new DebugReplayVector(fixes);
  }

protected:
  void CopyFromFix(const IGCFixEnhanced &fix);
  void Compute(const int elevation);
};

#endif /* XCSOAR_DEBUG_REPLAY_VECTOR_HPP */
