// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MultiSelectListWidget.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include <functional>
#include "system/Path.hpp"

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

  using TextProvider = std::function<const TCHAR*(const FileItem&)>;

  FileMultiSelectWidget(MultiFileDataField &df,
                        TextProvider first_left_provider,
                        const TCHAR *caption,
                        const TCHAR *help_text) noexcept
    : df_(df), first_left_provider_(std::move(first_left_provider)),
      caption_(caption), help_text_(help_text) {}

  void ShowHelp() noexcept;

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
  const TCHAR *caption_ = nullptr;
  const TCHAR *help_text_ = nullptr;

  void LoadFiles() noexcept;
  void ApplySelection(const std::vector<Path> &paths) noexcept;
  void MergePaths(const std::vector<Path> &paths) noexcept;
  void RestoreSelection(const std::vector<Path> &saved_selection,
                        const std::vector<FileItem> &previous_items,
                        const std::vector<Path> &current_items) noexcept;
};
