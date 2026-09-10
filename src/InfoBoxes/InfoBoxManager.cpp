// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Border.hpp"
#ifndef USE_WINUSER
#include "InfoBoxes/BorderWindow.hpp"
#endif
#include "InfoBoxes/Content/Factory.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "Look/InfoBoxLook.hpp"
#include "MainWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "UIState.hpp"

#include <algorithm>
#include <cstdint>

namespace InfoBoxManager {

InfoBoxLayout::Layout layout;

/**
 * The layout as calculated from the geometry alone, before the
 * contents of the current panel were applied to it.
 */
static InfoBoxLayout::Layout base_layout;

/**
 * The panel contents which #layout was derived from.
 */
static InfoBoxFactory::Type
layout_contents[InfoBoxSettings::Panel::MAX_CONTENTS];

/**
 * Is this the initial DisplayInfoBox() call?  If yes, then all
 * content objects need to be created.
 */
static bool first;

static void
DisplayInfoBox() noexcept;

static void
InfoBoxDrawIfDirty() noexcept;

/**
 * Apply the contents of the given panel to #layout.  Returns true if
 * the layout was recalculated, which means the InfoBox slots may have
 * moved.
 */
static bool
UpdateLayout(const InfoBoxSettings::Panel &panel) noexcept;

} // namespace InfoBoxManager

static bool infoboxes_dirty = false;
static bool infoboxes_hidden = false;

/**
 * Bit mask of the InfoBox slots which are currently configured as
 * #InfoBoxFactory::e_Invisible.  Kept up to date by DisplayInfoBox();
 * a change of this mask means the map window needs to be resized.
 */
static uint_least32_t invisible_mask = 0;

static_assert(InfoBoxSettings::Panel::MAX_CONTENTS <= 32,
              "invisible_mask is too small");

/* True after Create() finishes and until Destroy() runs.  Startup can
   re-enter layout (terrain load, PageActions::Update) while windows
   are half-built; defer refresh via ScheduleRefreshInfoBoxes() and
   skip work here until the manager is ready. */
static bool infoboxes_ready = false;

static InfoBoxWindow *infoboxes[InfoBoxSettings::Panel::MAX_CONTENTS];

#ifndef USE_WINUSER
/**
 * One per slot; only those of "invisible" slots are ever shown.  They
 * draw the border lines of a slot whose #InfoBoxWindow is hidden, on
 * top of the map which shows through.
 */
static InfoBoxBorderWindow *border_windows[InfoBoxSettings::Panel::MAX_CONTENTS];
#endif

[[gnu::pure]]
static const InfoBoxSettings::Panel &
GetCurrentPanel() noexcept
{
  const unsigned panel = CommonInterface::GetUIState().panel_index;
  return CommonInterface::GetUISettings().info_boxes.panels[panel];
}

/**
 * Is the given slot of the current panel configured as
 * #InfoBoxFactory::e_Invisible?  Such an InfoBox is never shown; the
 * map window is extended over it instead.
 */
[[gnu::pure]]
static bool
IsInvisible(const InfoBoxSettings::Panel &panel, unsigned i) noexcept
{
  return panel.contents[i] == InfoBoxFactory::e_Invisible;
}

/**
 * Recalculate #invisible_mask; returns true if it has changed, which
 * means the map window needs to be moved.
 */
static bool
UpdateInvisibleMask() noexcept
{
  const InfoBoxSettings::Panel &panel = GetCurrentPanel();

  uint_least32_t mask = 0;
  for (unsigned i = 0; i < InfoBoxManager::layout.count; ++i)
    if (IsInvisible(panel, i))
      mask |= uint_least32_t(1) << i;

  if (mask == invisible_mask)
    return false;

  invisible_mask = mask;
  return true;
}

/**
 * The edges of an InfoBox slot which touch another slot, split by
 * whether that neighbour is visible or "invisible".  An edge which
 * appears in neither set faces the map outside the InfoBox area.
 */
struct NeighbourEdges {
  unsigned visible = 0, invisible = 0;
};

