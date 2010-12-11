from angle import Angle

class GeoRect:
    def __init__(self, left = 0, right = 0, top = 0, bottom = 0):
        if isinstance(left, Angle):
            self.left = left
        else:
            self.left = Angle.degrees(left)

        if isinstance(right, Angle):
            self.right = right
        else:
            self.right = Angle.degrees(right)

        if isinstance(top, Angle):
            self.top = top
        else:
            self.top = Angle.degrees(top)

        if isinstance(bottom, Angle):
            self.bottom = bottom
        else:
            self.bottom = Angle.degrees(bottom)

    def __str__(self):
        return ("L: " + str(self.left) + ", " +
                "R: " + str(self.right) + ", " +
                "T: " + str(self.top) + ", " +
                "B: " + str(self.bottom))

    def intersects(self, other):
        if (self.inside(other.top, other.left) or
            self.inside(other.top, other.right) or
            self.inside(other.bottom, other.left) or
            self.inside(other.bottom, other.right)):
            return True

        if (other.inside(self.top, self.left) or
            other.inside(self.top, self.right) or
            other.inside(self.bottom, self.left) or
            other.inside(self.bottom, self.right)):
            return True

        return False

    def inside(self, lat, lon):
        return (lat.between(self.bottom, self.top) and
                lon.between(self.left, self.right))
