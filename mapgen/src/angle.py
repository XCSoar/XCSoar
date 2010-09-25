import math

class Angle:
    def __init__(self, value = 0):
        self.__value = value

    @staticmethod
    def degrees(value):
        return Angle(value)
    @staticmethod
    def radians(value):
        return Angle(value / math.pi * 180)
    @staticmethod
    def dms(degrees, minutes, seconds = 0):
        return Angle(degrees + minutes / 60 + seconds / 3600)

    def set_degrees(self, value):
        self.__value = value
    def set_radians(self, value):
        self.__value = value / math.pi * 180;

    def value_degrees(self):
        return self.__value
    def value_radians(self):
        return self.__value / 180 * math.pi;

    def limit_360(self):
        retval = Angle(self.__value)
        while (retval.value_degrees() < 0):
            retval.set_degrees(retval.value_degrees() + 360)
        while (retval.value_degrees() >= 360):
            retval.set_degrees(retval.value_degrees() - 360)
        return retval

    def limit_180(self):
        retval = Angle(self.__value)
        while (retval.value_degrees() < -180):
            retval.set_degrees(retval.value_degrees() + 360)
        while (retval.value_degrees() >= 180):
            retval.set_degrees(retval.value_degrees() - 360)
        return retval

    def between(self, start, end):
        width = (end - start).limit_360().value_degrees();
        delta = (self - start).limit_360().value_degrees();
        return delta <= width;

    def flip(self):
        self.__value *= -1

    def __cmp__(self, other):
        if self.value_degrees() < other.value_degrees():
            return -1
        elif self.value_degrees() > other.value_degrees():
            return 1
        else:
            return 0

    def __str__(self):
        return str(self.__value)
    
    def __sub__(self, other):
        return Angle.degrees(self.value_degrees() - other.value_degrees())
