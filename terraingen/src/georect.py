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
            