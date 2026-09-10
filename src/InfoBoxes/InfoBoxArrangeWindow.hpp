// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxSettings.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Renderer/TextRenderer.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/Timer.hpp"
#include "ui/window/PaintWindow.hpp"
#include "util/StaticArray.hxx"

#include <chrono>
#include <functional>
#include <optional>

struct DialogLook;
struct InfoBoxLook;

namespace InfoBoxLayout { struct Layout; }

/**
 * Shows the InfoBoxes of one panel as cards which the user can
 * exchange by dragging them, with the cursor keys or with a remote
 * stick.  Tapping a card describes its InfoBox, and a long press
 * chooses a different one.
 *
 * The window paints everything it shows, which is why the card which
 * follows the finger can never end up behind another window.
 */
class InfoBoxArrangeWindow : public PaintWindow {
public:
  /** Where is this window used? */
  enum class Style {
    /**
     * On top of the map: the background is translucent, the name of
     * the panel is painted above the buttons, and the cursor keys stop
     * at the edge of the layout.
     */
    MAP,

    /**
     * Inside a dialog: the background is opaque, the panel is named by
     * the dialog, and the cursor leaves the window at the edge of the
     * layout so that the other controls remain reachable.
     */
    DIALOG,
  };

  using Callback = std::function<void()>;

  /**
   * Does a card follow the finger right now?  While it does, the
   * window behind this one has to clear itself, because the card is
   * not clipped to this window.
   */
  [[gnu::pure]]
  static bool IsCardFloating() noexcept {
    return card_floating;
  }

  static constexpr unsigned MAX_BUTTONS = 5;

private:
  /** How is the InfoBox the user is working with drawn? */
  enum class CardState {
    /** just one of the InfoBoxes */
    NORMAL,

    /**
     * Selected with the cursor keys.  Like a focused button, the card
     * only gets a ring, because it is not taken yet.
     */
    FOCUSED,

    /** tapped, taken with Enter, or following the finger */
    ACTIVE,
  };

  /** How was the InfoBox which is being worked with selected? */
  enum class Selection {
    /** by tapping it, or by grabbing it with the finger */
    TOUCH,

    /** with the cursor keys */
    FOCUSED,

    /** taken with Enter, ready to be moved with the cursor keys */
    MOVING,
  };

  /** One of the buttons below the cards. */
  struct ButtonItem {
    const char *caption;
    Callback callback;

    /** 0 is the bottom row, 1 the one above it */
    unsigned row;

    bool enabled;
  };

  /** The InfoBox which is being dragged. */
  struct DragState {
    /** the slot the dragged InfoBox occupies right now */
    unsigned slot;

    /** the slot rectangle the drag started from */
    PixelRect start_rect;

    /** the press position in parent coordinates when the drag started */
    PixelPoint origin;

    /** the current pointer position in parent coordinates */
    PixelPoint pointer;

    /** does the InfoBox follow the finger already? */
    bool following;
  };

  /**
   * An InfoBox which was displaced by the dragged one slides into its
   * new slot instead of jumping there.
   */
  struct Shuffle {
    /** where the card starts, relative to its slot */
    PixelPoint offset{0, 0};

    std::chrono::steady_clock::time_point start{};
  };

  /**
   * Sort key for the Tab order: it follows the axis the InfoBoxes are
   * stacked on, so row by row when they stand in rows and column by
   * column when they stand in columns, and the buttons take their
   * place in that order by where they sit on the screen.
   */
  struct TabKey {
    /** the row or column on the screen */
    int group;

    /** the button row inside that group, top to bottom */
    int button_row;

    /** the place inside that button row */
    int position;

    constexpr bool operator<(const TabKey &other) const noexcept {
      if (group != other.group)
        return group < other.group;

      if (button_row != other.button_row)
        return button_row < other.button_row;

      return position < other.position;
    }
  };

  const InfoBoxLook &look;
  const DialogLook &dialog_look;

  const Style style;

  InfoBoxSettings::Panel *panel = nullptr;
  const InfoBoxLayout::Layout *layout = nullptr;

