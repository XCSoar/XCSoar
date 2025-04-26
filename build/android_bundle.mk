# This Makefile fragment builds the Android Bundle
# We're not using NDK's Makefiles because our Makefile is so big and
# complex, we don't want to duplicate that for another platform.

ifeq ($(TARGET),ANDROID)

ANDROID_KEYSTORE ?= $(HOME)/.android/mk.keystore
ANDROID_KEY_ALIAS ?= mk

ANDROID_OUTPUT_DIR = $(TARGET_OUTPUT_DIR)/android

ANDROID_BUILD = $(TARGET_OUTPUT_DIR)/$(XCSOAR_ABI)/build
ANDROID_BIN = $(TARGET_BIN_DIR)

ifeq ($(HOST_IS_DARWIN),y)
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-macosx
else
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-linux
endif
ANDROID_SDK_PLATFORM_DIR = $(ANDROID_SDK)/platforms/$(ANDROID_SDK_PLATFORM)
ANDROID_ABI_DIR = $(ANDROID_BUILD)/lib/$(ANDROID_APK_LIB_ABI)

JAVA_CLASSFILES_DIR = $(ANDROID_OUTPUT_DIR)/classes

ANDROID_BUILD_TOOLS_DIR = $(ANDROID_SDK)/build-tools/33.0.2
APKSIGNER = $(ANDROID_BUILD_TOOLS_DIR)/apksigner
ZIPALIGN = $(ANDROID_BUILD_TOOLS_DIR)/zipalign
AAPT = $(ANDROID_BUILD_TOOLS_DIR)/aapt
D8 = $(ANDROID_BUILD_TOOLS_DIR)/d8

ANDROID_LIB_NAMES = xcsoar

APKSIGN = $(APKSIGNER) sign
ifeq ($(V),2)
APKSIGN += --verbose
endif

APKSIGN_RELEASE = $(APKSIGN)

# The environment variable ANDROID_KEYSTORE_PASS may be used to
# specify the keystore password; if you don't set it, you will be
# asked interactively
ifeq ($(origin ANDROID_KEYSTORE_PASS),environment)
APKSIGN_RELEASE += --ks-pass env:ANDROID_KEYSTORE_PASS
endif

JAVA_PACKAGE = org.xcsoar

NATIVE_CLASSES := \
	FileProvider \
	TextEntryDialog \
	NativeView \
	EventBridge \
	NativeSensorListener \
	NativeInputListener \
	DownloadUtil \
	BatteryReceiver \
	NativePortListener \
	NativeDetectDeviceListener
NATIVE_SOURCES = $(patsubst %,android/src/%.java,$(NATIVE_CLASSES))
NATIVE_INCLUDE = $(TARGET_OUTPUT_DIR)/include
NATIVE_PREFIX = $(NATIVE_INCLUDE)/$(subst .,_,$(JAVA_PACKAGE))_
NATIVE_HEADERS = $(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES))

