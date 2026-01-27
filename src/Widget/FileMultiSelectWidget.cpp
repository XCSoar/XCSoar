// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileMultiSelectWidget.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/Rect.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "util/StaticString.hxx"
#include "util/UTF8.hpp"
#include "Dialogs/HelpDialog.hpp"
#include "Form/CheckBox.hpp"

#include <algorithm>

namespace {

/**
 * Helper to extract comparable filename from path.
 */
[[gnu::pure]]
const TCHAR *
GetComparableName(const Path &path) noexcept
{
  const auto base = path.GetBase();
  return (base != nullptr) ? base.c_str() : path.c_str();
}

static bool
ContainsPath(const std::vector<Path> &paths, const Path &p) noexcept
{
  return std::find(paths.begin(), paths.end(), p) != paths.end();
}

static bool
ContainsPath(const std::vector<FileMultiSelectWidget::FileItem> &items, const Path &p) noexcept
{
  return std::any_of(items.begin(), items.end(), [p](const auto &item) noexcept {
    return item.path == p;
  });
}

} // namespace

void
FileMultiSelectWidget::LoadFiles() noexcept
{
  items_.clear();
  FileDataField &file_field = df_.GetFileDataField();
  for (unsigned i = 0; i < file_field.size(); ++i)
    items_.push_back({file_field.GetItem(i).path});
}

void
FileMultiSelectWidget::ApplySelection(const std::vector<Path> &paths) noexcept
{
  for (const auto &path : paths) {
    for (unsigned i = 0; i < items_.size(); ++i) {
      if (items_[i].path == path) {
        SetSelected(i, true);
        break;
      }
    }
  }
}

void
FileMultiSelectWidget::MergePaths(const std::vector<Path> &paths) noexcept
{
  for (const auto &path : paths) {
    if (!ContainsPath(items_, path))
      items_.push_back({path});
  }
}

void
FileMultiSelectWidget::Refresh() noexcept
{
  const auto saved_selection = refreshed_ ? GetSelectedPaths() : std::vector<Path>{};
  const auto previous_items = refreshed_ ? items_ : std::vector<FileItem>{};

  LoadFiles();
  auto current_items = df_.GetPathFiles();

  // Reserve to avoid reallocations when merging
  items_.reserve(items_.size() + previous_items.size() + current_items.size());

  // Merge previous items to preserve user's deselections
  if (refreshed_) {
    for (const auto &item : previous_items) {
      if (!ContainsPath(items_, item.path))
        items_.push_back(item);
    }
  }

  MergePaths(current_items);

  SetLengthWithSelection(items_.size());
  ClearSelection();

  if (refreshed_) {
    RestoreSelection(saved_selection, previous_items, current_items);
  } else {
    ApplySelection(current_items);
  }

  refreshed_ = true;
}

std::vector<Path>
FileMultiSelectWidget::GetSelectedPaths() const noexcept
{
  std::vector<Path> result;
  auto indices = GetSelectedIndices();
  result.reserve(indices.size());

  for (unsigned idx : indices) {
    if (idx < items_.size())
      result.push_back(items_[idx].path);
  }

  return result;
}

void
FileMultiSelectWidget::ShowHelp() noexcept
{
  if (help_text_ != nullptr)
    HelpDialog(caption_, help_text_);
}

void
FileMultiSelectWidget::RestoreSelection(const std::vector<Path> &saved_selection,
                                        const std::vector<FileItem> &previous_items,
                                        const std::vector<Path> &current_items) noexcept
{
  for (unsigned i = 0; i < items_.size(); ++i) {
    const Path p = items_[i].path;

    // Restore previously selected items
    if (ContainsPath(saved_selection, p)) {
      SetSelected(i, true);
      continue;
    }

    // Auto-select new items from current_items that weren't in previous_items
    if (ContainsPath(current_items, p) && !ContainsPath(previous_items, p))
      SetSelected(i, true);
  }
}

void
FileMultiSelectWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  // Choose renderer based on whether second-row providers are set.
  use_two_rows_ = (second_left_provider_ || second_right_provider_);

  unsigned row_height;
  if (use_two_rows_)
    row_height = two_text_rows_renderer_.CalculateLayout(*look.list.font,
                                               look.small_font);
  else
    row_height = text_row_renderer_.CalculateLayout(*look.list.font);
  CreateList(parent, look, rc, row_height);
  MultiSelectListWidget::Prepare(parent, rc);
}

void
FileMultiSelectWidget::Show(const PixelRect &rc) noexcept
{
  MultiSelectListWidget::Show(rc);
  Refresh();
}

void
FileMultiSelectWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned idx) noexcept
{
  if (idx >= items_.size())
    return;

  const auto &item = items_[idx];
  const bool selected = IsSelected(idx);

  // Draw checkbox at left and then render text columns/rows according to
  // configured providers and selected renderer.
  const unsigned padding = Layout::GetTextPadding();
  unsigned box_size = rc.GetHeight() > 2 * padding ? rc.GetHeight() - 2 * padding : 0;
  PixelRect box_rc;
  box_rc.left = rc.left + (int)padding;
  box_rc.top = rc.top + (int)padding;
  box_rc.right = box_rc.left + (int)box_size;
  box_rc.bottom = box_rc.top + (int)box_size;

  const bool focused = !HasCursorKeys() || GetList().HasFocus();
  DrawCheckBox(canvas, UIGlobals::GetDialogLook(), box_rc, selected, focused, false, true);

  PixelRect text_rc = rc;
  text_rc.left = box_rc.right + 2 * (int)padding;

  // Helper to invoke provider and check for valid text
  auto resolve_text = [&item](const TextProvider &provider,
                          const TCHAR *fallback = nullptr) -> const TCHAR* {
    if (!provider)
      return fallback;

    const TCHAR *text = provider(item);
    return (text && *text) ? text : fallback;
  };

  if (use_two_rows_) {
    two_text_rows_renderer_.DrawFirstRow(canvas, text_rc, resolve_text(first_left_provider_, GetComparableName(item.path)));
    if (const auto text = resolve_text(first_right_provider_))
      two_text_rows_renderer_.DrawRightFirstRow(canvas, text_rc, text);
    if (const auto text = resolve_text(second_left_provider_))
      two_text_rows_renderer_.DrawSecondRow(canvas, text_rc, text);
    if (const auto text = resolve_text(second_right_provider_))
      two_text_rows_renderer_.DrawRightSecondRow(canvas, text_rc, text);
  } else {
    text_row_renderer_.DrawTextRow(canvas, text_rc, resolve_text(first_left_provider_, GetComparableName(item.path)));
    if (const auto text = resolve_text(first_right_provider_))
      text_row_renderer_.DrawRightColumn(canvas, text_rc, text);
  }
}
