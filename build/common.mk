MAKEFLAGS += --no-builtin-rules --no-builtin-variables

include $(topdir)/build/verbose.mk
include $(topdir)/build/util.mk

SRC = $(topdir)/src
ENGINE_SRC_DIR = $(SRC)/Engine
DOC = $(topdir)/doc
OUT = $(topdir)/output
TEST_SRC_DIR = $(topdir)/test/src
HOST_OUTPUT_DIR = $(OUT)/host
TARGET_OUTPUT_DIR = $(OUT)/$(TARGET_FLAVOR)
TARGET_BIN_DIR = $(TARGET_OUTPUT_DIR)/bin
