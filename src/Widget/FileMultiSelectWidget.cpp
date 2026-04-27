// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileMultiSelectWidget.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Icon.hpp"
#include "Resources.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/Point.hpp"
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
const char *
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
ContainsPath(const std::vector<AllocatedPath> &paths, const Path &p) noexcept
{
  return std::any_of(paths.begin(), paths.end(), [p](const auto &item) noexcept {
    return Path(item) == p;
  });
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
  if (loader_) {
    // loader returns FileItem with is_dir flags and owned paths
    auto loaded = loader_();
    for (auto &it : loaded) {
      // Never filter out directories — they are needed for navigation.
      if (!it.is_dir && filter_ && !filter_(it.path))
        continue;
      items_.push_back(std::move(it));
    }
    return;
  }

  if (df_) {
    FileDataField &file_field = df_->GetFileDataField();
    for (unsigned i = 0; i < file_field.size(); ++i) {
      const Path p = file_field.GetItem(i).path;
      if (filter_ && !filter_(p))
        continue;

      FileItem it;
      it.path = AllocatedPath(p);
      it.exists = true;
      items_.push_back(std::move(it));
    }
  }
}

void
FileMultiSelectWidget::SetFilter(std::function<bool(const Path &)> filter) noexcept
{
  filter_ = std::move(filter);
  // When the filter changes, force a full reload so previous_items are
  // not re-merged into the visible list (which would defeat the filter).
  refreshed_ = false;
}

std::vector<Path>
FileMultiSelectWidget::GetCurrentItems() const noexcept
{
  if (df_)
    return df_->GetPathFiles();

  return {};
}

void
FileMultiSelectWidget::ApplySelection(const std::vector<Path> &paths) noexcept
{
  for (const auto &path : paths) {
    for (unsigned i = 0; i < items_.size(); ++i) {
      if (items_[i].is_dir)
        continue;
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
    if (!ContainsPath(items_, path)) {
      FileItem it;
      it.path = AllocatedPath(path);
      it.exists = false;
      items_.push_back(std::move(it));
    }
  }
}

void
FileMultiSelectWidget::Refresh() noexcept
{
  const auto saved_selection = refreshed_ ? GetSelectedPaths() : std::vector<Path>{};
  std::vector<FileItem> previous_items;
  if (refreshed_)
    previous_items = std::move(items_);

  // Extract paths before previous_items may be moved.
  std::vector<AllocatedPath> previous_paths;
  if (!previous_items.empty()) {
    previous_paths.reserve(previous_items.size());
    for (const auto &it : previous_items)
      previous_paths.emplace_back(Path(it.path));
  }

  LoadFiles();
  const std::vector<Path> current_items = GetCurrentItems();

  // Reserve to reduce reallocations while merging.
  items_.reserve(items_.size() + previous_items.size() + current_items.size());

  MergePreviousItems(previous_items);

  if (df_)
    MergePaths(current_items);

  SetLengthWithSelection(items_.size());
  ClearSelection();

  RestoreAfterRefresh(saved_selection, previous_paths, current_items);

  refreshed_ = true;
}

void
FileMultiSelectWidget::MergePreviousItems(std::vector<FileItem> &previous_items) noexcept
{
  if (!refreshed_ || loader_)
    return;

  for (auto &item : previous_items) {
    if (!ContainsPath(items_, item.path))
      items_.push_back(std::move(item));
  }
}

void
FileMultiSelectWidget::RestoreAfterRefresh(const std::vector<Path> &saved_selection,
                                           const std::vector<AllocatedPath> &previous_paths,
                                           const std::vector<Path> &current_items) noexcept
{
  if (!refreshed_) {
    if (df_)
      ApplySelection(current_items);
    return;
  }

  if (loader_)
    ApplySelection(saved_selection);
  else
    RestoreSelection(saved_selection, previous_paths, current_items);
}

std::vector<Path>
FileMultiSelectWidget::GetSelectedPaths() const noexcept
{
  std::vector<Path> result;
  auto indices = GetSelectedIndices();
  result.reserve(indices.size());

  for (unsigned idx : indices) {
    if (idx < items_.size() && !items_[idx].is_dir)
      result.push_back(items_[idx].path);
  }

  return result;
}

std::vector<Path>
FileMultiSelectWidget::GetAllPaths() const noexcept
{
  std::vector<Path> result;
  result.reserve(items_.size());

  for (const auto &item : items_)
    if (!item.is_dir)
      result.push_back(item.path);

  return result;
}

void
FileMultiSelectWidget::ShowHelp() noexcept
{
  if (help_text_ != nullptr)
    HelpDialog(caption_, help_text_);
}

void
FileMultiSelectWidget::SetSelectionChangedCallback(std::function<void()> cb) noexcept
{
  selection_changed_callback_ = std::move(cb);
}

void
FileMultiSelectWidget::RestoreSelection(const std::vector<Path> &saved_selection,
                                        const std::vector<AllocatedPath> &previous_items_paths,
                                        const std::vector<Path> &current_items) noexcept
{
  for (unsigned i = 0; i < items_.size(); ++i) {
    const Path p = items_[i].path;
    if (items_[i].is_dir)
      continue;

    // Restore previously selected items
    if (ContainsPath(saved_selection, p)) {
      SetSelected(i, true);
      continue;
    }

    // Auto-select new items from current_items that weren't in previous_items
    if (ContainsPath(current_items, p) && !ContainsPath(previous_items_paths, p))
      SetSelected(i, true);
  }
}

void
FileMultiSelectWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  // Load folder icon for directory entries (only needed when loader
  // is set, because only loaders can produce directory items).
  if (loader_)
    folder_icon_.LoadResource(IDB_FOLDER_ALL);

  // Choose renderer based on whether second-row providers are set.
  use_two_rows_ = (second_left_provider_ || second_right_provider_);

  const DialogLook &look = UIGlobals::GetDialogLook();
  const unsigned row_height = ComputeRowHeight();
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

  if (item.is_dir) {
    PaintDirectoryItem(canvas, rc, item);
    return;
  }

  PaintFileItem(canvas, rc, idx, item);
}

