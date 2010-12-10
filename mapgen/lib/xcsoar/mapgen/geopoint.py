class GeoPoint:
    def __init__(self, lon, lat):
        self.lon = lon
        self.lat = lat

    def __str__(self):
        return str(self.lat) + ", " + str(self.lon)
