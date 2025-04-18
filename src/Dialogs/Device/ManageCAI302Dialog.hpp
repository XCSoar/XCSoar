// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct DialogLook;
namespace UI { class SingleWindow; }
class Device;

void
ManageCAI302Dialog(UI::SingleWindow &parent, const DialogLook &look,
                   Device &device);