JAVA_SOURCES := \
	$(wildcard android/src/*.java) \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/CH34xIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/CP210xIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/CP2130Ids.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/FTDISioIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/Helpers.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/PL2303Ids.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/XdcVcpIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/AbstractWorkerThread.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/Buffer.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CDCSerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CH34xSerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CP2102SerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CP2130SpiDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/PL2303SerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/FTDISerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/SerialBuffer.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/SerialInputStream.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/SerialOutputStream.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSerialDebugger.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSerialInterface.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSpiDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSpiInterface.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/utils/HexData.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/utils/SafeUsbRequest.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/AnalogInput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/CapSense.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/Closeable.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/DigitalInput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/DigitalOutput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IcspMaster.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IOIOConnection.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IOIO.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/PulseInput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/PwmOutput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/Sequencer.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/SpiMaster.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/TwiMaster.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/Uart.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/exception/ConnectionLostException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/exception/IncompatibilityException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/exception/OutOfResourceException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/AbstractPin.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/AbstractResource.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/AnalogInputImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/Board.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/CapSenseImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/Constants.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/DigitalInputImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/DigitalOutputImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/FixedReadBufferedInputStream.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/FlowControlledOutputStream.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/FlowControlledPacketSender.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/GenericResourceAllocator.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IcspMasterImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IncapImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IncomingState.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/InterruptibleQueue.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IOIOImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IOIOProtocol.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/PwmImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/QueueInputStream.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/ResourceLifeCycle.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/ResourceManager.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SequencerImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SocketIOIOConnectionBootstrap.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SocketIOIOConnection.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SpecificResourceAllocator.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SpiMasterImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/TwiMasterImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/UartImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/Version.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/IOIOConnectionBootstrap.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/IOIOConnectionFactory.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/Log.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/NoRuntimeSupportException.java \
	android/ioio/IOIOLibAndroid/src/main/java/ioio/lib/spi/LogImpl.java \
	android/ioio/IOIOLibAndroid/src/main/java/ioio/lib/util/android/ContextWrapperDependent.java \
	android/ioio/IOIOLibAndroidAccessory/src/main/java/ioio/lib/android/accessory/AccessoryConnectionBootstrap.java \
	android/ioio/IOIOLibAndroidBluetooth/src/main/java/ioio/lib/android/bluetooth/BluetoothIOIOConnectionBootstrap.java \
	android/ioio/IOIOLibAndroidBluetooth/src/main/java/ioio/lib/android/bluetooth/BluetoothIOIOConnection.java \
	android/ioio/IOIOLibAndroidDevice/src/main/java/ioio/lib/android/device/DeviceConnectionBootstrap.java \
	android/ioio/IOIOLibAndroidDevice/src/main/java/ioio/lib/android/device/Streams.java

GEN_DIR = $(ANDROID_OUTPUT_DIR)/gen
RES_DIR = $(ANDROID_OUTPUT_DIR)/res

DRAWABLE_DIR = $(RES_DIR)/drawable
RAW_DIR = $(RES_DIR)/raw

ANDROID_XML_RES := $(wildcard android/res/*/*.xml)
ANDROID_XML_RES_COPIES := $(patsubst android/res/%,$(RES_DIR)/%,$(ANDROID_XML_RES))

ifeq ($(TESTING),y)
ICON_SVG = $(topdir)/Data/graphics/logo_red.svg
else
ICON_SVG = $(topdir)/Data/graphics/logo.svg
endif

ICON_WHITE_SVG = $(topdir)/Data/graphics/logo_white.svg

$(RES_DIR)/drawable-ldpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-ldpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(RES_DIR)/drawable/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(RES_DIR)/drawable-hdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(RES_DIR)/drawable-xhdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=96 $< -o $@

$(RES_DIR)/drawable-xxhdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-xxhdpi/dirstamp
	$(Q)rsvg-convert --width=144 $< -o $@

$(RES_DIR)/drawable-xxxhdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-xxxhdpi/dirstamp
	$(Q)rsvg-convert --width=192 $< -o $@

$(RES_DIR)/drawable/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable/dirstamp
	$(Q)rsvg-convert --width=24 $< -o $@

$(RES_DIR)/drawable-hdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(RES_DIR)/drawable-xhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(RES_DIR)/drawable-xxhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-xxhdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(RES_DIR)/drawable-xxxhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-xxxhdpi/dirstamp
	$(Q)rsvg-convert --width=96 $< -o $@

OGGENC = oggenc --quiet --quality 1

SOUNDS = fail insert remove beep_bweep beep_clear beep_drip
SOUND_FILES = $(patsubst %,$(RAW_DIR)/%.ogg,$(SOUNDS))

$(SOUND_FILES): $(RAW_DIR)/%.ogg: Data/sound/%.wav | $(RAW_DIR)/dirstamp
	@$(NQ)echo "  OGGENC  $@"
	$(Q)$(OGGENC) -o $@ $<

PNG1 := $(patsubst Data/bitmaps/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_BITMAPS))

$(PNG1): $(DRAWABLE_DIR)/%.png: Data/bitmaps/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG2 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_LAUNCH_ALL))
$(PNG2): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG3 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_SPLASH_80) $(BMP_SPLASH_160) $(BMP_TITLE_110) $(BMP_TITLE_320))
$(PNG3): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG4 := $(patsubst $(DATA)/icons/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_ICONS_ALL))
$(PNG4): $(DRAWABLE_DIR)/%.png: $(DATA)/icons/%.png | $(DRAWABLE_DIR)/dirstamp
	$(Q)cp $< $@

PNG5 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_DIALOG_TITLE) $(BMP_PROGRESS_BORDER))
$(PNG5): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

