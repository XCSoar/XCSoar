GLIDE_SRC_DIR = $(SRC)/Engine/GlideSolvers

GLIDE_SOURCES = \
	$(GLIDE_SRC_DIR)/GlideSettings.cpp \
	$(GLIDE_SRC_DIR)/GlideState.cpp \
	$(GLIDE_SRC_DIR)/GlueGlideState.cpp \
	$(GLIDE_SRC_DIR)/GlidePolar.cpp \
	$(GLIDE_SRC_DIR)/PolarCoefficients.cpp \
	$(GLIDE_SRC_DIR)/GlideResult.cpp \
	$(GLIDE_SRC_DIR)/MacCready.cpp \
	$(GLIDE_SRC_DIR)/InstantSpeed.cpp

$(eval $(call link-library,libglide,GLIDE))
