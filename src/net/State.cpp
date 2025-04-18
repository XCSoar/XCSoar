// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "State.hpp"

#ifdef ANDROID
#define NO_SCREEN
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"

NetState
GetNetState()
{
  return native_view != nullptr
    ? NetState(native_view->GetNetState(Java::GetEnv()))
    : NetState::UNKNOWN;
}

#endif
