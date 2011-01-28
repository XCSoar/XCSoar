/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef XCSOAR_FORM_TABBAR_HPP
#define XCSOAR_FORM_TABBAR_HPP


#include "Util/StaticArray.hpp"
#include "Form/Tabbed.hpp"

class WndOwnerDrawFrame;
class ContainerWindow;
class TabDisplay;

/** TabBarControl displays tabs that show/hide the windows
 * associated with each tab.  For example a "Panel" control.
 * It can also display buttons with no associated Window.
 * Supports pre and post- tab click callbacks
 * Each tab must be added via code (not via XML)
 * ToDo: support lazy loading
 */
class TabBarControl : public TabbedControl {
public:
  typedef bool (*PreHideNotifyCallback_t)(void);
  typedef bool (*PreShowNotifyCallback_t)(void);
  typedef void (*PostShowNotifyCallback_t)(void);

public:
/**
 *
 * @param parent
 * @param x, y Location of the tab bar (unused)
 * @param width, height.  Size of the tab bar
 * @param style
 * @return
 */
  TabBarControl(ContainerWindow &parent,
                int x, int y, unsigned width, unsigned height,
                const WindowStyle style = WindowStyle());

private:
#define TabHeightInitUnscaled (unsigned)40
#define TabLineHeightInitUnscaled (unsigned)5

public:
/**
 * OneTabButton class holds display and callbacks data for a single tab
 */
  class OneTabButton {
  public:
    OneTabButton(const TCHAR* _Caption,
                bool _IsButtonOnly,
                PreHideNotifyCallback_t _PreHideFunction,
                PreShowNotifyCallback_t _PreShowFunction,
                PostShowNotifyCallback_t _PostShowFunction):
                  IsButtonOnly(_IsButtonOnly),
                  PreHideFunction(_PreHideFunction),
                  PreShowFunction(_PreShowFunction),
                  PostShowFunction(_PostShowFunction)
    {
      _tcscpy(Caption, _Caption);
    };

  public:
    TCHAR Caption[MAX_PATH];
    bool IsButtonOnly;
  /**
   * Called before the tab is hidden.
   * @returns  True if ok and tab may change.  False if click should be ignored
   */
    PreHideNotifyCallback_t PreHideFunction;

    /**
     * Called immediately after tab is clicked, before it is displayed.
     * @returns  True if ok and tab may change.  False if click should be ignored
     */
    PreShowNotifyCallback_t PreShowFunction;

    /**
     * Called immediately after tab is made active and shown
     */
    PostShowNotifyCallback_t PostShowFunction;
  };

public:
/** adds a tab to the TabBar
 * @param w. The window (e.g. created by LoadWindow()
 * @param Caption. Caption for the tab display
 * @param IsButtonOnly.  The tab button will resemble look/feel of a button
 * @param PreHideFunction client callback
 * @param PreShowFunction client callback
 * @param PostShowFunction client callback
 * @return index of added tab
 */
  unsigned AddClient(Window *w, const TCHAR* Caption,
        bool IsButtonOnly = false,
        PreHideNotifyCallback_t PreHideFunction = NULL,
        PreShowNotifyCallback_t PreShowFunction = NULL,
        PostShowNotifyCallback_t PostShowFunction = NULL);

public:
  void SetCurrentPage(unsigned i);
  void NextPage();
  void PreviousPage();
  unsigned GetTabCount() { return buttons.size(); }

/**
 * calculates the size and position of ith button
 * works in landscape or portrait mode
 * @param i index of button
 * @return Rectangle of button coordinates
 */
  const RECT GetButtonSize(unsigned i);
  const TCHAR* GetButtonCaption(unsigned i);
  bool GetButtonIsButtonOnly(unsigned i);
  unsigned GetBarHeight() { return TabBarHeight; }
  unsigned GetBarWidth() { return TabBarWidth; }


protected:
  TabDisplay * theTabDisplay;
  StaticArray<OneTabButton *, 32> buttons;
  const unsigned int TabBarHeight;
  const unsigned int TabBarWidth;
  const unsigned int TabLineHeight;

};

/**
 * TabDisplay class handles onPaint callback for TabBar UI
 * and handles Mouse and key events
 * TabDisplay uses a pointer to TabBarControl
 * to show/hide the appropriate pages in the Container Class
 */
class TabDisplay: PaintWindow
{
public:
/**
 *
 * @param parent
 * @param _theTabBar. An existing TabBar object
 * @param height the height to make the row of tabs the screen
 */
  TabDisplay(TabBarControl& _theTabBar, unsigned width, unsigned height);

public:
  void trigger_invalidate() { invalidate(); }

protected:
  TabBarControl& theTabBar;
  bool dragging; // tracks that mouse is down and captured
  int downindex; // index of tab where mouse down occurred

protected:
/**
 * paints the tab buttons
 * @param canvas
 */
  virtual void on_paint(Canvas &canvas);
  //ToDo: support function buttons

/**
 * track key presses to navigate without mouse
 * @param key_code
 * @return
 */
  virtual bool on_killfocus();
  virtual bool on_setfocus();
  virtual bool on_key_check(unsigned key_code) const;
  virtual bool on_key_down(unsigned key_code);

/**
 * track mouse clicks
 */
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  void drag_end();
};
#endif
