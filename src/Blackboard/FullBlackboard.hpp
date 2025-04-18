// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BaseBlackboard.hpp"
#include "SettingsBlackboard.hpp"

/**
 * A blackboard which contains all existing blackboards.  This is the
 * base class for InterfaceBlackboard, and may be used to pass
 * everything we have in one pointer.
 */
class FullBlackboard : public BaseBlackboard, public SettingsBlackboard {
};
