# XCSOAR_ABI specifies the ABI variant currently selected.  For
# example, DEBUG=n and DEBUG=y builds have a different incompatible
# ABI.  The XCSOAR_ABI value is used to build an output path, so
# different ABI variants are never combined in one build.

ifeq ($(DEBUG),y)
  XCSOAR_ABI = dbg
else
  XCSOAR_ABI = opt
endif
