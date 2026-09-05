// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxSettings.hpp"
#include "Renderer/TextRenderer.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/Timer.hpp"
#include "ui/window/PaintWindow.hpp"

#include <chrono>
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
 * Buttons belong to the parent (#WidgetDialog or the map overlay
 * #ButtonPanel), not to this window, so a card cannot end up behind
 * them.
 */
class InfoBoxArrangeWindow : public PaintWindow {
public:
  /** Where is this window used? */
  enum class Style {
    /**
     * On top of the map: the background is translucent, the name of
     * the panel is painted above the description, and the cursor keys
     * leave the cards for the Help/Close buttons.
     */
    MAP,

    /**
     * Inside a dialog: the background is opaque, the panel is named
     * by the dialog, and the cursor leaves the window at the edge of
     * the layout so that the other controls remain reachable.
     */
    DIALOG,
  };

  /**
   * Does a card follow the finger right now?  While it does, the
   * window behind this one has to clear itself, because the card is
   * not clipped to this window.
   */
  [[gnu::pure]]
  static bool IsCardFloating() noexcept {
    return card_floating;
  }

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

  /** The InfoBox which is being dragged. */
  struct DragState {
    /** the slot the dragged InfoBox occupies right now */
    unsigned slot;

    /** the slot rectangle the drag started from */
    PixelRect start_rect;

    /** the press position in layout coordinates when the drag started */
    PixelPoint origin;

    /** the current pointer position in layout coordinates */
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

  const InfoBoxLook &look;
  const DialogLook &dialog_look;

  const Style style;

  InfoBoxSettings::Panel *panel = nullptr;
  const InfoBoxLayout::Layout *layout = nullptr;

  /** are the InfoBoxes of #layout arranged in columns? */
  bool columns = false;

  /** the area between the cards, for the description */
  PixelRect content;

  /** another paragraph for the help text, or nullptr */
  const char *extra_help = nullptr;

  /** @see IsCardFloating() */
  static bool card_floating;

  std::optional<DragState> drag;

  /** the InfoBox configuration as it was when the drag started */
  InfoBoxSettings::Panel drag_snapshot;

  /**
   * Where the cursor sits across the axis it moves on, as a fraction
   * of the row or column it is in.  Crossing to another row of a
   * different length keeps this fraction.
   */
  double cross_fraction = 0.5;

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

  /** when the still hold that opens the picker started */
  std::chrono::steady_clock::time_point picker_start{};

  /** the hold finished; the picker waits for lift-off */
  bool picker_armed = false;

  /** starts the fill once the press is past a tap */
  UI::Timer hold_timer{[this]{ OnHoldArmed(); }};

  /** fades the held card while that hold is running */
  UI::PeriodicTimer fade_timer{[this]{ Invalidate(); }};

  UI::PeriodicTimer shuffle_timer{[this]{ OnShuffleTimer(); }};

public:
  InfoBoxArrangeWindow(const InfoBoxLook &_look,
                       const DialogLook &_dialog_look,
                       Style _style) noexcept;

  /** Another paragraph for the help text, shown after the general one. */
  void SetExtraHelp(const char *text) noexcept {
    extra_help = text;
  }

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept;

  /** Which panel does the user arrange? */
  void SetPanel(InfoBoxSettings::Panel &_panel) noexcept;

  /**
   * @param _layout where the cards are
   * @param _content the area between them, for the description;
   * usually InfoBoxLayout::Layout::remaining, less what the caller
   * puts there itself
   */
  void SetLayout(const InfoBoxLayout::Layout &_layout,
                 PixelRect _content) noexcept;

  /** Select an InfoBox with no drag, for the cursor keys. */
  void FocusSlot(unsigned slot) noexcept {
    DescribeSlot(slot, Selection::FOCUSED);
  }

  /** Select an InfoBox after a lift-off, as if it was tapped. */
  void SelectSlot(unsigned slot) noexcept {
    DescribeSlot(slot, Selection::TOUCH);
    SetFocus();
  }

  /** Begin a drag which was started on another window. */
  void BeginDrag(unsigned slot, PixelPoint pointer, bool follow) noexcept;

  /** Abort the drag and undo the exchanges it has made. */
  void CancelDrag() noexcept;

  /** Finish a drag which is still in progress. */
  void Drop() noexcept;

  /** Explain how an InfoBox is moved and how it is replaced. */
  void ShowHelp() noexcept;

  /**
   * The card that follows the finger, for the parent to paint above
   * its buttons.
   */
  void PaintFloatingCard(Canvas &canvas) noexcept;

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

  /** Which slot covers the given position in layout coordinates? */
  [[gnu::pure]]
  int FindSlot(PixelPoint p) const noexcept;

  /** Where does the card which follows the finger sit? */
  [[gnu::pure]]
  PixelRect GetFloatingRect() const noexcept;

  /**
   * Where does the InfoBox in @p slot sit across the axis, from 0
   * (the first place of its row or column) to 1 (the last one)?  Rows
   * of different length are compared by this fraction.
   */
  [[gnu::pure]]
  double GetSlotFraction(unsigned slot) const noexcept;

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

  /** Where the name of the panel is drawn, above the description. */
  [[gnu::pure]]
  PixelRect GetPanelNameRect() const noexcept;

  /**
   * MainWindow origin of the overlay parent, or {0,0} in a dialog.
   * iOS insets the MainWindow client rect; slots stay in MainWindow
   * coordinates.
   */
  [[gnu::pure]]
  PixelPoint GetMapOrigin() const noexcept;

  /** Layout coordinates to this window. */
  [[gnu::pure]]
  PixelRect ToLocal(PixelRect rc) const noexcept;

  /** This window's local point to layout coordinates. */
  [[gnu::pure]]
  PixelPoint ToLayoutPoint(PixelPoint local) const noexcept;

  /** This window's rectangle in layout coordinates. */
  [[gnu::pure]]
  PixelRect GetLayoutBounds() const noexcept;

  /**
   * Is this cursor step toward the Help/Close (or dialog) buttons?
   * Those sit below the cards on a portrait page and to the left on
   * a landscape one.
   */
  [[gnu::pure]]
  bool TowardChrome(int dx, int dy) const noexcept;

  [[gnu::pure]]
  CardState GetCardState(unsigned slot) const noexcept;

  void DrawCard(Canvas &canvas, const PixelRect &rc, unsigned slot,
                unsigned number, CardState state) noexcept;

  void PaintCards(Canvas &canvas) noexcept;
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

  /** Start the still-hold that opens the picker. */
  void SchedulePicker() noexcept;

  /** The hold has moved or ended. */
  void CancelPicker() noexcept;

  /** The press is past a tap; start the fill. */
  void OnHoldArmed() noexcept;

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
   * Is this step along the axis the InfoBoxes are stacked on?
   */
  [[gnu::pure]]
  bool IsAlong(int dx, int dy) const noexcept;

  /**
   * The slot a cursor step from #described_slot would land on.
   * Along the stack axis the place inside the row is kept; across it
   * the nearest neighbour wins.
   *
   * @return the slot, or -1 if the layout ends there
   */
  [[gnu::pure]]
  int FindNextSlot(int dx, int dy) const noexcept;

  /** Remember where the cursor sits across the axis. */
  void RememberCross() noexcept;

  /**
   * Move the keyboard focus to the next (or previous) sibling, so Tab
   * reaches the parent's buttons.
   */
  bool MoveFocusToParent(bool forward) noexcept;

  void DescribeSlot(unsigned slot, Selection sel) noexcept;

  void ClearDrag() noexcept;

  /** Move the selection with the cursor keys or a remote stick. */
  bool MoveSelection(int dx, int dy) noexcept;

  /**
   * Enter takes the selected InfoBox, puts it down again, or opens
   * the picker if it was put down where it was taken.
   */
  bool Activate() noexcept;

  /** Move the dragged card; @p p is in layout coordinates. */
  void Drag(PixelPoint p) noexcept;

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;

  /* virtual methods from class Window */
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
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
