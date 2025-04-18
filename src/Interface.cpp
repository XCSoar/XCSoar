// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Interface.hpp"
#include "UIState.hpp"

UIState CommonInterface::Private::ui_state;

bool CommonInterface::Private::movement_detected = false;

InterfaceBlackboard CommonInterface::Private::blackboard;

MainWindow *CommonInterface::main_window;
