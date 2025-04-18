// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DebugReplay.hpp"
#include "IGCFixEnhanced.hpp"
#include <cassert>


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
