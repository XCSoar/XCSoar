from georect import GeoRect
from angle import Angle
from geopoint import GeoPoint

class Waypoint:
    def __init__(self):
        self.location = GeoPoint()
        self.altitude = 0
        self.name = ""

    def __str__(self):
        return str(self.name) + ", " + str(self.location) + ", " + str(self.altitude)

class WaypointList:
    def __init__(self):
        self.__list = []

    def len(self):
        return len(self.__list)

    def entry(self, index):
        if index < 0 or index > self.len():
            return None

        return self.__list[index]

    def entries(self):
        return self.__list

    def get_bounds(self):
        rc = GeoRect()
        rc.left = Angle.degrees(180)
        rc.right = Angle.degrees(-180)
        rc.top = Angle.degrees(-90)
        rc.bottom = Angle.degrees(90)
        for wp in self.__list:
            rc.left = min(rc.left, wp.location.lon)
            rc.right = max(rc.right, wp.location.lon)
            rc.top = max(rc.top, wp.location.lat)
            rc.bottom = min(rc.bottom, wp.location.lat)
        return rc

    def parse(self, lines, type = "WinPilot"):
        if type == "WinPilot":
            self.__parse_winpilot(lines)

    def __parse_winpilot(self, lines):
        for line in lines:
            line = line.strip()
            if line == "" or line.startswith("*"):
                continue

            fields = line.split(",")
            if len(fields) < 6:
                continue

            wp = Waypoint()
            wp.location.lat = self.__parse_winpilot_coordinate(fields[1]);
            wp.location.lon = self.__parse_winpilot_coordinate(fields[2]);
            wp.altitude = self.__parse_winpilot_altitude(fields[3]);
            wp.name = fields[5].strip();
            self.__list.append(wp)

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
        if str.endswith("s") or str.endswith("w"):
            negative = True;
            str = str.rstrip("sw")
        else:
            negative = False;
            str = str.rstrip("ne")

        str = str.split(":")
        if len(str) < 2:
            return None

        a = Angle.dms(int(str[0]), float(str[1]))
        if (negative):
            a.flip()

        return a