/**
 * Determine which edges of slot @p i touch which kind of neighbour.
 * This is purely geometric and therefore works for every InfoBox
 * geometry without knowing anything about it.
 */
[[gnu::pure]]
static NeighbourEdges
GetNeighbourEdges(unsigned i) noexcept
{
  const auto &layout = InfoBoxManager::layout;
  const PixelRect &rc = layout.positions[i];

  NeighbourEdges edges;

  for (unsigned j = 0; j < layout.count; ++j) {
    if (j == i || !layout.visible[j])
      /* a collapsed slot keeps its rectangle from the geometry, which
         its neighbours have grown over; it is not an edge any more */
      continue;

    const PixelRect &other = layout.positions[j];

    /* do the two rectangles share a section of the edge, or do they
       just touch in a corner? */
    const bool overlaps_x = rc.left < other.right && other.left < rc.right;
    const bool overlaps_y = rc.top < other.bottom && other.top < rc.bottom;

    unsigned border = 0;
    if (overlaps_x && rc.top == other.bottom)
      border |= BORDERTOP;
    if (overlaps_x && rc.bottom == other.top)
      border |= BORDERBOTTOM;
    if (overlaps_y && rc.left == other.right)
      border |= BORDERLEFT;
    if (overlaps_y && rc.right == other.left)
      border |= BORDERRIGHT;

    if ((invisible_mask & (uint_least32_t(1) << j)) != 0)
      edges.invisible |= border;
    else
      edges.visible |= border;
  }

  return edges;
}

/**
 * The border flags the layout assigns to slot @p i.
 */
[[gnu::pure]]
static unsigned
GetLayoutBorder(unsigned i) noexcept
{
  /* these have been adjusted by InfoBoxLayout::ApplyContents() where
     an InfoBox has grown over a collapsed neighbour */
  return unsigned(InfoBoxManager::layout.borders[i]);
}

/**
 * Calculate the border flags for the #InfoBoxWindow of slot @p i.
 */
[[gnu::pure]]
static unsigned
CalculateBorder(const InfoBoxSettings &settings,
                const InfoBoxSettings::Panel &panel, unsigned i) noexcept
{
  if (settings.border_style == InfoBoxSettings::BorderStyle::TAB)
    /* this style draws no borders at all */
    return 0;

  if (IsInvisible(panel, i))
    /* the window is hidden; an #InfoBoxBorderWindow draws the edges
       of this slot (or, on GDI, the visible neighbours do) */
    return 0;

#ifdef USE_WINUSER
  /* no transparent windows: the neighbours have to draw the edges of
     an invisible slot, which shifts them by one pixel */
  return GetLayoutBorder(i) | GetNeighbourEdges(i).invisible;
#else
  return GetLayoutBorder(i);
#endif
}

#ifndef USE_WINUSER

/**
 * Calculate the border flags for the #InfoBoxBorderWindow of slot
 * @p i: the edges the layout assigns to this slot, but only where the
 * slot borders a visible InfoBox.
 *
 * An edge towards another invisible slot is dropped so that a group of
 * invisible slots forms one uninterrupted hole, and an edge which
 * faces the map outside the InfoBox area is dropped as well, because
 * there the map simply continues.
 */
[[gnu::pure]]
static unsigned
CalculateSlotBorder(const InfoBoxSettings &settings,
                    const InfoBoxSettings::Panel &panel, unsigned i) noexcept
{
  if (settings.border_style == InfoBoxSettings::BorderStyle::TAB ||
      !IsInvisible(panel, i))
    return 0;

  return GetLayoutBorder(i) & GetNeighbourEdges(i).visible;
}

#endif

/**
 * Apply CalculateBorder() and CalculateSlotBorder() to all windows,
 * and show or hide the border windows.  Called when the set of
 * "invisible" InfoBoxes has changed.
 */
