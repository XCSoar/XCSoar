from xcsoar.mapgen.angle import Angle

class GeoRect:
    def __init__(self, left = 0, right = 0, top = 0, bottom = 0):
        self.left = left if isinstance(left, Angle)       else Angle.degrees(left)
        self.right = right if isinstance(right, Angle)    else Angle.degrees(right)
        self.top = top if isinstance(top, Angle)          else Angle.degrees(top)
        self.bottom = bottom if isinstance(bottom, Angle) else Angle.degrees(bottom)

    def __str__(self):
        return ("L: " + str(self.left) + ", " +
                "R: " + str(self.right) + ", " +
                "T: " + str(self.top) + ", " +
                "B: " + str(self.bottom))

    def intersects(self, other):
        return (self.inside(other.top, other.left) or
                self.inside(other.top, other.right) or
                self.inside(other.bottom, other.left) or
                self.inside(other.bottom, other.right) or
                other.inside(self.top, self.left) or
                other.inside(self.top, self.right) or
                other.inside(self.bottom, self.left) or
                other.inside(self.bottom, self.right))

    def inside(self, lat, lon):
        return (lat.between(self.bottom, self.top) and
                lon.between(self.left, self.right))
