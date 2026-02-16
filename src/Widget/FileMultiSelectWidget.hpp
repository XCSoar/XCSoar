// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MultiSelectListWidget.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "system/Path.hpp"

#include <functional>
#include <vector>

class ContainerWindow;
struct PixelRect;
class Canvas;

/**
 * A multi-select widget for file picking from FileDataField.
 * Shows available files with checkmarks and handles file rendering.
 */
class FileMultiSelectWidget : public MultiSelectListWidget {
public:
  struct FileItem {
    Path path;
  };

  using TextProvider = std::function<const char*(const FileItem&)>;

  FileMultiSelectWidget(MultiFileDataField &df,
                        TextProvider first_left_provider,
                        const char *caption,
                        const char *help_text) noexcept
    : df_(df), first_left_provider_(std::move(first_left_provider)),
      caption_(caption), help_text_(help_text) {}

  void ShowHelp() noexcept;

  /**
    * Set a callback invoked when the selection changes. The callback is
    * copied; do not capture references to stack variables.
    */
  void SetSelectionChangedCallback(std::function<void()> cb) noexcept;

  /* Provider setters for optional rendering text */
  void SetFirstRightProvider(TextProvider p) noexcept { first_right_provider_ = std::move(p); }
  void SetSecondLeftProvider(TextProvider p) noexcept { second_left_provider_ = std::move(p); }
  void SetSecondRightProvider(TextProvider p) noexcept { second_right_provider_ = std::move(p); }

  [[nodiscard]]
  std::vector<Path> GetSelectedPaths() const noexcept;

  void Refresh() noexcept;

  // MultiSelectListWidget virtual methods
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override;
  unsigned OnListResized() noexcept override;

  void OnSelectionChanged() noexcept override;

protected:
  unsigned ComputeRowHeight() noexcept;

private:
  std::vector<FileItem> items_;
  MultiFileDataField &df_;
  TwoTextRowsRenderer two_text_rows_renderer_;
  TextRowRenderer text_row_renderer_;

  TextProvider first_left_provider_;
  TextProvider first_right_provider_;
  TextProvider second_left_provider_;
  TextProvider second_right_provider_;

  bool use_two_rows_ = false;
  bool refreshed_ = false;
  const char *caption_ = nullptr;
  const char *help_text_ = nullptr;
  std::function<void()> selection_changed_callback_;

  void LoadFiles() noexcept;
  void ApplySelection(const std::vector<Path> &paths) noexcept;
  void MergePaths(const std::vector<Path> &paths) noexcept;
  void RestoreSelection(const std::vector<Path> &saved_selection,
                        const std::vector<FileItem> &previous_items,
                        const std::vector<Path> &current_items) noexcept;
};
