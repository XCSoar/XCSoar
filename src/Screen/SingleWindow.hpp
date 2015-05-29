/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_SCREEN_SINGLE_WINDOW_HXX
#define XCSOAR_SCREEN_SINGLE_WINDOW_HXX

#include "Screen/TopWindow.hpp"

#include <list>

#include <assert.h>

struct Event;
class WndForm;

/**
 * The single top-level window of an application.  When it is closed,
 * the process quits.
 */
class SingleWindow : public TopWindow {
#ifdef USE_GDI
  static constexpr const TCHAR *class_name = _T("XCSoarMain");
#endif

  std::list<WndForm *> dialogs;

public:
#ifdef USE_GDI
  static bool Find(const TCHAR *text) {
    return TopWindow::find(class_name, text);
  }

  /**
   * Register the WIN32 window class.
   */
  static bool RegisterClass(HINSTANCE hInstance);
#endif

  void Create(const TCHAR *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle()) {
#ifdef USE_GDI
    TopWindow::Create(class_name, text, size, style);
#else
    TopWindow::Create(text, size, style);
#endif
  }

  void AddDialog(WndForm *dialog);
  void RemoveDialog(WndForm *dialog);

  /**
   * Forcefully cancel the top-most dialog.
   */
  void CancelDialog();

  gcc_pure
  bool HasDialog() const {
    return !dialogs.empty();
  }

  /**
   * Check whether the specified dialog is the top-most one.
   */
  gcc_pure
  bool IsTopDialog(const WndForm &dialog) const {
    assert(HasDialog());

    return &dialog == dialogs.front();
  }

  WndForm &GetTopDialog() {
    assert(HasDialog());

    return *dialogs.front();
  }

#ifndef USE_GDI
protected:
  gcc_pure
  bool FilterMouseEvent(RasterPoint pt, Window *allowed) const;
#endif

public:
  /**
   * Check if the specified event should be allowed.  An event may be
   * rejected when a modal dialog is active, and the event should go
   * to a window outside of the dialog.
   */
  gcc_pure
  bool FilterEvent(const Event &event, Window *allowed) const;

protected:
  bool OnClose() override;
  void OnDestroy() override;
  void OnResize(PixelSize new_size) override;
};

#endif
