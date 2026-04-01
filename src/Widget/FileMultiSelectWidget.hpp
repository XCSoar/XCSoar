// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MultiSelectListWidget.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "system/Path.hpp"
#include "util/StringAPI.hxx"
#include "ui/canvas/Icon.hpp"
#include <functional>
#include <vector>

class ContainerWindow;
struct PixelRect;
class Canvas;
class MultiFileDataField;

/**
 * A multi-select widget for file picking from FileDataField.
 * Shows available files with checkmarks and handles file rendering.
 */
class FileMultiSelectWidget : public MultiSelectListWidget {
public:
  struct FileItem {
    AllocatedPath path;
    bool is_dir = false;
    bool is_up = false;

    /** true if the file was found on disk during scanning */
    bool exists = true;

    /**
     * Comparator: ".." first, then directories, then files,
     * each group sorted by base name using locale collation.
     */
    static bool Compare(const FileItem &a, const FileItem &b) noexcept {
      if (a.is_up != b.is_up) return a.is_up;
      if (a.is_dir != b.is_dir) return a.is_dir;
      return StringCollate(a.path.GetBase().c_str(),
                           b.path.GetBase().c_str()) < 0;
    }
  };

  using TextProvider = std::function<const char*(const FileItem&)>;

  FileMultiSelectWidget(MultiFileDataField &df,
                        TextProvider first_left_provider,
                        const char *caption,
                        const char *help_text) noexcept
    : df_(&df), first_left_provider_(std::move(first_left_provider)),
      caption_(caption), help_text_(help_text) {}

  /**
   * Construct a file list widget from a custom loader. The loader returns
   * the set of items (files and directories) to display. This is useful for
   * callers that enumerate filesystem entries themselves.
   */
  FileMultiSelectWidget(std::function<std::vector<FileItem>()> loader,
                        TextProvider first_left_provider,
                        const char *caption,
                        const char *help_text) noexcept
    : df_(nullptr), loader_(std::move(loader)),
      first_left_provider_(std::move(first_left_provider)),
      caption_(caption), help_text_(help_text) {}

  void ShowHelp() noexcept;

  /**
    * Set a callback invoked when the selection changes. The callback is
    * copied; do not capture references to stack variables.
    */
  void SetSelectionChangedCallback(std::function<void()> cb) noexcept;

  /**
   * Set a callback invoked when a directory item is activated.
   */
  void SetNavigateCallback(std::function<void(AllocatedPath)> cb) noexcept;

  /* Provider setters for optional rendering text */
  void SetFirstRightProvider(TextProvider p) noexcept { first_right_provider_ = std::move(p); }
  void SetSecondLeftProvider(TextProvider p) noexcept { second_left_provider_ = std::move(p); }
  void SetSecondRightProvider(TextProvider p) noexcept { second_right_provider_ = std::move(p); }

  [[nodiscard]]
  std::vector<Path> GetSelectedPaths() const noexcept;

  [[nodiscard]]
  std::vector<Path> GetAllPaths() const noexcept;

  void Refresh() noexcept;

  /** Select all file items (exclude directories). */
  void SelectAllFiles() noexcept;

  /** Set an optional filter for which files to show. If `filter` is
    default-constructed or empty, all files are shown. */
  void SetFilter(std::function<bool(const Path &)> filter) noexcept;

  // MultiSelectListWidget virtual methods
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override;
  unsigned OnListResized() noexcept override;

  void OnSelectionChanged() noexcept override;

protected:
  void OnActivateItem(unsigned index) noexcept override;
  unsigned ComputeRowHeight() noexcept;

private:
  std::vector<FileItem> items_;
  MultiFileDataField *df_ = nullptr;
  TwoTextRowsRenderer two_text_rows_renderer_;
  TextRowRenderer text_row_renderer_;
  std::function<std::vector<FileItem>()> loader_;

  TextProvider first_left_provider_;
  TextProvider first_right_provider_;
  TextProvider second_left_provider_;
  TextProvider second_right_provider_;

  bool use_two_rows_ = false;
  bool refreshed_ = false;
  const char *caption_ = nullptr;
  const char *help_text_ = nullptr;
  std::function<void()> selection_changed_callback_;
  std::function<bool(const Path &)> filter_;
  std::function<void(AllocatedPath)> navigate_callback_;
  MaskedIcon folder_icon_;

  void LoadFiles() noexcept;
  std::vector<Path> GetCurrentItems() const noexcept;
  void ApplySelection(const std::vector<Path> &paths) noexcept;
  void MergePaths(const std::vector<Path> &paths) noexcept;
  void MergePreviousItems(std::vector<FileItem> &previous_items) noexcept;
  void RestoreAfterRefresh(const std::vector<Path> &saved_selection,
                           const std::vector<AllocatedPath> &previous_paths,
                           const std::vector<Path> &current_items) noexcept;
  void PaintDirectoryItem(Canvas &canvas, PixelRect rc,
                          const FileItem &item) noexcept;
  void PaintFileItem(Canvas &canvas, PixelRect rc,
                     unsigned idx, const FileItem &item) noexcept;
  void ActivateDirectoryItem(const FileItem &item) noexcept;
  void RestoreSelection(const std::vector<Path> &saved_selection,
                        const std::vector<AllocatedPath> &previous_items_paths,
                        const std::vector<Path> &current_items) noexcept;
};
