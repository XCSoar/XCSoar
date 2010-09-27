#ifndef JPC_RTC_H
#define JPC_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

  long jas_rtc_SkipMarkerSegment(long file_offset);
  void jas_rtc_MarkerSegment(long file_offset, unsigned id);

  void jas_rtc_SetTile(unsigned index,
		       int xstart, int ystart, int xend, int yend);
  bool jas_rtc_PollTiles(int viewx, int viewy);
  short* jas_rtc_GetImageBuffer(unsigned index);
  void jas_rtc_SetLatLonBounds(double lon_min, double lon_max, double lat_min, double lat_max);
  void jas_rtc_SetSize(unsigned width, unsigned height);
  void jas_rtc_SetInitialised(bool val);
  short* jas_rtc_GetOverview(void);

#ifdef __cplusplus
}
#endif

#endif
