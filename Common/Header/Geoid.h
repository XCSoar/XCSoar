#ifndef GEOID_H
#define GEOID_H

void OpenGeoid(void);
void CloseGeoid(void);
double LookupGeoidSeparation(double lat, double lon);

#endif
