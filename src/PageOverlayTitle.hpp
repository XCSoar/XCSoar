// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

class RaspStore;

template<typename T>
class BasicStringBuilder;

void
AppendOverlayTitle(BasicStringBuilder<char> &builder,
                   const PageLayout &layout,
                   const RaspStore *rasp);
