// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CurrentAirspacesWidget.hpp"
#include "AirspaceWarningMonitor.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Form/Button.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/AirspaceLook.hpp"
#include "Look/Colors.hpp"
#include "Look/DialogLook.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/ContainerWindow.hpp"
#include <fmt/format.h>

#include <cassert>
#include <memory>

namespace {

static const Font &
GetRowFont() noexcept
{
  return UIGlobals::GetDialogLook().bold_font;
}

class AirspaceRowsWindow final : public ContainerWindow {
  CurrentAirspacesWidget &widget;
  ProtectedAirspaceWarningManager &manager;

  std::array<CurrentAirspacesWidget::Entry, 2> entries;
  unsigned n_entries;

  /* A revoke button is created only for Cleared entries. */
  std::array<std::unique_ptr<Button>, 2> revoke_buttons;

  unsigned row_height = 0;
  unsigned status_column_width = 0;

public:
  AirspaceRowsWindow(CurrentAirspacesWidget &_widget,
                     ProtectedAirspaceWarningManager &_manager,
                     std::span<const CurrentAirspacesWidget::Entry> _entries)
    noexcept
    :widget(_widget), manager(_manager),
     n_entries(_entries.size())
  {
    assert(!_entries.empty());
    assert(_entries.size() <= entries.size());
    for (unsigned i = 0; i < n_entries; ++i)
      entries[i] = _entries[i];
  }

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept {
    const Font &font = GetRowFont();
    /* Compact single-line rows, smaller than the warning
       monitor (which uses GetMinimumControlHeight). */
    row_height = font.GetHeight() + Layout::GetTextPadding();

    status_column_width = std::max(font.TextSize(_("(inside)")).width,
                                   font.TextSize(_("cleared")).width)
      + 2 * Layout::GetTextPadding();

    WindowStyle style;
    style.Hide();
    style.ControlParent();
    ContainerWindow::Create(parent, rc, style);

    LayoutChildren();
  }

  [[gnu::pure]]
  unsigned GetPreferredHeight() const noexcept {
    return row_height * n_entries;
  }

protected:
  void OnResize(PixelSize new_size) noexcept override {
    ContainerWindow::OnResize(new_size);
    LayoutChildren();
  }

  void OnPaint(Canvas &canvas) noexcept override {
    const PixelRect client_rc = GetClientRect();
    const DialogLook &dialog_look = UIGlobals::GetDialogLook();
    canvas.DrawFilledRectangle(client_rc, dialog_look.background_color);

    for (unsigned i = 0; i < n_entries; ++i)
      PaintRow(canvas, i);

    ContainerWindow::OnPaint(canvas);
  }

  bool OnMouseDown(PixelPoint p) noexcept override {
    if (ContainerWindow::OnMouseDown(p))
      return true;

    if (row_height == 0)
      return false;

    const PixelRect client_rc = GetClientRect();
    const int rel_y = p.y - client_rc.top;
    if (rel_y < 0)
      return false;

    const unsigned i = unsigned(rel_y) / row_height;
    if (i >= n_entries)
      return false;

    dlgAirspaceDetails(entries[i].airspace, &manager);
    return true;
  }

private:
  bool HasRevoke(unsigned i) const noexcept {
    return entries[i].role == CurrentAirspacesWidget::Role::Cleared;
  }

  PixelRect ComputeRowRect(unsigned i) const noexcept {
    PixelRect rc = GetClientRect();
    rc.top += i * row_height;
    rc.bottom = rc.top + row_height;
    return rc;
  }

  /** Right edge of the row reserved by the status badge column. */
  int GetStatusLeft() const noexcept {
    return GetClientRect().right - int(status_column_width);
  }

  PixelRect ComputeRevokeRect(unsigned i) const noexcept {
    PixelRect rc = ComputeRowRect(i);
    const unsigned padding = Layout::GetTextPadding();
    // Button sits immediately left of the status column
    const unsigned width = revoke_buttons[i] != nullptr
      ? revoke_buttons[i]->GetMinimumWidth()
      : 0u;
    rc.right = GetStatusLeft() - int(padding);
    rc.left = rc.right - int(width);
    rc.top += 1;
    rc.bottom -= 1;
    return rc;
  }