PNG_FILES = $(PNG1) $(PNG1b) $(PNG2) $(PNG3) $(PNG4) $(PNG5) \
	$(RES_DIR)/drawable-ldpi/icon.png \
	$(RES_DIR)/drawable/icon.png \
	$(RES_DIR)/drawable-hdpi/icon.png \
	$(RES_DIR)/drawable-xhdpi/icon.png \
	$(RES_DIR)/drawable-xxhdpi/icon.png \
	$(RES_DIR)/drawable-xxxhdpi/icon.png \
	$(RES_DIR)/drawable/notification_icon.png \
	$(RES_DIR)/drawable-hdpi/notification_icon.png \
	$(RES_DIR)/drawable-xhdpi/notification_icon.png \
	$(RES_DIR)/drawable-xxhdpi/notification_icon.png \
	$(RES_DIR)/drawable-xxxhdpi/notification_icon.png

ifeq ($(TESTING),y)
MANIFEST = android/testing/AndroidManifest.xml
else
MANIFEST = android/AndroidManifest.xml
endif

$(ANDROID_XML_RES_COPIES): $(RES_DIR)/%: android/res/%
	$(Q)-$(MKDIR) -p $(dir $@)
	$(Q)cp $< $@

$(ANDROID_OUTPUT_DIR)/resources.apk: $(PNG_FILES) $(SOUND_FILES) $(ANDROID_XML_RES_COPIES) $(MANIFEST) | $(GEN_DIR)/dirstamp
	@$(NQ)echo "  AAPT"
	$(Q)$(AAPT) package -f -m --auto-add-overlay \
		--custom-package $(JAVA_PACKAGE) \
		-M $(MANIFEST) \
		-S $(RES_DIR) \
		-J $(GEN_DIR) \
		-I $(ANDROID_SDK_PLATFORM_DIR)/android.jar \
		-F $(ANDROID_OUTPUT_DIR)/resources.apk

# R.java is generated by aapt, when resources.apk is generated
$(GEN_DIR)/org/xcsoar/R.java: $(ANDROID_OUTPUT_DIR)/resources.apk

$(ANDROID_OUTPUT_DIR)/classes.zip: $(JAVA_SOURCES) $(GEN_DIR)/org/xcsoar/R.java | $(JAVA_CLASSFILES_DIR)/dirstamp
	@$(NQ)echo "  JAVAC   $(JAVA_CLASSFILES_DIR)"
	$(Q)$(JAVAC) \
		-source 1.7 -target 1.7 \
		-Xlint:all \
		-Xlint:-deprecation \
		-Xlint:-options \
		-Xlint:-static \
		-cp $(ANDROID_SDK_PLATFORM_DIR)/android.jar:$(JAVA_CLASSFILES_DIR) \
		-d $(JAVA_CLASSFILES_DIR) $(GEN_DIR)/org/xcsoar/R.java \
		-h $(NATIVE_INCLUDE) \
		$(JAVA_SOURCES)
	$(Q)$(ZIP) -0 -r $(ANDROID_OUTPUT_DIR)/classes.zip $(JAVA_CLASSFILES_DIR)

# Note: desugaring causes crashes on Android 13 (Pixel 6); as a
# workaround, it's disabled for now.
$(ANDROID_OUTPUT_DIR)/classes.dex: $(ANDROID_OUTPUT_DIR)/classes.zip
	@$(NQ)echo "  D8      $@"
	$(Q)$(D8) \
		--no-desugaring \
		--min-api 21 \
		--output $(ANDROID_OUTPUT_DIR) $(ANDROID_OUTPUT_DIR)/classes.zip

ifeq ($(FAT_BINARY),y)

# generate a "fat" APK file with binaries for all ABIs

ANDROID_LIB_BUILD =
ANDROID_THIRDPARTY_STAMPS =

# Example: $(eval $(call generate-abi,xcsoar,armeabi-v7a,ANDROID7))
define generate-abi

ANDROID_LIB_BUILD += $$(ANDROID_BUILD)/lib/$(2)/lib$(1).so

# copy libxcsoar.so to ANDROIDFAT
$$(ANDROID_BUILD)/lib/$(2)/lib$(1).so: $$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1).so | $$(ANDROID_BUILD)/lib/$(2)/dirstamp
	$$(Q)cp $$< $$@

# build third-party libraries
ANDROID_THIRDPARTY_STAMPS += $$(TARGET_OUTPUT_DIR)/$(2)/thirdparty.stamp
$$(TARGET_OUTPUT_DIR)/$(2)/thirdparty.stamp: FORCE
	$$(Q)$$(MAKE) TARGET_OUTPUT_DIR=$$(TARGET_OUTPUT_DIR) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) libs

