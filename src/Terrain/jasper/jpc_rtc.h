#ifndef JPC_RTC_H
#define JPC_RTC_H

#include "Compiler.h"

struct jas_matrix;

#ifdef __cplusplus
extern "C" {
#endif

  gcc_const
  long jas_rtc_SkipMarkerSegment(long file_offset);
  void jas_rtc_MarkerSegment(long file_offset, unsigned id);

  void jas_rtc_SetTile(unsigned index,
		       int xstart, int ystart, int xend, int yend);

  void jas_rtc_PutTileData(unsigned index, unsigned x, unsigned y,
			   const struct jas_matrix *data);

  void jas_rtc_SetLatLonBounds(double lon_min, double lon_max, double lat_min, double lat_max);
  void jas_rtc_SetSize(unsigned width, unsigned height,
		       unsigned tile_width, unsigned tile_height,
		       unsigned tile_columns, unsigned tile_rows);

#ifdef __cplusplus
}
#endif

#endif
