WAYPOINT_SRC_DIR = $(SRC)/Engine/Waypoint

WAYPOINT_SOURCES = \
	$(WAYPOINT_SRC_DIR)/Waypoints.cpp \
	$(WAYPOINT_SRC_DIR)/Waypoint.cpp

WAYPOINT_DEPENDS = GEO UTIL

$(eval $(call link-library,libwaypoint,WAYPOINT))
