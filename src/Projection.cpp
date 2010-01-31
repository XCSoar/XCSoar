#include "Projection.hpp"
#include "Math/Geometry.hpp"

Projection::Projection():
  DisplayAngle (fixed_zero),
  m_scale_meters_to_screen (fixed_zero)
{
  PanLocation.Latitude = 0.0;
  PanLocation.Longitude = 0.0;
}

/**
 * Converts screen coordinates to a GEOPOINT
 * @param x x-Coordinate on the screen
 * @param y y-Coordinate on the screen
 * @param g Output GEOPOINT
 */
void 
Projection::Screen2LonLat(const int &x,
                          const int &y,
                          GEOPOINT &g) const
{
  const FastIntegerRotation::Pair p =
    DisplayAngle.Rotate(x - Orig_Screen.x, y - Orig_Screen.y);
  g.Latitude = PanLocation.Latitude - p.second * InvDrawScale;
  g.Longitude = PanLocation.Longitude + p.first * invfastcosine(g.Latitude)
    * InvDrawScale;
}

/**
 * Converts a GEOPOINT to screen coordinates
 * @param g GEOPOINT to convert
 * @param sc Output screen coordinate
 */
void
Projection::LonLat2Screen(const GEOPOINT &g,
                          POINT &sc) const
{
  const GEOPOINT d = PanLocation-g;
  const FastIntegerRotation::Pair p =
    DisplayAngle.Rotate((int)(d.Longitude * fastcosine(g.Latitude)
                              * DrawScale),
                        (int)(d.Latitude * DrawScale));

  sc.x = Orig_Screen.x - p.first;
  sc.y = Orig_Screen.y + p.second;
}

/**
 * Converts a LatLon-based polygon to screen coordinates
 *
 * This one is optimised for long polygons.
 * @param ptin Input polygon
 * @param ptout Output polygon
 * @param n Number of points in the polygon
 * @param skip Number of corners to skip after a successful conversion
 */
