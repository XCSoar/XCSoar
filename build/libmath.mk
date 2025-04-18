# Build rules for the generic math library

MATH_SRC_DIR = $(SRC)/Math

MATH_SOURCES = \
	$(MATH_SRC_DIR)/Angle.cpp \
	$(MATH_SRC_DIR)/ARange.cpp \
	$(MATH_SRC_DIR)/FastMath.cpp \
	$(MATH_SRC_DIR)/FastTrig.cpp \
	$(MATH_SRC_DIR)/FastRotation.cpp \
	$(MATH_SRC_DIR)/LeastSquares.cpp \
	$(MATH_SRC_DIR)/DiffFilter.cpp \
	$(MATH_SRC_DIR)/Filter.cpp \
	$(MATH_SRC_DIR)/ZeroFinder.cpp \
	$(MATH_SRC_DIR)/KalmanFilter1d.cpp \
	$(MATH_SRC_DIR)/SelfTimingKalmanFilter1d.cpp \
	$(MATH_SRC_DIR)/XYDataStore.cpp \
	$(MATH_SRC_DIR)/ConvexFilter.cpp \
	$(MATH_SRC_DIR)/Histogram.cpp

$(eval $(call link-library,math,MATH))
