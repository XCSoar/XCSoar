// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/TrivialArray.hxx"

#include <cstdint>

struct IGCExtension {
  uint16_t start, finish;

  char code[4];
};

struct IGCExtensions : public TrivialArray<IGCExtension, 16> {
};
