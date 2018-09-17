# This Makefile fragment builds the Android package (XCSoar.apk).
# We're not using NDK's Makefiles because our Makefile is so big and
# complex, we don't want to duplicate that for another platform.

ifeq ($(TARGET),ANDROID)

ANDROID_KEYSTORE ?= $(HOME)/.android/mk.keystore
ANDROID_KEY_ALIAS ?= mk

ANDROID_BUILD = $(TARGET_OUTPUT_DIR)/$(XCSOAR_ABI)/build
ANDROID_BIN = $(TARGET_BIN_DIR)

ifeq ($(HOST_IS_DARWIN),y)
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-macosx
else
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-linux
endif
ANDROID_SDK_PLATFORM_DIR = $(ANDROID_SDK)/platforms/$(ANDROID_SDK_PLATFORM)
ANDROID_ABI_DIR = $(ANDROID_BUILD)/lib/$(ANDROID_ABI5)

JAVA_CLASSFILES_DIR = $(ABI_BIN_DIR)/bin/classes

ANDROID_BUILD_TOOLS_DIR = $(ANDROID_SDK)/build-tools/28.0.3
ZIPALIGN = $(ANDROID_BUILD_TOOLS_DIR)/zipalign
AAPT = $(ANDROID_BUILD_TOOLS_DIR)/aapt
DX = $(ANDROID_BUILD_TOOLS_DIR)/dx

ANDROID_LIB_NAMES = xcsoar

JARSIGNER_RELEASE := $(JARSIGNER) -digestalg SHA1 -sigalg MD5withRSA

# The environment variable ANDROID_KEYSTORE_PASS may be used to
# specify the keystore password; if you don't set it, you will be
# asked interactively
ifeq ($(origin ANDROID_KEYSTORE_PASS),environment)
JARSIGNER_RELEASE += -storepass:env ANDROID_KEYSTORE_PASS
endif

JAVA_PACKAGE = org.xcsoar

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
NATIVE_INCLUDE = $(TARGET_OUTPUT_DIR)/include
NATIVE_PREFIX = $(NATIVE_INCLUDE)/$(subst .,_,$(JAVA_PACKAGE))_
NATIVE_HEADERS = $(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES))

JAVA_SOURCES := \
	$(wildcard android/src/*.java) \
	$(wildcard android/ioio/software/IOIOLib/src/ioio/lib/*/*.java) \
	$(wildcard android/ioio/software/IOIOLib/src/ioio/lib/*/*/*.java) \
	$(wildcard android/ioio/software/IOIOLib/target/android/src/ioio/lib/spi/*.java) \
	android/ioio/software/IOIOLib/target/android/src/ioio/lib/util/android/ContextWrapperDependent.java \
	$(wildcard android/ioio/software/IOIOLibAccessory/src/ioio/lib/android/accessory/*.java) \
	$(wildcard android/ioio/software/IOIOLibBT/src/ioio/lib/android/bluetooth/*.java) \
	$(wildcard android/ioio/software/IOIOLibAndroidDevice/src/ioio/lib/android/device/*.java)
ifeq ($(TESTING),y)
	JAVA_SOURCES += $(wildcard android/src/testing/*.java)
endif

ANDROID_XML_RES := $(wildcard android/res/*/*.xml)
ANDROID_XML_RES_COPIES := $(patsubst android/%,$(ANDROID_BUILD)/%,$(ANDROID_XML_RES))

DRAWABLE_DIR = $(ANDROID_BUILD)/res/drawable
RAW_DIR = $(ANDROID_BUILD)/res/raw

ifeq ($(TESTING),y)
ICON_SVG = $(topdir)/Data/graphics/logo_red.svg
else
ICON_SVG = $(topdir)/Data/graphics/logo.svg
endif

ICON_WHITE_SVG = $(topdir)/Data/graphics/logo_white.svg

$(ANDROID_BUILD)/res/drawable-ldpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-ldpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(ANDROID_BUILD)/res/drawable/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(ANDROID_BUILD)/res/drawable-hdpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(ANDROID_BUILD)/res/drawable-xhdpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=96 $< -o $@