static void
UpdateBorders() noexcept
{
  const InfoBoxSettings &settings =
    CommonInterface::GetUISettings().info_boxes;
  const InfoBoxSettings::Panel &panel = GetCurrentPanel();

  for (unsigned i = 0; i < InfoBoxManager::layout.count; ++i) {
    if (infoboxes[i] != nullptr)
      infoboxes[i]->SetBorderKind(CalculateBorder(settings, panel, i));

#ifndef USE_WINUSER
    if (border_windows[i] != nullptr) {
      const unsigned border = CalculateSlotBorder(settings, panel, i);
      border_windows[i]->SetBorderKind(border);

      if (border != 0 && !infoboxes_hidden)
        border_windows[i]->Show();
      else
        border_windows[i]->FastHide();
    }
#endif
  }
}

int
InfoBoxManager::FindInvisibleAt(PixelPoint p) noexcept
{
  if (!infoboxes_ready || invisible_mask == 0 || infoboxes_hidden)
    /* in full screen mode the whole screen is map, and the InfoBox
       slots are not shown at all */
    return -1;

  for (unsigned i = 0; i < layout.count; ++i)
    if ((invisible_mask & (uint_least32_t(1) << i)) != 0 &&
        layout.positions[i].Contains(p))
      return int(i);

  return -1;
}

PixelRect
InfoBoxManager::ExpandOverInvisible(PixelRect rc) noexcept
{
  if (invisible_mask == 0)
    return rc;

  for (unsigned i = 0; i < layout.count; ++i) {
    if ((invisible_mask & (uint_least32_t(1) << i)) == 0)
      continue;

    const PixelRect &ib = layout.positions[i];
    rc.left = std::min(rc.left, ib.left);
    rc.top = std::min(rc.top, ib.top);
    rc.right = std::max(rc.right, ib.right);
    rc.bottom = std::max(rc.bottom, ib.bottom);
  }

  return rc;
}

void
InfoBoxManager::PaintInvisible(Canvas &canvas,
                               const InfoBoxLook &look) noexcept
{
  if (!infoboxes_ready || infoboxes_hidden || invisible_mask == 0)
    return;

  for (unsigned i = 0; i < layout.count; ++i)
    if ((invisible_mask & (uint_least32_t(1) << i)) != 0)
      canvas.DrawFilledRectangle(layout.positions[i],
                                 look.background_color);
}

// TODO locking
void
InfoBoxManager::Hide() noexcept
{
  if (infoboxes_hidden)
    return;

  infoboxes_hidden = true;

  if (!infoboxes_ready)
    return;

  for (unsigned i = 0; i < layout.count; i++) {
    if (infoboxes[i] != nullptr)
      infoboxes[i]->FastHide();

#ifndef USE_WINUSER
    if (border_windows[i] != nullptr)
      border_windows[i]->FastHide();
#endif
  }
}

void
InfoBoxManager::Show() noexcept
{
  if (!infoboxes_hidden)
    return;

  infoboxes_hidden = false;

  if (!infoboxes_ready)
    return;

  const InfoBoxSettings::Panel &panel = GetCurrentPanel();

  for (unsigned i = 0; i < layout.count; i++) {
    /* "invisible" InfoBoxes stay hidden (the map is drawn there), and
       so do those which have released their space */
    if (infoboxes[i] != nullptr && layout.visible[i] &&
        !IsInvisible(panel, i))
      infoboxes[i]->Show();
  }

  /* ... but their borders are drawn */
  UpdateBorders();

  SetDirty();
}

bool
InfoBoxManager::UpdateLayout(const InfoBoxSettings::Panel &panel) noexcept
{
  if (std::equal(layout_contents, layout_contents + layout.count,
                 panel.contents))
    return false;

  std::copy_n(panel.contents, layout.count, layout_contents);

  layout = base_layout;
  InfoBoxLayout::ApplyContents(layout, panel);

  for (unsigned i = 0; i < layout.count; ++i) {
    if (infoboxes[i] == nullptr)
      continue;

    if (!layout.visible[i]) {
      infoboxes[i]->Hide();
#ifndef USE_WINUSER
      if (border_windows[i] != nullptr)
        border_windows[i]->FastHide();
#endif
      continue;
    }

    infoboxes[i]->Move(layout.positions[i]);

#ifndef USE_WINUSER
    if (border_windows[i] != nullptr)
      border_windows[i]->Move(layout.positions[i]);
#endif

    if (!infoboxes_hidden && !IsInvisible(panel, i))
      infoboxes[i]->Show();
  }

  /* the collapsed slots have handed their edges to their neighbours */
  UpdateBorders();

  return true;
}

