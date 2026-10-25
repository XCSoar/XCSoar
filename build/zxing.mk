# zxing-cpp reads and writes QR codes.  Android and iOS scan them.
# Linux (except the Kobo cross toolchain) and Apple build the codec,
# so those targets can draw a task QR code.  Kobo's musl CMake setup
# is not wired for this library.

ifeq ($(TARGET_IS_KOBO),y)
ZXING ?= n
else ifeq ($(call bool_or,$(TARGET_IS_LINUX),$(TARGET_IS_DARWIN)),y)
ZXING ?= y
else
ZXING ?= n
endif

ifeq ($(ZXING),y)

TARGET_CPPFLAGS += -DHAVE_ZXING

ifeq ($(USE_THIRDPARTY_LIBS),y)

$(eval $(call pkg-config-library,ZXING,zxing))

else

# Native UNIX has no third-party prefix, so build the pinned release
# here.  WRITERS=OLD is zxing's own QR encoder (a module grid) and does
# not pull in Zint.

ZXING_VERSION = 3.1.1
ZXING_URL = https://github.com/zxing-cpp/zxing-cpp/archive/v$(ZXING_VERSION).tar.gz
ZXING_TARBALL = $(DOWNLOAD_DIR)/$(notdir $(ZXING_URL))
ZXING_SRC = $(OUT)/src/zxing-cpp-$(ZXING_VERSION)
ZXING_BUILD = $(ARCH_OUTPUT_DIR)/build/zxing
ZXING_PREFIX = $(ARCH_OUTPUT_DIR)/lib/zxing
ZXING_LIB = $(ZXING_PREFIX)/lib/libZXing.a
ZXING_SHA256 = 7286b1e6ade66fe82b7c8208b4595deeb55d6486b410834fdc65702f46650542

$(ZXING_TARBALL): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(ZXING_URL) $(ZXING_URL) $(ZXING_SHA256) $(DOWNLOAD_DIR)

# CreateBarcode.cpp calls MultiFormatWriter, which this release
# marks deprecated.  The warning is inside the library.
# BarcodeFormats holds a std::vector.  _GLIBCXX_DEBUG changes that
# type, so this library has to see the same flags as the callers.
ZXING_CXX_FLAGS = -Wno-deprecated-declarations $(LIBSTDCXX_CPPFLAGS)

# The flags file is rewritten only when the text changes, so a later
# DEBUG_GLIBCXX=y build does not keep the library from the plain one.
.PHONY: zxing-cxxflags-force
zxing-cxxflags-force:

$(ZXING_PREFIX)/cxxflags.txt: zxing-cxxflags-force
	$(Q)mkdir -p $(dir $@)
	$(Q)echo '$(ZXING_CXX_FLAGS)' | cmp -s - $@ || echo '$(ZXING_CXX_FLAGS)' > $@

$(ZXING_LIB): $(ZXING_TARBALL) $(topdir)/build/zxing.mk $(ZXING_PREFIX)/cxxflags.txt
	$(Q)rm -rf $(ZXING_SRC) $(ZXING_BUILD)
	$(Q)mkdir -p $(OUT)/src $(ZXING_BUILD)
	$(Q)tar -xzf $(ZXING_TARBALL) -C $(OUT)/src
	$(Q)cmake -S $(ZXING_SRC) -B $(ZXING_BUILD) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_FLAGS="$(ZXING_CXX_FLAGS)" \
		-DCMAKE_INSTALL_PREFIX=$(abspath $(ZXING_PREFIX)) \
		-DCMAKE_INSTALL_LIBDIR=lib \
		-DBUILD_SHARED_LIBS=OFF \
		-DZXING_READERS=ON \
		-DZXING_WRITERS=OLD \
		-DZXING_ENABLE_1D=OFF \
		-DZXING_ENABLE_AZTEC=OFF \
		-DZXING_ENABLE_DATAMATRIX=OFF \
		-DZXING_ENABLE_MAXICODE=OFF \
		-DZXING_ENABLE_PDF417=OFF \
		-DZXING_ENABLE_QRCODE=ON \
		-DZXING_C_API=OFF \
		-DZXING_EXAMPLES=OFF \
		-DZXING_UNIT_TESTS=OFF \
		-DZXING_BLACKBOX_TESTS=OFF \
		-DZXING_DEPENDENCIES=LOCAL \
		-DZXING_DISABLE_PCH=ON
	$(Q)cmake --build $(ZXING_BUILD) --parallel
	$(Q)cmake --install $(ZXING_BUILD)

ZXING_CPPFLAGS += -isystem $(ZXING_PREFIX)/include
ZXING_LDLIBS += $(ZXING_LIB) -pthread

$(call SRC_TO_OBJ,$(SRC)/QR/QRCode.cpp): $(ZXING_LIB)

endif

endif