$(ANDROID_BUILD)/res/drawable-xxhdpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-xxhdpi/dirstamp
	$(Q)rsvg-convert --width=144 $< -o $@

$(ANDROID_BUILD)/res/drawable-xxxhdpi/icon.png: $(ICON_SVG) | $(ANDROID_BUILD)/res/drawable-xxxhdpi/dirstamp
	$(Q)rsvg-convert --width=192 $< -o $@

$(ANDROID_BUILD)/res/drawable/notification_icon.png: $(ICON_WHITE_SVG) | $(ANDROID_BUILD)/res/drawable/dirstamp
	$(Q)rsvg-convert --width=24 $< -o $@

$(ANDROID_BUILD)/res/drawable-hdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(ANDROID_BUILD)/res/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(ANDROID_BUILD)/res/drawable-xhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(ANDROID_BUILD)/res/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(ANDROID_BUILD)/res/drawable-xxhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(ANDROID_BUILD)/res/drawable-xxhdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(ANDROID_BUILD)/res/drawable-xxxhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(ANDROID_BUILD)/res/drawable-xxxhdpi/dirstamp
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
	$(ANDROID_BUILD)/res/drawable-xhdpi/icon.png \
	$(ANDROID_BUILD)/res/drawable-xxhdpi/icon.png \
	$(ANDROID_BUILD)/res/drawable-xxxhdpi/icon.png \
	$(ANDROID_BUILD)/res/drawable/notification_icon.png \
	$(ANDROID_BUILD)/res/drawable-hdpi/notification_icon.png \
	$(ANDROID_BUILD)/res/drawable-xhdpi/notification_icon.png \
	$(ANDROID_BUILD)/res/drawable-xxhdpi/notification_icon.png \
	$(ANDROID_BUILD)/res/drawable-xxxhdpi/notification_icon.png

ifeq ($(TESTING),y)
MANIFEST = android/testing/AndroidManifest.xml
else
MANIFEST = android/AndroidManifest.xml
endif

$(ANDROID_XML_RES_COPIES): $(ANDROID_BUILD)/%: android/%
	$(Q)-$(MKDIR) -p $(dir $@)
	$(Q)cp $< $@

$(ANDROID_BUILD)/resources.apk: $(PNG_FILES) $(SOUND_FILES) $(ANDROID_XML_RES_COPIES) | $(ANDROID_BUILD)/gen/dirstamp
	@$(NQ)echo "  AAPT"
	$(Q)$(AAPT) package -f -m --auto-add-overlay \
		--custom-package $(JAVA_PACKAGE) \
		-M $(MANIFEST) \
		-S $(ANDROID_BUILD)/res \
		-J $(ANDROID_BUILD)/gen \
		-I $(ANDROID_SDK_PLATFORM_DIR)/android.jar \
		-F $(ANDROID_BUILD)/resources.apk

# R.java is generated by aapt, when resources.apk is generated
$(ANDROID_BUILD)/gen/org/xcsoar/R.java: $(ANDROID_BUILD)/resources.apk

$(ANDROID_BUILD)/classes.dex: $(JAVA_SOURCES) $(ANDROID_BUILD)/gen/org/xcsoar/R.java | $(JAVA_CLASSFILES_DIR)/dirstamp
	@$(NQ)echo "  JAVAC   $(JAVA_CLASSFILES_DIR)"
	$(Q)$(JAVAC) -source 1.6 -target 1.6 -Xlint:-options \
		-cp $(ANDROID_SDK_PLATFORM_DIR)/android.jar:$(JAVA_CLASSFILES_DIR) \
		-d $(JAVA_CLASSFILES_DIR) $(ANDROID_BUILD)/gen/org/xcsoar/R.java \
		-h $(NATIVE_INCLUDE) \
		$(JAVA_SOURCES)
	@$(NQ)echo "  DX      $@"
	$(Q)$(DX) --dex --output $@ $(JAVA_CLASSFILES_DIR)

ifeq ($(FAT_BINARY),y)

# generate a "fat" APK file with binaries for all ABIs

ANDROID_LIB_BUILD =
ANDROID_THIRDPARTY_STAMPS =

