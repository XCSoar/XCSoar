// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace UI { class SingleWindow; }

void dlgBasicSettingsShowModal();

void dlgChecklistShowModal();
void dlgConfigurationShowModal();
void dlgConfigFontsShowModal();

void ShowWindSettingsDialog();

void dlgStatusShowModal(int page);

void dlgCreditsShowModal(UI::SingleWindow &parent);

void
dlgQuickMenuShowModal(UI::SingleWindow &parent) noexcept;