# build libxcsoar.so
$$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1).so: $(NATIVE_HEADERS) generate boost FORCE
	$$(Q)$$(MAKE) TARGET_OUTPUT_DIR=$$(TARGET_OUTPUT_DIR) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) $$@

# extract symbolication files for Google Play
ANDROID_SYMBOLICATION_BUILD += $$(ANDROID_BUILD)/symbols/$(2)/lib$(1).so
$$(ANDROID_BUILD)/symbols/$(2)/lib$(1).so: $$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1)-ns.so | $$(ANDROID_BUILD)/symbols/$(2)/dirstamp
	$$(Q)$$(TCPREFIX)objcopy$$(EXE) --strip-debug $$< $$@

endef

# Example: $(eval $(call generate-abi,xcsoar))
define generate-all-abis
$(eval $(call generate-abi,$(1),armeabi-v7a,ANDROID7))
$(eval $(call generate-abi,$(1),x86,ANDROID86))
$(eval $(call generate-abi,$(1),arm64-v8a,ANDROIDAARCH64))
$(eval $(call generate-abi,$(1),x86_64,ANDROIDX64))
endef

$(foreach NAME,$(ANDROID_LIB_NAMES),$(eval $(call generate-all-abis,$(NAME))))

.PHONY: libs compile
libs: $(ANDROID_THIRDPARTY_STAMPS)
compile: $(ANDROID_LIB_BUILD)

# Generate symbols.zip (symbolication file) for Google Play, which
# allows Google Play to show symbol names in stack traces.
$(TARGET_OUTPUT_DIR)/symbols.zip: $(ANDROID_SYMBOLICATION_BUILD)
	cd $(ANDROID_BUILD)/symbols && $(ZIP) $(abspath $@) */*.so

else # !FAT_BINARY

# add dependency to this source file
$(call SRC_TO_OBJ,$(SRC)/Android/Main.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/EventBridge.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeSensorListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeDetectDeviceListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/Battery.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativePortListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeInputListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/DownloadManager.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/TextEntryDialog.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/FileProvider.cpp): $(NATIVE_HEADERS)

ANDROID_LIB_BUILD = $(patsubst %,$(ANDROID_ABI_DIR)/lib%.so,$(ANDROID_LIB_NAMES))
$(ANDROID_LIB_BUILD): $(ANDROID_ABI_DIR)/lib%.so: $(ABI_BIN_DIR)/lib%.so | $(ANDROID_ABI_DIR)/dirstamp
	$(Q)cp $< $@

endif # !FAT_BINARY


$(NATIVE_HEADERS): $(ANDROID_OUTPUT_DIR)/classes.dex

.DELETE_ON_ERROR: $(ANDROID_BUILD)/unsigned.apk
$(ANDROID_BUILD)/unsigned.apk: $(ANDROID_OUTPUT_DIR)/classes.dex $(ANDROID_OUTPUT_DIR)/resources.apk $(ANDROID_LIB_BUILD)
	@$(NQ)echo "  APK     $@"
	$(Q)cp $(ANDROID_OUTPUT_DIR)/classes.dex $(dir $@)/
	$(Q)cp $(ANDROID_OUTPUT_DIR)/resources.apk $@
	$(Q)cd $(dir $@) && $(ZIP) -r $(notdir $@) classes.dex lib/*/*.so

.DELETE_ON_ERROR: $(ANDROID_BUILD)/aligned.apk
$(ANDROID_BUILD)/aligned.apk: $(ANDROID_BUILD)/unsigned.apk
	@$(NQ)echo "  ALIGN   $@"
	$(Q)$(ZIPALIGN) -f 8 $< $@

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

$(ANDROID_BIN)/XCSoar-debug.apk: $(ANDROID_BUILD)/aligned.apk $(HOME)/.android/debug.keystore | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(APKSIGN) --in $< --out $@ --debuggable-apk-permitted -ks $(HOME)/.android/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android

$(ANDROID_BIN)/XCSoar.apk: $(ANDROID_BUILD)/aligned.apk | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(APKSIGN_RELEASE) --in $< --out $@ -ks $(ANDROID_KEYSTORE) --ks-key-alias $(ANDROID_KEY_ALIAS)

endif