void
FileMultiSelectWidget::PaintDirectoryItem(Canvas &canvas, PixelRect rc,
                                          const FileItem &item) noexcept
{
  const unsigned padding = Layout::GetTextPadding();
  PixelRect text_rc = rc;
  text_rc.left = rc.left + (int)padding;

  const unsigned max_icon_size =
    rc.GetHeight() > 2 * padding ? rc.GetHeight() - 2 * padding : 0;

  if (folder_icon_.IsDefined() && max_icon_size > 0) {
    // Centre the icon in the same column as the file checkboxes
    // (which occupy max_icon_size × max_icon_size).
    // Use the target_height overload so the icon scales to fit the row,
    // matching the checkbox behaviour at any DPI / window size.
    PixelPoint center{
      rc.left + (int)padding + (int)max_icon_size / 2,
      rc.top + (int)rc.GetHeight() / 2,
    };
    folder_icon_.Draw(canvas, center, max_icon_size);
    text_rc.left += (int)max_icon_size + 2 * (int)padding;
  }

  const char *name = item.is_up ? "..." : GetComparableName(item.path);
  if (use_two_rows_) {
    const auto &font = two_text_rows_renderer_.GetFirstFont();
    canvas.Select(font);
    const int y = text_rc.top + (text_rc.GetHeight() - (int)font.GetHeight()) / 2;
    canvas.DrawClippedText({text_rc.left + two_text_rows_renderer_.GetX(), y},
                           text_rc, name);
  } else {
    text_row_renderer_.DrawTextRow(canvas, text_rc, name);
  }
}

void
FileMultiSelectWidget::PaintFileItem(Canvas &canvas, PixelRect rc,
                                     unsigned idx,
                                     const FileItem &item) noexcept
{
  const bool selected = IsSelected(idx);
  const unsigned padding = Layout::GetTextPadding();

  // Draw checkbox at left and then render text columns/rows according to
  // configured providers and selected renderer.
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
                          const char *fallback = nullptr) -> const char* {
    if (!provider)
      return fallback;

    const char *text = provider(item);
    return (text && *text) ? text : fallback;
  };

  if (use_two_rows_) {
    const char *first_left = resolve_text(first_left_provider_, GetComparableName(item.path));
    const char *first_right = resolve_text(first_right_provider_);
    const char *second_left = resolve_text(second_left_provider_);
    const char *second_right = resolve_text(second_right_provider_);

    two_text_rows_renderer_.DrawFirstRow(canvas, text_rc, first_left);
    if (second_left != nullptr)
      two_text_rows_renderer_.DrawSecondRow(canvas, text_rc, second_left);

    if (first_right != nullptr)
      two_text_rows_renderer_.DrawRightFirstRow(canvas, text_rc,
                                                first_right);

    if (second_right != nullptr)
      two_text_rows_renderer_.DrawRightSecondRow(canvas, text_rc, second_right);
  } else {
    const char *left_text = resolve_text(first_left_provider_, GetComparableName(item.path));
    const char *right_text = resolve_text(first_right_provider_);

    text_row_renderer_.DrawTextRow(canvas, text_rc, left_text);
    if (right_text != nullptr) {
      const int left_end = text_row_renderer_.NextColumn(canvas, text_rc, left_text);
      const int right_limit = text_row_renderer_.PreviousRightColumn(canvas, text_rc, right_text);
      if (right_limit > left_end + (int)Layout::GetTextPadding())
        text_row_renderer_.DrawRightColumn(canvas, text_rc, right_text);
    }
  }
}

void
FileMultiSelectWidget::SetNavigateCallback(std::function<void(AllocatedPath)> cb) noexcept
{
  navigate_callback_ = std::move(cb);
}

void
FileMultiSelectWidget::OnActivateItem(unsigned index) noexcept
{
  if (index >= items_.size())
    return;

  const auto &item = items_[index];
  if (item.is_dir)
    ActivateDirectoryItem(item);
  else
    ToggleSelection(index);
}

void
FileMultiSelectWidget::ActivateDirectoryItem(const FileItem &item) noexcept
{
  if (navigate_callback_)
    navigate_callback_(Path(item.path));
}

unsigned
FileMultiSelectWidget::OnListResized() noexcept
{
  return ComputeRowHeight();
}

unsigned
FileMultiSelectWidget::ComputeRowHeight() noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  /* Row height is driven purely by font metrics so that it adapts
     when Layout scale factors change on window resize.  The folder
     icon scales to fit via the target_height Draw() overload. */
  return use_two_rows_
    ? two_text_rows_renderer_.CalculateLayout(*look.list.font, look.small_font)
    : text_row_renderer_.CalculateLayout(*look.list.font);
}

void
FileMultiSelectWidget::OnSelectionChanged() noexcept
{
  if (selection_changed_callback_)
    selection_changed_callback_();
}

void
FileMultiSelectWidget::SelectAllFiles() noexcept
{
  for (unsigned i = 0; i < items_.size(); ++i) {
    if (!items_[i].is_dir)
      SetSelected(i, true);
    else
      SetSelected(i, false);
  }
}