void
InfoBoxManager::DisplayInfoBox() noexcept
{
  static int DisplayTypeLast[InfoBoxSettings::Panel::MAX_CONTENTS];
  static bool displaying = false;

  if (!infoboxes_ready || displaying)
    return;

  displaying = true;

  // JMW note: this is updated every GPS time step

  const unsigned panel = CommonInterface::GetUIState().panel_index;

  const InfoBoxSettings::Panel &settings =
    CommonInterface::GetUISettings().info_boxes.panels[panel];

  const bool layout_changed = UpdateLayout(settings);

  for (unsigned i = 0; i < layout.count; i++) {
    if (infoboxes[i] == nullptr)
      continue;

    // All calculations are made in a separate thread. Slow calculations
    // should apply to the function DoCalculationsSlow()
    // Do not put calculations here!

    InfoBoxFactory::Type DisplayType = settings.contents[i];
    if ((unsigned)DisplayType > (unsigned)InfoBoxFactory::MAX_TYPE_VAL)
      DisplayType = InfoBoxFactory::NavAltitude;

    const bool needupdate = ((DisplayType != DisplayTypeLast[i]) || first);

    if (needupdate || !infoboxes[i]->HasContent()) {
      infoboxes[i]->SetTitle(gettext(InfoBoxFactory::GetCaption(DisplayType)));
      infoboxes[i]->SetContentProvider(InfoBoxFactory::Create(DisplayType));
      DisplayTypeLast[i] = DisplayType;
    }

    /* apply the visibility on every pass: Show() may have run while
       UIState::panel_index still pointed at the previous page, and
       would then have shown an InfoBox which this page collapses or
       draws the map over */
    if (DisplayType == InfoBoxFactory::e_Invisible || !layout.visible[i])
      infoboxes[i]->FastHide();
    else if (!infoboxes_hidden)
      infoboxes[i]->Show();

    infoboxes[i]->UpdateContent();
  }

  first = false;
  displaying = false;

  const bool mask_changed = UpdateInvisibleMask();

  if (mask_changed)
    /* the set of "invisible" InfoBoxes has changed: the neighbours
       must draw the now unpainted shared edges themselves */
    UpdateBorders();

  if ((mask_changed || (layout_changed && invisible_mask != 0)) &&
      CommonInterface::main_window != nullptr)
    /* the map window covers the "invisible" slots: follow both a
       change of the set and a move of the slots which the new
       contents of the panel may have caused */
    CommonInterface::main_window->RelayoutMapArea();
}

void
InfoBoxManager::InfoBoxDrawIfDirty() noexcept
{
  // No need to redraw map or infoboxes if screen is blanked.
  // This should save lots of battery power due to CPU usage
  // of drawing the screen

  if (infoboxes_dirty && !infoboxes_hidden &&
      !CommonInterface::GetUIState().screen_blanked) {
    DisplayInfoBox();
    infoboxes_dirty = false;
  }
}

InfoBoxWindow *
InfoBoxManager::GetWindow(unsigned id) noexcept
{
  if (!infoboxes_ready || id >= layout.count)
    return nullptr;

  return infoboxes[id];
}

void
InfoBoxManager::SetDirty() noexcept
{
  infoboxes_dirty = true;
}

void
InfoBoxManager::InvalidateAfterLanguageChange() noexcept
{
  first = true;
  SetDirty();
}

void
InfoBoxManager::ScheduleRedraw() noexcept
{
  if (infoboxes_hidden || !infoboxes_ready)
    return;

  for (unsigned i = 0; i < layout.count; i++) {
    if (infoboxes[i] != nullptr)
      infoboxes[i]->Invalidate();
  }
}

bool
InfoBoxManager::IsReady() noexcept
{
  return infoboxes_ready;
}

