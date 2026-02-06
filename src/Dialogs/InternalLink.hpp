// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;

/**
 * Helper to show a config panel in a dialog.
 * Opens a full-screen dialog with the given title, displays the widget
 * created by create_panel(), and saves the profile if changes were made.
 *
 * @param title The dialog title
 * @param create_panel Function that creates the config panel widget
 */
void
ShowConfigPanel(const char *title,
                std::unique_ptr<Widget> (*create_panel)());

/**
 * Handle internal xcsoar:// URIs.
 *
 * Supported URIs:
 * - xcsoar://config/devices - Opens device configuration
 * - xcsoar://config/planes - Opens plane configuration
 * - xcsoar://config/site-files - Opens site files configuration
 * - xcsoar://config/logger - Opens logger configuration
 * - xcsoar://config/time - Opens time configuration
 * - xcsoar://config/infoboxes - Opens InfoBox sets configuration
 * - xcsoar://config/pages - Opens pages configuration
 * - xcsoar://config/weglide - Opens WeGlide configuration
 * - xcsoar://dialog/checklist - Opens checklist dialog
 * - xcsoar://dialog/flight - Opens flight settings (basic settings)
 * - xcsoar://dialog/wind - Opens wind settings
 * - xcsoar://dialog/task - Opens task manager
 * - xcsoar://dialog/analysis - Opens analysis dialog
 * - xcsoar://dialog/status - Opens status dialog
 * - xcsoar://dialog/credits - Opens credits dialog
 *
 * @param url The xcsoar:// URL to handle
 * @return true if the URL was handled, false if unknown
 */
bool
HandleInternalLink(const char *url);
