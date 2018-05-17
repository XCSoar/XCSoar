SRC = $(topdir)/src
ENGINE_SRC_DIR = $(SRC)/Engine
DOC = $(topdir)/doc
OUT = $(topdir)/output
DATA = $(OUT)/data
TEST_SRC_DIR = $(topdir)/test/src
PYTHON_SRC = $(topdir)/python/src
HOST_OUTPUT_DIR = $(OUT)/host
DOWNLOAD_DIR = $(OUT)/download
TARGET_OUTPUT_DIR = $(OUT)/$(TARGET_FLAVOR)
TARGET_BIN_DIR = $(TARGET_OUTPUT_DIR)/bin

ABI_OUTPUT_DIR = $(TARGET_OUTPUT_DIR)/$(XCSOAR_ABI)
ABI_BIN_DIR = $(TARGET_OUTPUT_DIR)/$(XCSOAR_ABI)/bin
