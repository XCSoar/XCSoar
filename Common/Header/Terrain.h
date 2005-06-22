#ifndef TERRAIN_H
#define TERRAIN_H

void SetTopologyBounds(RECT rcin);
void ReadTopology();
void OpenTopology();
void CloseTopology();
void DrawTopology( HDC hdc, RECT rc);
void DrawTerrain(HDC hdc, RECT rc, double sunazimuth, double sunelevation);
void DrawMarks(HDC hdc, RECT rc);
void MarkLocation(double lon, double lat);
#endif
