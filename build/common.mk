MAKEFLAGS += --no-builtin-rules --no-builtin-variables

include $(topdir)/build/verbose.mk
include $(topdir)/build/util.mk

SRC = $(topdir)/src
ENGINE_SRC_DIR = $(SRC)/Engine
DOC = $(topdir)/Doc
OUT = $(topdir)/output
TEST_SRC_DIR = $(topdir)/ex/Test
