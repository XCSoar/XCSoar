# XCSOAR_ABI specifies the ABI variant currently selected.  For
# example, DEBUG=n and DEBUG=y builds have a different incompatible
# ABI.  The XCSOAR_ABI value is used to build an output path, so
# different ABI variants are never combined in one build.

ifeq ($(DEBUG),y)
  ifeq ($(ANDROID_LEGACY),y)
    XCSOAR_ABI := dbg_legacy
  else
    XCSOAR_ABI := dbg
  endif
else
  ifeq ($(ANDROID_LEGACY),y)
    XCSOAR_ABI := opt_legacy
  else
    XCSOAR_ABI := opt
  endif
endif
