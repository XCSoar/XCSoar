from xcsoar.mapgen.georect import GeoRect
from xcsoar.mapgen.geopoint import GeoPoint

class Waypoint(GeoPoint):
    def __init__(self):
        self.altitude = 0
        self.name = ""

    def __str__(self):
        return str(self.name) + ", " + super() + ", " + str(self.altitude)

class WaypointList:
    def __init__(self):
        self.__list = []

    def __len__(self):
        return len(self.__list)

    def __getitem__(self, i):
        if i < 0 or i > len(self):
            return None
        return self.__list[i]

    def __iter__(self):
        return iter(self.__list)

    def append(self, wp):
        self.__list.append(wp)

    def get_bounds(self):
        rc = GeoRect(180, -180, -90, 90)
        for wp in self.__list:
            rc.left = min(rc.left, wp.lon)
            rc.right = max(rc.right, wp.lon)
            rc.top = max(rc.top, wp.lat)
            rc.bottom = min(rc.bottom, wp.lat)
        return rc

    def parse(self, lines, type = 'WinPilot'):
        if type == 'WinPilot':
            self.__parse_winpilot(lines)
        else:
            raise RuntimeError, 'Waypoint format ' + type + ' is not supported'

    def __parse_winpilot(self, lines):
        for line in lines:
            line = line.strip()
            if line == "" or line.startswith("*"):
                continue

            fields = line.split(",")
            if len(fields) < 6:
                continue

            wp = Waypoint()
            wp.lat = self.__parse_winpilot_coordinate(fields[1]);
            wp.lon = self.__parse_winpilot_coordinate(fields[2]);
            wp.altitude = self.__parse_winpilot_altitude(fields[3]);
            wp.name = fields[5].strip();
            self.append(wp)

    def __parse_winpilot_altitude(self, str):
        str = str.lower()
        if str.endswith("ft") or str.endswith("f"):
            str = str.rstrip("ft")
            return int(str) * 0.3048
        else:
            str = str.rstrip("m")
            return int(str)

    def __parse_winpilot_coordinate(self, str):
        str = str.lower()
        negative = str.endswith("s") or str.endswith("w")
        str = str.rstrip("sw") if negative else str.rstrip("ne")

        str = str.split(":")
        if len(str) < 2:
            return None

        # degrees + minutes / 60
        a = int(str[0]) + float(str[1]) / 60
        if (negative):
            a *= -1
        return a