void
Projection::LonLat2Screen(const GEOPOINT *ptin, POINT *ptout,
                          unsigned n, unsigned skip) const
{
  static fixed lastangle(-1);
  static int cost=1024, sint=0;
  const fixed mDisplayAngle(GetDisplayAngle());

  if (mDisplayAngle != lastangle) {
    lastangle = mDisplayAngle;
    int deg = DEG_TO_INT(AngleLimit360(mDisplayAngle));
    cost = ICOSTABLE[deg];
    sint = ISINETABLE[deg];
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const fixed mDrawScale = DrawScale;
  const GEOPOINT mPan = PanLocation;
  const GEOPOINT *p = ptin;
  const GEOPOINT *ptend = ptin + n;

  while (p<ptend) {
    int Y = Real2Int((mPan.Latitude - p->Latitude) * mDrawScale);
    int X = Real2Int((mPan.Longitude - p->Longitude) *
                     fastcosine(p->Latitude) * mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}

/**
 * Converts a LatLon-based polygon to screen coordinates
 *
 * This one is optimised for long polygons.
 * @param ptin Input polygon
 * @param ptout Output polygon
 * @param n Number of points in the polygon
 * @param skip Number of corners to skip after a successful conversion
 */
void
Projection::LonLat2Screen(const pointObj* const ptin,
                          POINT *ptout,
                          const int n,
                          const int skip) const
{
  static fixed lastangle(-1);
  static int cost=1024, sint=0;
  const fixed mDisplayAngle(GetDisplayAngle());

  if(mDisplayAngle != lastangle) {
    lastangle = mDisplayAngle;
    int deg = DEG_TO_INT(AngleLimit360(mDisplayAngle));
    cost = ICOSTABLE[deg];
    sint = ISINETABLE[deg];
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const fixed mDrawScale = DrawScale;
  const fixed mPanLongitude = PanLocation.Longitude;
  const fixed mPanLatitude = PanLocation.Latitude;
  pointObj const * p = ptin;
  const pointObj* ptend = ptin+n;

  while (p<ptend) {
    int Y = Real2Int((mPanLatitude-p->y)*mDrawScale);
    int X = Real2Int((mPanLongitude-p->x)*fastcosine(p->y)*mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}

bool
Projection::LonLatVisible(const GEOPOINT &loc) const
{
  if ((loc.Longitude> screenbounds_latlon.minx) &&
      (loc.Longitude< screenbounds_latlon.maxx) &&
      (loc.Latitude> screenbounds_latlon.miny) &&
      (loc.Latitude< screenbounds_latlon.maxy))
    return true;
  else
    return false;
}

bool
Projection::LonLat2ScreenIfVisible(const GEOPOINT &loc,
                                   POINT *sc) const
{
  if (LonLatVisible(loc)) {
    LonLat2Screen(loc, *sc);
    return PointVisible(*sc);
  } else {
    return false;
  }
}


bool
Projection::PointVisible(const POINT &P) const
{
  if(( P.x >= MapRect.left )
     &&
     ( P.x <= MapRect.right )
     &&
     ( P.y >= MapRect.top  )
     &&
     ( P.y <= MapRect.bottom  )
     )
    return true;
  else
    return false;
}


void 
Projection::SetScaleMetersToScreen(const fixed scale_meters_to_screen)
{
  m_scale_meters_to_screen = scale_meters_to_screen;
  DrawScale = 111194*m_scale_meters_to_screen;
  InvDrawScale = 1.0/DrawScale;
}

void
Projection::UpdateScreenBounds() 
{
  screenbounds_latlon = CalculateScreenBounds(fixed_zero);
}


rectObj
Projection::CalculateScreenBounds(const fixed scale) const
{
  // compute lat lon extents of visible screen
  rectObj sb;

  if (scale>= 1.0) {
    POINT screen_center;
    LonLat2Screen(PanLocation,screen_center);

    sb.minx = sb.maxx = PanLocation.Longitude;
    sb.miny = sb.maxy = PanLocation.Latitude;

    int dx, dy;
    unsigned int maxsc=0;
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));

    for (int i=0; i<10; i++) {
      double ang = i*360.0/10;
      POINT p;
      GEOPOINT g;
      p.x = screen_center.x + iround(fastcosine(ang)*maxsc*scale);
      p.y = screen_center.y + iround(fastsine(ang)*maxsc*scale);
      Screen2LonLat(p.x, p.y, g);
      sb.minx = min((double)g.Longitude, sb.minx);
      sb.miny = min((double)g.Latitude, sb.miny);
      sb.maxx = max((double)g.Longitude, sb.maxx);
      sb.maxy = max((double)g.Latitude, sb.maxy);
    }

  } else {

    double xmin, xmax, ymin, ymax;
    int x, y;
    GEOPOINT g;

    x = MapRect.left;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = g.Longitude; xmax = g.Longitude;
    ymin = g.Latitude; ymax = g.Latitude;

    x = MapRect.right;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, (double)g.Longitude);
    xmax = max(xmax, (double)g.Longitude);
    ymin = min(ymin, (double)g.Latitude);
    ymax = max(ymax, (double)g.Latitude);

    x = MapRect.right;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, (double)g.Longitude);
    xmax = max(xmax, (double)g.Longitude);
    ymin = min(ymin, (double)g.Latitude);
    ymax = max(ymax, (double)g.Latitude);

    x = MapRect.left;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, (double)g.Longitude);
    xmax = max(xmax, (double)g.Longitude);
    ymin = min(ymin, (double)g.Latitude);
    ymax = max(ymax, (double)g.Latitude);

    sb.minx = xmin;
    sb.maxx = xmax;
    sb.miny = ymin;
    sb.maxy = ymax;

  }

  return sb;
}


/*
  
  set PanLocation to latlon center 
  set MapRect
  Orig_Screen.x = (rc.left + rc.right)/2;
  Orig_Screen.y = (rc.bottom + rc.top)/2;
  UpdateScreenBounds();
*/
