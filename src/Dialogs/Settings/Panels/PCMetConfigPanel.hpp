// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;

/**
 * The page in the configuration dialogue: the pc_met account plus the
 * DWD forecast switch.
 */
std::unique_ptr<Widget>
CreatePCMetConfigPanel();

/**
 * The same account rows without the forecast switch, for the weather
 * dialogue's credential gate.  That gate stops showing this panel once
 * an account is entered, which is no place for a setting the pilot has
 * to be able to find again.
 */
std::unique_ptr<Widget>
CreatePCMetCredentialsPanel();
