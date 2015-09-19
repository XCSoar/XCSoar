#include "jasper/jpc_rtc.h"
#include "Terrain/Loader.hpp"

extern "C" {

  long jas_rtc_SkipMarkerSegment(void *_loader, long file_offset) {
    auto &loader = *(TerrainLoader *)_loader;
    return loader.SkipMarkerSegment(file_offset);
  }

  void jas_rtc_MarkerSegment(void *_loader, long file_offset, unsigned id) {
    auto &loader = *(TerrainLoader *)_loader;
    return loader.MarkerSegment(file_offset, id);
  }

  void jas_rtc_ProcessComment(void *_loader, const char *data, unsigned size) {
    auto &loader = *(TerrainLoader *)_loader;
    return loader.ProcessComment(data, size);
  }

  void jas_rtc_StartTile(void *_loader, unsigned index) {
    auto &loader = *(TerrainLoader *)_loader;
    loader.StartTile(index);
  }

  void jas_rtc_PutTileData(void *_loader,
                           unsigned index,
                           unsigned start_x, unsigned start_y,
                           unsigned end_x, unsigned end_y,
                           const struct jas_matrix *data) {
    auto &loader = *(TerrainLoader *)_loader;
    loader.PutTileData(index, start_x, start_y, end_x, end_y,
                                     *data);
  }

  void jas_rtc_SetSize(void *_loader,
                       unsigned width, unsigned height,
                       unsigned tile_width, unsigned tile_height,
                       unsigned tile_columns, unsigned tile_rows) {
    auto &loader = *(TerrainLoader *)_loader;
    loader.SetSize(width, height,
                   tile_width, tile_height,
                   tile_columns, tile_rows);
  }
};