  /** are the InfoBoxes of #layout arranged in columns? */
  bool columns = false;

  /** the area between the cards, for the description and the buttons */
  PixelRect content;

  StaticArray<ButtonItem, MAX_BUTTONS> buttons;

  /** draws all buttons, one after the other */
  TextButtonRenderer button_renderer;

  /** another paragraph for the help text, or nullptr */
  const char *extra_help = nullptr;

  /** @see IsCardFloating() */
  static bool card_floating;

  std::optional<DragState> drag;

  /** the InfoBox configuration as it was when the drag started */
  InfoBoxSettings::Panel drag_snapshot;

  /** the button the press started on, or -1 */
  int held_button = -1;

  /** the button the cursor keys have selected, or -1 */
  int focused_button = -1;

  /**
   * Where the cursor sits across the axis it moves on, as a fraction
   * of the row or column it is in.  Crossing the button block keeps
   * this fraction, so that the cursor returns to the place it came
   * from.
   */
  double cross_fraction = 0.5;

  /** is the finger still on #held_button? */
  bool button_down = false;

  Shuffle shuffle[InfoBoxSettings::Panel::MAX_CONTENTS];

  /**
   * The number each slot shows.  While dragging, the numbers travel
   * with the InfoBoxes instead of staying on the slots, so that they
   * only change once the finger is lifted.
   */
  unsigned card_number[InfoBoxSettings::Panel::MAX_CONTENTS];

  /** the InfoBox whose description is shown, or -1 */
  int described_slot = -1;

  /** how #described_slot was selected */
  Selection selection = Selection::TOUCH;

  /** the slot #Selection::MOVING started from */
  unsigned grab_slot;

  TextRenderer name_renderer, description_renderer;

  /** opens the InfoBox picker when an InfoBox is held down */
  UI::Timer picker_timer{[this]{ OnPickerTimer(); }};

  UI::PeriodicTimer shuffle_timer{[this]{ OnShuffleTimer(); }};

public:
  InfoBoxArrangeWindow(const InfoBoxLook &_look,
                       const DialogLook &_dialog_look,
                       Style _style) noexcept;

  /**
   * Add a button below the cards, to the right of the previous one of
   * its row.  Row 0 is the bottom row, row 1 the one above it; the
   * rows are filled from the bottom up, so none of them may stay
   * empty.  Only before Create().
   *
   * @return the index for SetButtonEnabled()
   */
  unsigned AddButton(unsigned row, const char *caption,
                     Callback callback) noexcept;

  /** Add a button to the bottom row. */
  unsigned AddButton(const char *caption, Callback callback) noexcept {
    return AddButton(0, caption, std::move(callback));
  }

  /** A disabled button is drawn greyed out and cannot be selected. */
  void SetButtonEnabled(unsigned i, bool enabled) noexcept;

  /** Another paragraph for the help text, shown after the general one. */
  void SetExtraHelp(const char *text) noexcept {
    extra_help = text;
  }

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept;

  /** Which panel does the user arrange? */
  void SetPanel(InfoBoxSettings::Panel &_panel) noexcept;

  /**
   * @param _layout where the cards are
   * @param _content the area between them, for the description and the
   * buttons; usually InfoBoxLayout::Layout::remaining, less what the
   * caller puts there itself
   */
  void SetLayout(const InfoBoxLayout::Layout &_layout,
                 PixelRect _content) noexcept;

  /** Select an InfoBox with no drag, for the cursor keys. */
  void FocusSlot(unsigned slot) noexcept;

  /** Begin a drag which was started on another window. */
  void BeginDrag(unsigned slot, PixelPoint pointer, bool follow) noexcept;

  /** Abort the drag and undo the exchanges it has made. */
  void CancelDrag() noexcept;

  /** Finish a drag which is still in progress. */
  void Drop() noexcept;

  /** Explain how an InfoBox is moved and how it is replaced. */
  void ShowHelp() noexcept;

protected:
  /**
   * The user has done something; the map overlay restarts its timeout.
   */
  virtual void OnArrangeActivity() noexcept {}