void
InfoBoxManager::ProcessTimer() noexcept
{
  if (!infoboxes_ready)
    return;

  InfoBoxDrawIfDirty();
}

void
InfoBoxManager::Create(ContainerWindow &parent,
                       const InfoBoxLayout::Layout &_layout,
                       const InfoBoxLook &look) noexcept
{
  const InfoBoxSettings &settings =
    CommonInterface::GetUISettings().info_boxes;

  const InfoBoxSettings::Panel &panel = GetCurrentPanel();

  infoboxes_ready = false;
  first = true;
  base_layout = _layout;
  layout = _layout;
  InfoBoxLayout::ApplyContents(layout, panel);
  std::copy_n(panel.contents, layout.count, layout_contents);

  /* determine the "invisible" slots before the caller queries
     ExpandOverInvisible() to position the map window */
  invisible_mask = 0;
  UpdateInvisibleMask();

  for (unsigned i = layout.count; i < InfoBoxSettings::Panel::MAX_CONTENTS; ++i) {
    infoboxes[i] = nullptr;
#ifndef USE_WINUSER
    border_windows[i] = nullptr;
#endif
  }

  WindowStyle style;
  style.Hide();

  // create infobox windows
  for (unsigned i = layout.count; i-- > 0;) {
    const PixelRect &rc = layout.positions[i];

    infoboxes[i] = new InfoBoxWindow(parent, rc,
                                     CalculateBorder(settings, panel, i),
                                     settings, look,
                                     i, style);
  }

#ifndef USE_WINUSER
  /* create the border windows after the InfoBox windows so they end up
     in front of them; they are shown only for "invisible" slots (by
     UpdateBorders(), from Show()) */
  for (unsigned i = layout.count; i-- > 0;)
    border_windows[i] =
      new InfoBoxBorderWindow(parent, layout.positions[i],
                              CalculateSlotBorder(settings, panel, i),
                              look);
#endif

  infoboxes_hidden = true;
  infoboxes_ready = true;
}

void
InfoBoxManager::Destroy() noexcept
{
  infoboxes_ready = false;
  first = true;

  for (unsigned i = 0; i < InfoBoxSettings::Panel::MAX_CONTENTS; ++i) {
    delete infoboxes[i];
    infoboxes[i] = nullptr;

#ifndef USE_WINUSER
    delete border_windows[i];
    border_windows[i] = nullptr;
#endif
  }
}

void
InfoBoxManager::ShowInfoBoxPicker(const int i) noexcept
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = CommonInterface::GetUIState().panel_index;
  InfoBoxSettings::Panel &panel = settings.panels[panel_index];

  const InfoBoxFactory::Type old_type = panel.contents[i];

  ComboList list;
  for (unsigned j = InfoBoxFactory::MIN_TYPE_VAL; j < InfoBoxFactory::NUM_TYPES; j++) {
    const char *desc = InfoBoxFactory::GetDescription((InfoBoxFactory::Type)j);
    list.Append(j, gettext(InfoBoxFactory::GetName((InfoBoxFactory::Type)j)),
                gettext(InfoBoxFactory::GetName((InfoBoxFactory::Type)j)),
                desc != NULL ? gettext(desc) : NULL);
  }

  list.Sort();
  list.current_index = list.LookUp(old_type);

  /* let the user select */

  StaticString<20> caption;
  caption.Format("%s: %d", _("InfoBox"), i + 1);
  int result = ComboPicker(caption, list, nullptr, true);
  if (result < 0)
    return;

  /* was there a modification? */

  InfoBoxFactory::Type new_type = (InfoBoxFactory::Type)list[result].int_value;
  if (new_type == old_type)
    return;

  /* yes: apply and save it */

  panel.contents[i] = new_type;
  DisplayInfoBox();

  Profile::Save(Profile::map, panel, panel_index);
}

void
InfoBoxManager::ClearFocusExcept(unsigned except_id) noexcept
{
  for (unsigned i = 0; i < layout.count; i++) {
    if (i != except_id && infoboxes[i] != nullptr && infoboxes[i]->HasFocus()) {
      infoboxes[i]->FocusParent();
    }
  }
}
