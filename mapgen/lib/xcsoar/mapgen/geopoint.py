from angle import Angle

class GeoPoint:
    def __init__(self,
                 lon = Angle.degrees(0),
                 lat = Angle.degrees(0)):
        self.lon = lon
        self.lat = lat

    def __str__(self):
        return str(self.lat) + ", " + str(self.lon)