  /**
   * A modal dialog is about to cover this window; the map overlay
   * stops its timeout until the next OnArrangeActivity().
   */
  virtual void OnArrangeSuspend() noexcept {}

  /** The panel has been modified. */
  virtual void OnArrangeModified() noexcept {}

  /**
   * The Escape key was pressed.
   *
   * @return true if this window has handled it
   */
  virtual bool OnArrangeCancel() noexcept {
    return false;
  }

private:
  [[gnu::pure]]
  PixelPoint SlotCenter(unsigned slot) const noexcept;

  /** The coordinate along the axis the InfoBoxes are stacked on. */
  [[gnu::pure]]
  int Along(PixelPoint p) const noexcept;

  /** The coordinate across that axis. */
  [[gnu::pure]]
  int Across(PixelPoint p) const noexcept;

  /** Which slot covers the given position in parent coordinates? */
  [[gnu::pure]]
  int FindSlot(PixelPoint p) const noexcept;

  /** Where does the card which follows the finger sit? */
  [[gnu::pure]]
  PixelRect GetFloatingRect() const noexcept;

  /** The gap between the buttons and around the button row. */
  [[gnu::pure]]
  static int GetButtonGap() noexcept;

  /** How many rows do the buttons occupy? */
  [[gnu::pure]]
  unsigned GetButtonRowCount() const noexcept;

  /** How many buttons are in @p row? */
  [[gnu::pure]]
  unsigned GetRowButtonCount(unsigned row) const noexcept;

  /** Which place does the button @p i take inside its row? */
  [[gnu::pure]]
  unsigned GetIndexInRow(unsigned i) const noexcept;

  /**
   * How wide is a button of @p row?  The buttons of a row share the
   * width which is available to them, so that every row is as wide as
   * the description above it.
   */
  [[gnu::pure]]
  int GetButtonWidth(unsigned row) const noexcept;

  /** How high is a button?  Enough rows make them flatter. */
  [[gnu::pure]]
  int GetButtonHeight() const noexcept;

  /** One of the rows which hold the buttons. */
  [[gnu::pure]]
  PixelRect GetButtonRowRect(unsigned row) const noexcept;

  /** All button rows together. */
  [[gnu::pure]]
  PixelRect GetButtonBlockRect() const noexcept;

  /** One of the buttons in #GetButtonRowRect(). */
  [[gnu::pure]]
  PixelRect GetButtonRect(int i) const noexcept;

  /**
   * Where does the InfoBox in @p slot sit across the axis, from 0
   * (the first place of its row or column) to 1 (the last one)?  Rows
   * of different length are compared by this fraction, so that the
   * second of five InfoBoxes lands on the first of three buttons and
   * not on the second.
   */
  [[gnu::pure]]
  double GetSlotFraction(unsigned slot) const noexcept;

  /** Where does the button @p i sit inside its row? */
  [[gnu::pure]]
  double GetButtonFraction(unsigned i) const noexcept;

  /** Where does the button @p i sit across the axis? */
  [[gnu::pure]]
  double GetButtonCrossFraction(unsigned i) const noexcept;

  /**
   * The next InfoBox row or column beyond
   * @p along in the direction @p direction.
   *
   * @return std::nullopt if there is none
   */
  [[gnu::pure]]
  std::optional<int> FindSlotGroup(int along, int direction) const noexcept;

  /** Which InfoBox of @p group sits closest to @p fraction? */
  [[gnu::pure]]
  int FindSlotAt(int group, double fraction) const noexcept;

  /**
   * Which button of @p row sits closest to @p fraction?
   *
   * @return the button, or -1 if the row has none to select
   */
  [[gnu::pure]]
  int FindButtonAt(unsigned row, double fraction) const noexcept;

  /**
   * The next button the user may select inside the row of @p i.
   *
   * @param dx -1 for the left, 1 for the right neighbour
   * @return the button, or -1 if the row ends there
   */
  [[gnu::pure]]
  int FindNextInRow(int i, int dx) const noexcept;

