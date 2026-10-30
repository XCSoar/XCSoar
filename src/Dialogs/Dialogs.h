// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;
namespace UI { class SingleWindow; }

void dlgBasicSettingsShowModal();

void dlgChecklistShowModal();

/**
 * Called from #SettingsLeave when the checklist file path in Site Files
 * was saved so the next checklist open uses the new profile entry.
 */
void dlgChecklistNotifySiteFileChanged() noexcept;
void dlgConfigurationShowModal();

/** Open Configuration on an existing menu page. */
void dlgConfigurationShowPage(std::unique_ptr<Widget> (*page)());
void dlgConfigFontsShowModal();

void ShowWindSettingsDialog();

void dlgStatusShowModal(int page);

void dlgCreditsShowModal(UI::SingleWindow &parent);

void
dlgQuickMenuShowModal(UI::SingleWindow &parent) noexcept;
