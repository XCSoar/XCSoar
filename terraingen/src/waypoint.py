from geopoint import GeoPoint

class Waypoint:
    def __init__(self):
        self.location = GeoPoint()
        self.altitude = 0
        self.name = ""
        
    def __str__(self):
        return str(self.name) + ", " + str(self.location) + ", " + str(self.altitude)