// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;

/**
 * Creates a widget for configuring NOTAM (Notice to Airmen) settings.
 * @return A unique pointer to the configuration panel widget.
 */
std::unique_ptr<Widget>
CreateNOTAMConfigPanel();
