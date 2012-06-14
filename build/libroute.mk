ROUTE_SRC_DIR = $(SRC)/Engine/Route

ROUTE_SOURCES = \
	$(ROUTE_SRC_DIR)/Config.cpp \
	$(ROUTE_SRC_DIR)/RoutePlanner.cpp \
	$(ROUTE_SRC_DIR)/AirspaceRoute.cpp \
	$(ROUTE_SRC_DIR)/TerrainRoute.cpp \
	$(ROUTE_SRC_DIR)/RouteLink.cpp \
	$(ROUTE_SRC_DIR)/RoutePolar.cpp \
	$(ROUTE_SRC_DIR)/RoutePolars.cpp \
	$(ROUTE_SRC_DIR)/FlatTriangleFan.cpp \
	$(ROUTE_SRC_DIR)/FlatTriangleFanTree.cpp \
	$(ROUTE_SRC_DIR)/ReachFan.cpp

$(eval $(call link-library,libroute,ROUTE))
