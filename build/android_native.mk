# This Makefile fragment builds the native library libxcsoar.so.
# We're not using NDK's Makefiles because our Makefile is so big and
# complex, we don't want to duplicate that for another platform.
# The actual Android APK, and architecture independent stuff is built
# in android.mk. 

ifeq ($(TARGET),ANDROID)


NATIVE_CLASSES := \
	NativeView \
	EventBridge \
	InternalGPS \
	NonGPSSensors \
	NativeInputListener \
	DownloadUtil \
	BatteryReceiver \
	GliderLinkReceiver \
	NativePortListener \
	NativeLeScanCallback \
	NativeBMP085Listener \
	NativeI2CbaroListener \
	NativeNunchuckListener \
	NativeVoltageListener
NATIVE_SOURCES = $(patsubst %,android/src/%.java,$(NATIVE_CLASSES))
# $(NATIVE_INCLUDE) comes from the command line of the sub-make call 
NATIVE_PREFIX = $(NATIVE_INCLUDE)/$(subst .,_,$(JAVA_PACKAGE))_
NATIVE_HEADERS = $(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES))

# add dependency to this source file
$(call SRC_TO_OBJ,$(SRC)/Android/Main.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/EventBridge.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/InternalSensors.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/Battery.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/GliderLink.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativePortListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeLeScanCallback.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeInputListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/DownloadManager.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeBMP085Listener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeI2CbaroListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeNunchuckListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeVoltageListener.cpp): $(NATIVE_HEADERS)

$(NATIVE_HEADERS): $(ANDROID_BUILD)/classes.dex

# Add the shared native headers directory to CPPFLAGS
CPPFLAGS += -I$(NATIVE_INCLUDE)

endif