# Example: $(eval $(call generate-abi,xcsoar,armeabi-v7a,ANDROID7))
define generate-abi

ANDROID_LIB_BUILD += $$(ANDROID_BUILD)/lib/$(2)/lib$(1).so

$$(ANDROID_BUILD)/lib/$(2)/lib$(1).so: $$(OUT)/$(3)/$$(XCSOAR_ABI)/bin/lib$(1).so | $$(ANDROID_BUILD)/lib/$(2)/dirstamp
	$$(Q)cp $$< $$@

ANDROID_THIRDPARTY_STAMPS += $$(OUT)/$(3)/thirdparty.stamp
$$(OUT)/$(3)/thirdparty.stamp:
	$$(Q)$$(MAKE) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) libs

$$(OUT)/$(3)/$$(XCSOAR_ABI)/bin/lib$(1).so: $$(OUT)/$(3)/thirdparty.stamp
	$$(Q)$$(MAKE) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) $$@

endef

# Example: $(eval $(call generate-abi,xcsoar))
define generate-all-abis
$(eval $(call generate-abi,$(1),armeabi-v7a,ANDROID7))
$(eval $(call generate-abi,$(1),x86,ANDROID86))
$(eval $(call generate-abi,$(1),arm64-v8a,ANDROIDAARCH64))
$(eval $(call generate-abi,$(1),x86_64,ANDROIDX64))
endef

$(foreach NAME,$(ANDROID_LIB_NAMES),$(eval $(call generate-all-abis,$(NAME))))

.PHONY: libs
libs: $(ANDROID_THIRDPARTY_STAMPS)

else # !FAT_BINARY

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

ANDROID_LIB_BUILD = $(patsubst %,$(ANDROID_ABI_DIR)/lib%.so,$(ANDROID_LIB_NAMES))
$(ANDROID_LIB_BUILD): $(ANDROID_ABI_DIR)/lib%.so: $(ABI_BIN_DIR)/lib%.so | $(ANDROID_ABI_DIR)/dirstamp
	$(Q)cp $< $@

endif # !FAT_BINARY


$(NATIVE_HEADERS): $(ANDROID_BUILD)/classes.dex

.DELETE_ON_ERROR: $(ANDROID_BUILD)/unsigned.apk
$(ANDROID_BUILD)/unsigned.apk: $(ANDROID_BUILD)/classes.dex $(ANDROID_BUILD)/resources.apk $(ANDROID_LIB_BUILD)
	@$(NQ)echo "  APK     $@"
	$(Q)cp $(ANDROID_BUILD)/resources.apk $@
	$(Q)cd $(dir $@) && zip -q -r $(notdir $@) classes.dex lib

# Generate ~/.android/debug.keystore, if it does not exists, as the official
# Android build tools do it:
$(HOME)/.android/debug.keystore:
	@$(NQ)echo "  KEYTOOL $@"
	$(Q)-$(MKDIR) -p $(HOME)/.android
	$(Q)$(KEYTOOL) -genkey -noprompt \
		-keystore $@ \
		-storepass android \
		-alias androiddebugkey \
		-keypass android \
		-dname "CN=Android Debug" \
		-keyalg RSA -keysize 2048 -validity 10000

$(ANDROID_BIN)/XCSoar-debug.apk: $(ANDROID_BUILD)/unsigned.apk $(HOME)/.android/debug.keystore | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(JARSIGNER) -keystore $(HOME)/.android/debug.keystore -storepass android -digestalg SHA1 -sigalg MD5withRSA -signedjar $@ $< androiddebugkey

$(ANDROID_BUILD)/XCSoar-release-unaligned.apk: $(ANDROID_BUILD)/unsigned.apk
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(JARSIGNER_RELEASE) -keystore $(ANDROID_KEYSTORE) -signedjar $@ $< $(ANDROID_KEY_ALIAS)

$(ANDROID_BIN)/XCSoar.apk: $(ANDROID_BUILD)/XCSoar-release-unaligned.apk | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  ALIGN   $@"
	$(Q)$(ZIPALIGN) -f 8 $< $@

endif