  void LayoutChildren() noexcept {
    if (row_height == 0)
      return;

    const DialogLook &dialog_look = UIGlobals::GetDialogLook();

    for (unsigned i = 0; i < n_entries; ++i) {
      if (!HasRevoke(i))
        continue;

      if (revoke_buttons[i] == nullptr) {
        /* Create at a placeholder rect; we resize immediately after
           construction once GetMinimumWidth() is meaningful. */
        ConstAirspacePtr airspace = entries[i].airspace;
        CurrentAirspacesWidget *widget_ptr = &widget;
        WindowStyle bstyle;
        bstyle.TabStop();
        revoke_buttons[i] = std::make_unique<Button>(
          *this, dialog_look.button, _("Revoke"),
          ComputeRowRect(i), bstyle,
          [airspace = std::move(airspace), widget_ptr]() {
            widget_ptr->RevokeClearance(airspace);
            /* widget_ptr (and this button) can be destroyed by the
               above call; do not touch anything past here. */
          });
      }

      revoke_buttons[i]->Move(ComputeRevokeRect(i));
    }
  }

  void PaintRow(Canvas &canvas, unsigned i) noexcept {
    const CurrentAirspacesWidget::Entry &entry = entries[i];
    const bool has_revoke = HasRevoke(i);
    const PixelRect row_rc = ComputeRowRect(i);
    const AbstractAirspace &airspace = *entry.airspace;
    const unsigned padding = Layout::GetTextPadding();
    const unsigned line_height = row_rc.GetHeight();

    PixelRect layout_rc = row_rc;

    /* Icon (left): small filled rectangle in airspace class colours. */
    {
      const AirspaceLook &airspace_look = UIGlobals::GetMapLook().airspace;
      const AirspaceRendererSettings &renderer_settings =
        CommonInterface::GetMapSettings().airspace;

      const AirspaceClass type = airspace.GetTypeOrClass();
      PixelRect icon_rc;
      icon_rc.left = layout_rc.left + padding;
      icon_rc.top = layout_rc.top + 1;
      icon_rc.right = layout_rc.left + line_height - padding;
      icon_rc.bottom = layout_rc.bottom - 1;

      if (AirspacePreviewRenderer::PrepareFill(canvas, type, airspace_look,
                                               renderer_settings)) {
        canvas.DrawRectangle(icon_rc);
        AirspacePreviewRenderer::UnprepareFill(canvas);
      }
      if (AirspacePreviewRenderer::PrepareOutline(canvas, type, airspace_look,
                                                  renderer_settings))
        canvas.DrawRectangle(icon_rc);

      layout_rc.left += line_height + padding;
    }

    canvas.Select(GetRowFont());
    canvas.SetBackgroundTransparent();

    // Status badge at the right edge of the row.
    PixelRect status_rc = row_rc;
    status_rc.left = status_rc.right - int(status_column_width);
    const char *state_text =
      entry.role == CurrentAirspacesWidget::Role::Suppressed
      ? _("(inside)") : _("cleared");

    // Reserve space on the right for the revoke button + status.
    layout_rc.right = status_rc.left - int(padding);
    if (has_revoke && revoke_buttons[i] != nullptr)
      layout_rc.right -=
        int(revoke_buttons[i]->GetMinimumWidth()) + int(padding);

    // Name + class/type (left), vertically centred.
    canvas.SetTextColor(COLOR_CLEARANCE);
    const auto text = fmt::format("{} {}", airspace.GetName(),
                                  AirspaceFormatter::GetClassOrType(airspace));
    const PixelSize text_size = canvas.CalcTextSize(text.c_str());
    const int text_y = layout_rc.top
      + (int(line_height) - int(text_size.height)) / 2;
    canvas.DrawClippedText({layout_rc.left, text_y},
                           layout_rc.GetWidth(), text.c_str());

    // Status badge: filled rectangle with centred text
    PixelRect badge_rc = status_rc;
    badge_rc.top += 1;
    badge_rc.bottom -= 1;
    canvas.DrawFilledRectangle(badge_rc, COLOR_AIRSPACE_CLEARED);

    canvas.SetTextColor(COLOR_BLACK);
    const PixelSize ts = canvas.CalcTextSize(state_text);
    canvas.DrawText(status_rc.CenteredTopLeft(ts), state_text);
  }
};

} // namespace