  /** Which button covers the given position in parent coordinates? */
  [[gnu::pure]]
  int FindButton(PixelPoint p) const noexcept;

  /** Where the name of the panel is drawn, above the buttons. */
  [[gnu::pure]]
  PixelRect GetPanelNameRect() const noexcept;

  [[gnu::pure]]
  PixelRect ToLocal(PixelRect rc) const noexcept;

  [[gnu::pure]]
  ButtonState GetButtonState(int i) const noexcept;

  [[gnu::pure]]
  CardState GetCardState(unsigned slot) const noexcept;

  void DrawCard(Canvas &canvas, const PixelRect &rc, unsigned slot,
                unsigned number, CardState state) noexcept;

  void PaintCards(Canvas &canvas) noexcept;
  void PaintButtons(Canvas &canvas) noexcept;
  void PaintPanelName(Canvas &canvas) noexcept;
  void PaintDescription(Canvas &canvas) noexcept;

  /** Exchange the InfoBoxes in two slots. */
  void Exchange(unsigned a, unsigned b) noexcept;

  void ResetCardNumbers() noexcept;

  /** Let the InfoBox in @p slot slide in from @p from. */
  void StartShuffle(unsigned slot, const PixelRect &from) noexcept;

  /** How far is the InfoBox in @p slot still away from its slot? */
  [[gnu::pure]]
  PixelPoint GetShuffleOffset(unsigned slot) const noexcept;

  void OnShuffleTimer() noexcept;

  /** Open the picker for the InfoBox which is being held down. */
  void OnPickerTimer() noexcept;

  /** Let the user choose a different InfoBox for @p slot. */
  void ShowPicker(unsigned slot) noexcept;

  /**
   * Which slot lies next to @p origin in the direction (@p dx, @p dy)?
   *
   * @return the slot, or -1 if there is none in that direction
   */
  [[gnu::pure]]
  int FindNeighbour(PixelPoint origin, int dx, int dy) const noexcept;

  /**
   * Does the button row come before @p slot when moving away from
   * @p origin?
   */
  [[gnu::pure]]
  bool IsButtonRowCloser(PixelPoint origin, int slot,
                         bool forward) const noexcept;

  /** Is there any button the user may select? */
  [[gnu::pure]]
  bool HasEnabledButton() const noexcept;

  /**
   * Move the cursor from the InfoBoxes into the buttons.
   *
   * @return false if there is no button to select
   */
  bool FocusButtonFrom(int dx, int dy) noexcept;

  /** Remember where the cursor sits across the axis. */
  void RememberCross() noexcept;

  /** Move the cursor away from the button row. */
  bool MoveFromButton(int dx, int dy, bool along) noexcept;

  [[gnu::pure]]
  TabKey GetTabKey(unsigned i) const noexcept;

  void FocusItem(unsigned i) noexcept;

  /**
   * Move the focus one step along the Tab order, wrapping around at
   * the ends.
   */
  bool MoveTab(bool forward) noexcept;

  /** Would MoveSelection() move the cursor? */
  [[gnu::pure]]
  bool CanMoveSelection(int dx, int dy) const noexcept;

  /** Move the selection with the cursor keys or a remote stick. */
  bool MoveSelection(int dx, int dy) noexcept;

  /**
   * Enter takes the selected InfoBox, puts it down again, or opens
   * the picker if it was put down where it was taken.
   */
  bool Activate() noexcept;

  /** Run what the button in @p i does. */
  void OnButton(int i) noexcept;

  void HoldButton(int i, bool down) noexcept;
  void ReleaseButton() noexcept;

  /** Move the dragged card; @p p is in parent coordinates. */
  void Drag(PixelPoint p) noexcept;

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;

  /* virtual methods from class Window */
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
#ifdef HAVE_MULTI_TOUCH
  bool OnMultiTouchDown() noexcept override;
#endif
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnCancelMode() noexcept override;
};
