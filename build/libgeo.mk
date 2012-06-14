# Build rules for the geo math library

GEO_SRC_DIR = $(SRC)/Geo

# note: Geoid.cpp is not included here, because it's glue code with
# global variables that needs explicit global initialisation

GEO_SOURCES := \
	$(GEO_SRC_DIR)/Math.cpp \
	$(GEO_SRC_DIR)/GeoPoint.cpp \
	$(GEO_SRC_DIR)/GeoVector.cpp \
	$(GEO_SRC_DIR)/GeoClip.cpp \
	$(GEO_SRC_DIR)/UTM.cpp

$(eval $(call link-library,libgeo,GEO))