CurrentAirspacesWidget::CurrentAirspacesWidget(
    AirspaceWarningMonitor &_monitor,
    ProtectedAirspaceWarningManager &_manager,
    std::span<const Entry> _entries) noexcept
  :monitor(_monitor), manager(_manager),
   n_entries(_entries.size())
{
  assert(!_entries.empty());
  assert(_entries.size() <= entries.size());
  for (unsigned i = 0; i < n_entries; ++i)
    entries[i] = _entries[i];
}

CurrentAirspacesWidget::~CurrentAirspacesWidget() noexcept
{
  assert(monitor.current_widget == this);
  monitor.current_widget = nullptr;
}

bool
CurrentAirspacesWidget::Matches(std::span<const Entry> other) const noexcept
{
  if (other.size() != n_entries)
    return false;
  for (unsigned i = 0; i < n_entries; ++i)
    if (!(entries[i] == other[i]))
      return false;
  return true;
}

void
CurrentAirspacesWidget::RevokeClearance(ConstAirspacePtr airspace) noexcept
{
  manager.SetCleared(std::move(airspace), false);

  /* Let the monitor recompute and either replace this widget with one
     showing the remaining entries, or destroy if nothing is left. */
  monitor.Schedule();
  monitor.Check();
  /* 'this' may have been destroyed by Check(),
     do not access any members past this point. */
}

PixelSize
CurrentAirspacesWidget::GetMinimumSize() const noexcept
{
  const Font &font = UIGlobals::GetDialogLook().bold_font;
  const unsigned row_height = font.GetHeight() + Layout::GetTextPadding();
  return PixelSize(0u, row_height * n_entries);
}

PixelSize
CurrentAirspacesWidget::GetMaximumSize() const noexcept
{
  return GetMinimumSize();
}

void
CurrentAirspacesWidget::Prepare(ContainerWindow &parent,
                                const PixelRect &rc) noexcept
{
  auto window = std::make_unique<AirspaceRowsWindow>(
    *this, manager, std::span<const Entry>(entries.data(), n_entries));
  window->Create(parent, rc);
  SetWindow(std::move(window));
}

unsigned
CurrentAirspacesWidget::CollectEntries(
    const ProtectedAirspaceWarningManager &manager,
    std::array<Entry, 2> &out) noexcept
{
  /* Suppressed entries take precedence over cleared, and we have at
     most two rows. Collect suppressed first, then cleared. */

  Entry suppressed[2];
  unsigned n_suppressed = 0;
  Entry cleared[2];
  unsigned n_cleared = 0;

  const ProtectedAirspaceWarningManager::Lease lease(manager);
  for (auto it = lease->begin(), end = lease->end(); it != end; ++it) {
    const AirspaceWarning &w = *it;
    if (!w.IsInside())
      continue;
    if (w.HasExplicitAck())
      continue;

    if (w.IsCoveredByClearance()) {
      if (n_suppressed < 2)
        suppressed[n_suppressed++] = Entry{w.GetAirspacePtr(),
                                           Role::Suppressed};
    } else if (w.IsCleared()) {
      if (n_cleared < 2)
        cleared[n_cleared++] = Entry{w.GetAirspacePtr(), Role::Cleared};
    }
  }

  /* Trigger condition: at least one suppressed entry. */
  if (n_suppressed == 0)
    return 0;

  unsigned n = 0;
  for (unsigned i = 0; i < n_suppressed && n < 2; ++i)
    out[n++] = suppressed[i];
  for (unsigned i = 0; i < n_cleared && n < 2; ++i)
    out[n++] = cleared[i];

  return n;
}
