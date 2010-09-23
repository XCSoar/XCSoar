from angle import Angle

class GeoRect:
    def __init__(self,
                 left = Angle.degrees(0),
                 right = Angle.degrees(0),
                 top = Angle.degrees(0),
                 bottom = Angle.degrees(0)):
        self.left = left
        self.right = right
        self.top = top
        self.bottom = bottom

    def __str__(self):
        return ("L: " + str(self.left) + ", " +
                "R: " + str(self.right) + ", " +
                "T: " + str(self.top) + ", " +
                "B: " + str(self.bottom))
