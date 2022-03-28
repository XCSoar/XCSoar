# Build rules for the DataField library

DATA_FIELD_SRC_DIR = $(SRC)/Form/DataField

DATA_FIELD_SOURCES = \
	$(DATA_FIELD_SRC_DIR)/Base.cpp \
	$(DATA_FIELD_SRC_DIR)/Boolean.cpp \
	$(DATA_FIELD_SRC_DIR)/ComboList.cpp \
	$(DATA_FIELD_SRC_DIR)/Enum.cpp \
	$(DATA_FIELD_SRC_DIR)/File.cpp \
	$(DATA_FIELD_SRC_DIR)/Number.cpp \
	$(DATA_FIELD_SRC_DIR)/Float.cpp \
	$(DATA_FIELD_SRC_DIR)/Angle.cpp \
	$(DATA_FIELD_SRC_DIR)/GeoPoint.cpp \
	$(DATA_FIELD_SRC_DIR)/RoughTime.cpp \
	$(DATA_FIELD_SRC_DIR)/Time.cpp \
	$(DATA_FIELD_SRC_DIR)/Integer.cpp \
	$(DATA_FIELD_SRC_DIR)/String.cpp \
	$(DATA_FIELD_SRC_DIR)/Prefix.cpp \
	$(DATA_FIELD_SRC_DIR)/Date.cpp \
	$(DATA_FIELD_SRC_DIR)/TextNumPadAdapter.cpp \
	$(DATA_FIELD_SRC_DIR)/NumPadWidgetInterface.cpp \
	$(DATA_FIELD_SRC_DIR)/NumPadAdapter.cpp \
	$(DATA_FIELD_SRC_DIR)/Password.cpp

DATA_FIELD_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)


$(eval $(call link-library,datafield,DATA_FIELD))
