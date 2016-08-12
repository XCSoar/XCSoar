# Build rules for the geo math library

GEO_SRC_DIR = $(SRC)/Geo

# note: Geoid.cpp is not included here, because it's glue code with
# global variables that needs explicit global initialisation

GEO_SOURCES := \
	$(GEO_SRC_DIR)/Boost/RangeBox.cpp \
	$(GEO_SRC_DIR)/ConvexHull/GrahamScan.cpp \
	$(GEO_SRC_DIR)/ConvexHull/PolygonInterior.cpp \
	$(GEO_SRC_DIR)/Memento/DistanceMemento.cpp \
	$(GEO_SRC_DIR)/Memento/GeoVectorMemento.cpp \
	$(GEO_SRC_DIR)/Flat/FlatProjection.cpp \
	$(GEO_SRC_DIR)/Flat/TaskProjection.cpp \
	$(GEO_SRC_DIR)/Flat/FlatBoundingBox.cpp \
	$(GEO_SRC_DIR)/Flat/FlatGeoPoint.cpp \
	$(GEO_SRC_DIR)/Flat/FlatRay.cpp \
	$(GEO_SRC_DIR)/Flat/FlatPoint.cpp \
	$(GEO_SRC_DIR)/Flat/FlatEllipse.cpp \
	$(GEO_SRC_DIR)/Flat/FlatLine.cpp \
	$(GEO_SRC_DIR)/Math.cpp \
	$(GEO_SRC_DIR)/SimplifiedMath.cpp \
	$(GEO_SRC_DIR)/Quadrilateral.cpp \
	$(GEO_SRC_DIR)/GeoPoint.cpp \
	$(GEO_SRC_DIR)/GeoVector.cpp \
	$(GEO_SRC_DIR)/GeoBounds.cpp \
	$(GEO_SRC_DIR)/GeoClip.cpp \
	$(GEO_SRC_DIR)/Quadrilateral.cpp \
	$(GEO_SRC_DIR)/SearchPoint.cpp \
	$(GEO_SRC_DIR)/SearchPointVector.cpp \
	$(GEO_SRC_DIR)/GeoEllipse.cpp \
	$(GEO_SRC_DIR)/UTM.cpp

$(eval $(call link-library,libgeo,GEO))
