// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MultiSelectListWidget.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Renderer/TextRowRenderer.hpp"
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

  FileMultiSelectWidget(MultiFileDataField &df,
                        const TCHAR *caption = nullptr,
                        const TCHAR *help_text = nullptr) noexcept
    : df_(df), caption_(caption), help_text_(help_text) {}

  void ShowHelp() noexcept;

  [[nodiscard]]
  std::vector<Path> GetSelectedPaths() const noexcept;

  void Refresh() noexcept;

  // MultiSelectListWidget virtual methods
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override;
  unsigned OnListResized() noexcept override;

private:
  std::vector<FileItem> items_;
  MultiFileDataField &df_;
  TextRowRenderer row_renderer_;
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
