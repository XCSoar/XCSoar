// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/InfoBoxSettings.hpp"

struct DialogLook;
struct InfoBoxLook;
namespace UI { class SingleWindow; }

/**
 * @return true when the #InfoBoxPanelConfig object has been modified
 */
bool
dlgConfigInfoboxesShowModal(UI::SingleWindow &parent,
                            const DialogLook &dialog_look,
                            const InfoBoxLook &look,
                            InfoBoxSettings::Geometry geometry,
                            InfoBoxSettings::Panel &data,
                            bool allow_name_change);
