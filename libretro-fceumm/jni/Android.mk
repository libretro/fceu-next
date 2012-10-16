LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := retro
FCEU_SRC_DIRS := ../../src-fceumm ../../src-fceumm/boards ../../src-fceumm/input ../../src-fceumm/mappers

FCEU_CSRCS := $(foreach dir,$(FCEU_SRC_DIRS),$(wildcard $(dir)/*.c))

LOCAL_SRC_FILES  = ../libretro.c ../memstream.c $(FCEU_CSRCS)

LOCAL_CFLAGS = -DINLINE=inline -DSOUND_QUALITY=0 -DPATH_MAX=1024 -DPSS_STYLE=1 -DLSB_FIRST -D__LIBRETRO__ -DHAVE_ASPRINTF

include $(BUILD_SHARED_LIBRARY)
