#ifndef JPC_RTC_H
#define JPC_RTC_H

#include "Compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

  gcc_const
  long jas_rtc_SkipMarkerSegment(long file_offset);
  void jas_rtc_MarkerSegment(long file_offset, unsigned id);

  void jas_rtc_SetTile(unsigned index,
		       int xstart, int ystart, int xend, int yend);

  gcc_const
  bool jas_rtc_PollTiles(int viewx, int viewy);

  gcc_const
  short* jas_rtc_GetImageBuffer(unsigned index);
  void jas_rtc_SetLatLonBounds(double lon_min, double lon_max, double lat_min, double lat_max);
  void jas_rtc_SetSize(unsigned width, unsigned height,
		       unsigned tile_width, unsigned tile_height,
		       unsigned tile_columns, unsigned tile_rows);
  void jas_rtc_SetInitialised(bool val);

  gcc_const
  short* jas_rtc_GetOverview(void);

#ifdef __cplusplus
}
#endif

#endif
