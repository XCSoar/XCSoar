// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct DialogLook;
namespace UI { class SingleWindow; }
class DeviceDescriptor;

void
ManageI2CPitotDialog(UI::SingleWindow &parent, const DialogLook &look,
                     DeviceDescriptor &descriptor) noexcept;
