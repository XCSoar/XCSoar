# Build rules for the profile library

PROFILE_SOURCES = \
	$(SRC)/Profile/File.cpp \
	$(SRC)/Profile/Current.cpp \
	$(SRC)/Profile/Map.cpp \
	$(SRC)/Profile/StringValue.cpp \
	$(SRC)/Profile/NumericValue.cpp \
	$(SRC)/Profile/PathValue.cpp \
	$(SRC)/Profile/GeoValue.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Profile/ProfileMap.cpp

$(eval $(call link-library,profile,PROFILE))
