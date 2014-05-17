# This Makefile fragment builds the Android package (XCSoar.apk).
# We're not using NDK's Makefiles because our Makefile is so big and
# complex, we don't want to duplicate that for another platform.

ifeq ($(TARGET),ANDROID)

ANDROID_KEYSTORE = $(HOME)/.android/mk.keystore
ANDROID_KEY_ALIAS = mk
ANDROID_BUILD = $(TARGET_OUTPUT_DIR)/build
ANDROID_BIN = $(TARGET_BIN_DIR)

ifeq ($(HOST_IS_DARWIN),y)
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-macosx
else
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-linux
endif
ANDROID_SDK_PLATFORM_DIR = $(ANDROID_SDK)/platforms/$(ANDROID_SDK_PLATFORM)
ANDROID_ABI_DIR = $(ANDROID_BUILD)/libs/$(ANDROID_ABI5)

ANDROID_LIB_NAMES = xcsoar

ifneq ($(V),2)
ANDROID_TOOL_OPTIONS = --silent
else
ANDROID_TOOL_OPTIONS = --verbose
endif

JARSIGNER += -digestalg SHA1 -sigalg MD5withRSA

# The environment variable ANDROID_KEYSTORE_PASS may be used to
# specify the keystore password; if you don't set it, you will be
# asked interactively
ifeq ($(origin ANDROID_KEYSTORE_PASS),environment)
JARSIGNER += -storepass:env ANDROID_KEYSTORE_PASS
endif

JAVA_PACKAGE = org.xcsoar
CLASS_NAME = $(JAVA_PACKAGE).NativeView
CLASS_SOURCE = $(subst .,/,$(CLASS_NAME)).java
CLASS_CLASS = $(patsubst %.java,%.class,$(CLASS_SOURCE))

NATIVE_CLASSES = NativeView EventBridge InternalGPS NonGPSSensors NativeInputListener DownloadUtil BatteryReceiver
NATIVE_CLASSES += NativeBMP085Listener
NATIVE_CLASSES += NativeI2CbaroListener
NATIVE_CLASSES += NativeBaroListener
NATIVE_CLASSES += NativeNunchuckListener
NATIVE_CLASSES += NativeVoltageListener
NATIVE_SOURCES = $(patsubst %,android/src/%.java,$(NATIVE_CLASSES))
NATIVE_PREFIX = $(TARGET_OUTPUT_DIR)/include/$(subst .,_,$(JAVA_PACKAGE))_
NATIVE_HEADERS = $(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES))

