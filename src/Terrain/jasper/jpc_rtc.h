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
  void jas_rtc_ProcessComment(const char *data, unsigned size);
  void jas_rtc_StartTile(unsigned index);

  void jas_rtc_PutTileData(unsigned index,
			   unsigned start_x, unsigned start_y,
			   unsigned end_x, unsigned end_y,
			   const struct jas_matrix *data);

  void jas_rtc_SetSize(unsigned width, unsigned height,
		       unsigned tile_width, unsigned tile_height,
		       unsigned tile_columns, unsigned tile_rows);

#ifdef __cplusplus
}
#endif

#endif
