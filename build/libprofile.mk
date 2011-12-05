# Build rules for the profile library

PROFILE_SOURCES = \
	$(SRC)/Profile/ProfileMap.cpp

$(eval $(call link-library,profile,PROFILE))
