MAKEFLAGS += --no-builtin-rules --no-builtin-variables

include $(topdir)/build/verbose.mk
include $(topdir)/build/util.mk

SRC = $(topdir)/Common/Source
HDR = $(topdir)/Common/Header
ENGINE_SRC_DIR = $(topdir)/src/Engine
OUT = $(topdir)/output
