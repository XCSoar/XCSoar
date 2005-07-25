#if !defined(AFX_AIRSPACE_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_AIRSPACE_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


void ReadAirspace(void);
int FindAirspaceCircle(double Longditude,double Lattitude);
int FindAirspaceArea(double Longditude,double Lattitude);
BOOL CheckAirspaceAltitude(double Base, double Top);
void CloseAirspace(void);

#endif
