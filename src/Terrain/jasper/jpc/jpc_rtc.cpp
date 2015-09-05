#include "jasper/jpc_rtc.h"
#include "Terrain/Loader.hpp"

thread_local TerrainLoader *current_terrain_loader = 0;

extern "C" {

  long jas_rtc_SkipMarkerSegment(long file_offset) {
    return current_terrain_loader->SkipMarkerSegment(file_offset);
  }

  void jas_rtc_MarkerSegment(long file_offset, unsigned id) {
    return current_terrain_loader->MarkerSegment(file_offset, id);
  }

  void jas_rtc_ProcessComment(const char *data, unsigned size) {
    return current_terrain_loader->ProcessComment(data, size);
  }

  void jas_rtc_StartTile(unsigned index) {
    current_terrain_loader->StartTile(index);
  }

  void jas_rtc_PutTileData(unsigned index,
                           unsigned start_x, unsigned start_y,
                           unsigned end_x, unsigned end_y,
                           const struct jas_matrix *data) {
    current_terrain_loader->PutTileData(index, start_x, start_y, end_x, end_y,
                                     *data);
  }

  void jas_rtc_SetSize(unsigned width, unsigned height,
                       unsigned tile_width, unsigned tile_height,
                       unsigned tile_columns, unsigned tile_rows) {
    current_terrain_loader->SetSize(width, height,
                                 tile_width, tile_height,
                                 tile_columns, tile_rows);
  }
};