ANDROID_JAVA_SOURCES := $(wildcard android/src/*.java)

DRAWABLE_DIR = $(ANDROID_BUILD)/res/drawable
RAW_DIR = $(ANDROID_BUILD)/res/raw

ifeq ($(TESTING),y)
ICON_SVG = $(topdir)/Data/graphics/logo_red.svg
else
ICON_SVG = $(topdir)/Data/graphics/logo.svg
endif

$(ANDROID_BUILD)/res/drawable-ldpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-ldpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(ANDROID_BUILD)/res/drawable/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(ANDROID_BUILD)/res/drawable-hdpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(ANDROID_BUILD)/res/drawable-xhdpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=96 $< -o $@

OGGENC = oggenc --quiet --quality 1

SOUNDS = fail insert remove beep_bweep beep_clear beep_drip
SOUND_FILES = $(patsubst %,$(RAW_DIR)/%.ogg,$(SOUNDS))

$(SOUND_FILES): $(RAW_DIR)/%.ogg: Data/sound/%.wav | $(RAW_DIR)/dirstamp
	@$(NQ)echo "  OGGENC  $@"
	$(Q)$(OGGENC) -o $@ $<

PNG1 := $(patsubst Data/bitmaps/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_BITMAPS))

# workaround for an ImageMagick bug (observed with the Debian package
# 8:6.7.7.10-2): it corrupts 4-bit gray-scale images when converting
# BMP to PNG (TRAC #2220)
PNG1b := $(filter $(DRAWABLE_DIR)/vario_scale_%.png,$(PNG1))
PNG1 := $(filter-out $(DRAWABLE_DIR)/vario_scale_%.png,$(PNG1))
$(DRAWABLE_DIR)/vario_scale_%.png: Data/bitmaps/vario_scale_%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert -depth 8 $< $@

$(PNG1): $(DRAWABLE_DIR)/%.png: Data/bitmaps/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG2 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_LAUNCH_ALL))
$(PNG2): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG3 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_SPLASH_80) $(BMP_SPLASH_160) $(BMP_TITLE_110) $(BMP_TITLE_320))
$(PNG3): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG4 := $(patsubst $(DATA)/icons/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_ICONS) $(BMP_ICONS_160))
$(PNG4): $(DRAWABLE_DIR)/%.png: $(DATA)/icons/%.png | $(DRAWABLE_DIR)/dirstamp
	$(Q)cp $< $@

PNG5 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_DIALOG_TITLE) $(BMP_PROGRESS_BORDER))
$(PNG5): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG_FILES = $(PNG1) $(PNG1b) $(PNG2) $(PNG3) $(PNG4) $(PNG5) \
	$(ANDROID_BUILD)/res/drawable-ldpi/icon.png \
	$(ANDROID_BUILD)/res/drawable/icon.png \
	$(ANDROID_BUILD)/res/drawable-hdpi/icon.png \
	$(ANDROID_BUILD)/res/drawable-xhdpi/icon.png

ifeq ($(TESTING),y)
MANIFEST = android/testing/AndroidManifest.xml
else
MANIFEST = android/AndroidManifest.xml
endif

# symlink some important files to $(ANDROID_BUILD) and let the Android
# SDK generate build.xml
$(ANDROID_BUILD)/build.xml: $(MANIFEST) $(PNG_FILES) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  ANDROID $@"
	$(Q)rm -r -f $@ $(@D)/*_rules.xml $(@D)/AndroidManifest.xml $(@D)/src $(@D)/bin $(@D)/res/values $(@D)/res/xml
	$(Q)mkdir -p $(ANDROID_BUILD)/res $(ANDROID_BUILD)/src/org/xcsoar $(ANDROID_BUILD)/src/ioio/lib/android
	$(Q)ln -s ../../../$(MANIFEST) $(@D)/AndroidManifest.xml
	$(Q)ln -s ../bin $(@D)/bin
	$(Q)ln -s $(addprefix ../../../../../../,$(ANDROID_JAVA_SOURCES)) $(@D)/src/org/xcsoar
	$(Q)ln -s ../../../../../../android/ioio/software/IOIOLib/src/ioio/lib/api $(ANDROID_BUILD)/src/ioio/lib/api
	$(Q)ln -s ../../../../../../android/ioio/software/IOIOLib/src/ioio/lib/spi $(ANDROID_BUILD)/src/ioio/lib/spi
	$(Q)ln -s ../../../../../../android/ioio/software/IOIOLib/src/ioio/lib/util $(ANDROID_BUILD)/src/ioio/lib/util
	$(Q)ln -s ../../../../../../android/ioio/software/IOIOLib/src/ioio/lib/impl $(ANDROID_BUILD)/src/ioio/lib/impl
	$(Q)ln -s ../../../../../../android/ioio/software/IOIOLib/target/android/src/ioio/lib/spi $(ANDROID_BUILD)/src/ioio/lib/spi2
	$(Q)ln -s ../../../../../android/ioio/software/IOIOLib/target/android/src/ioio/lib/util/android/ContextWrapperDependent.java $(ANDROID_BUILD)/src/ioio/
	$(Q)ln -s ../../../../../../../android/ioio/software/IOIOLibAccessory/src/ioio/lib/android/accessory $(ANDROID_BUILD)/src/ioio/lib/android/accessory
	$(Q)ln -s ../../../../android/res/values $(@D)/res/values
	$(Q)ln -s ../../../../android/res/xml $(@D)/res/xml
ifeq ($(HOST_IS_WIN32),y)
	echo "now run your android build followed by exit.  For example:"
	echo "c:\opt\android-sdk\tools\android.bat update project --path c:\xcsoar\output\android\build --target $(ANDROID_SDK_PLATFORM)"
	cmd
else
	$(Q)$(ANDROID_SDK)/tools/android $(ANDROID_TOOL_OPTIONS) update project --path $(@D) --target $(ANDROID_SDK_PLATFORM)
	$(Q)ln -s ../../../android/custom_rules.xml $(@D)/
endif
ifeq ($(TESTING),y)
	$(Q)ln -s ../../../../../../android/src/testing $(@D)/src/org/xcsoar
	$(Q)ln -s ../../../android/testing/testing_rules.xml $(@D)/
endif
	@touch $@

ifeq ($(FAT_BINARY),y)

# generate a "fat" APK file with binaries for all ABIs

ANDROID_LIB_BUILD =

# Example: $(eval $(call generate-abi,xcsoar,armeabi-v7a,ANDROID7))
define generate-abi

ANDROID_LIB_BUILD += $$(ANDROID_BUILD)/libs/$(2)/lib$(1).so

$$(ANDROID_BUILD)/libs/$(2)/lib$(1).so: $$(OUT)/$(3)/bin/lib$(1).so | $$(ANDROID_BUILD)/libs/$(2)/dirstamp
	$$(Q)cp $$< $$@

$$(OUT)/$(3)/bin/lib$(1).so:
	$$(Q)$$(MAKE) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) $$@

endef

# Example: $(eval $(call generate-abi,xcsoar))
define generate-all-abis
$(eval $(call generate-abi,$(1),armeabi,ANDROID))
$(eval $(call generate-abi,$(1),armeabi-v7a,ANDROID7))
$(eval $(call generate-abi,$(1),x86,ANDROID86))
$(eval $(call generate-abi,$(1),mips,ANDROIDMIPS))
endef

$(foreach NAME,$(ANDROID_LIB_NAMES),$(eval $(call generate-all-abis,$(NAME))))

else # !FAT_BINARY

# add dependency to this source file
$(call SRC_TO_OBJ,$(SRC)/Android/Main.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/EventBridge.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/InternalSensors.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/Battery.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeInputListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/DownloadManager.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeBMP085Listener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeI2CbaroListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeBaroListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeNunchuckListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeVoltageListener.cpp): $(NATIVE_HEADERS)

ANDROID_LIB_BUILD = $(patsubst %,$(ANDROID_ABI_DIR)/lib%.so,$(ANDROID_LIB_NAMES))
$(ANDROID_LIB_BUILD): $(ANDROID_ABI_DIR)/lib%.so: $(TARGET_BIN_DIR)/lib%.so $(ANDROID_ABI_DIR)/dirstamp
	$(Q)cp $< $@

$(ANDROID_BUILD)/bin/classes/$(CLASS_CLASS): $(NATIVE_SOURCES) $(ANDROID_BUILD)/build.xml $(ANDROID_BUILD)/res/drawable/icon.png $(SOUND_FILES)
	@$(NQ)echo "  ANT     $@"
	$(Q)cd $(ANDROID_BUILD) && $(ANT) nodeps compile-jni-classes
	@touch $@

$(NATIVE_HEADERS): $(NATIVE_PREFIX)%.h: android/src/%.java $(ANDROID_BUILD)/bin/classes/$(CLASS_CLASS)
	@$(NQ)echo "  JAVAH   $@"
	$(Q)javah -classpath $(ANDROID_SDK_PLATFORM_DIR)/android.jar:$(ANDROID_BUILD)/bin/classes -d $(@D) $(subst _,.,$(patsubst $(patsubst ./%,%,$(TARGET_OUTPUT_DIR))/include/%.h,%,$@))
	@touch $@

endif # !FAT_BINARY

$(ANDROID_BIN)/XCSoar-debug.apk: $(ANDROID_LIB_BUILD) $(ANDROID_BUILD)/build.xml $(ANDROID_BUILD)/res/drawable/icon.png $(SOUND_FILES) $(ANDROID_JAVA_SOURCES)
	@$(NQ)echo "  ANT     $@"
	@rm -f $@ $(patsubst %.apk,%-unaligned.apk,$@) $(@D)/classes.dex
	$(Q)cd $(ANDROID_BUILD) && $(ANT) nodeps debug

$(ANDROID_BIN)/XCSoar-release-unsigned.apk: $(ANDROID_LIB_BUILD) $(ANDROID_BUILD)/build.xml $(ANDROID_BUILD)/res/drawable/icon.png $(SOUND_FILES) $(ANDROID_JAVA_SOURCES)
	@$(NQ)echo "  ANT     $@"
	@rm -f $@ $(@D)/classes.dex
	$(Q)cd $(ANDROID_BUILD) && $(ANT) nodeps release

$(ANDROID_BIN)/XCSoar-release-unaligned.apk: $(ANDROID_BIN)/XCSoar-release-unsigned.apk
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(JARSIGNER) -keystore $(ANDROID_KEYSTORE) -signedjar $@ $< $(ANDROID_KEY_ALIAS)

$(ANDROID_BIN)/XCSoar.apk: $(ANDROID_BIN)/XCSoar-release-unaligned.apk
	@$(NQ)echo "  ALIGN   $@"
	$(Q)$(ANDROID_SDK)/tools/zipalign -f 4 $< $@

endif
