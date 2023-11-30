// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef IS_OPENVARIO

#include <memory>

class Widget;

std::unique_ptr<Widget>
CreateOpenVarioConfigPanel();

#endif