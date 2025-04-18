// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace UI { class SingleWindow; }
class GlueMapWindow;
struct DialogSettings;
struct Look;
struct DialogLook;
struct IconLook;
struct MapLook;
struct FormatSettings;

/**
 * This namespace provides helper functions to access generic global
 * UI objects.  Use them when you don't know where else to get them.
 * This is a last resort only, don't use it if you have a better way
 * to do it.
 *
 * This namespace exists to avoid direct access to #CommonInterface
 * and #MainWindow, because that would mean the code is not reusable
 * in other applications, while the functions in this namespace can
 * easily be replaced in another program.
 */
namespace UIGlobals {
  [[gnu::const]]
  UI::SingleWindow &GetMainWindow();

  [[gnu::const]]
  GlueMapWindow *GetMap();

  [[gnu::pure]]
  GlueMapWindow *GetMapIfActive();

  [[gnu::const]]
  const DialogSettings &GetDialogSettings();

  [[gnu::const]]
  const Look &GetLook();

  [[gnu::const]]
  const DialogLook &GetDialogLook();

  [[gnu::const]]
  const FormatSettings &GetFormatSettings();

  [[gnu::const]]
  const IconLook &GetIconLook();

  [[gnu::const]]
  const MapLook &GetMapLook();
};
