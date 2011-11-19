#debug options, set 1 to enable
CELL_DEBUG_CONSOLE	= 0
CELL_DEBUG_FPS		= 0
CELL_DEBUG_LOGGER	= 0
CELL_DEBUG_PRINTF	= 0
CELL_DEBUG_PRINTF_DELAY = 700000
CELL_DEBUG_IP		= \"192.168.1.7\"
CELL_DEBUG_PORT		= 3490
MULTIMAN_SUPPORT	= 0
SDK_340			= 1

#specify build tools
CELL_BUILD_TOOLS	=	SNC
#explicitly set some cell sdk defaults
CELL_SDK		?=	/usr/local/cell
# CELL_GPU_TYPE (currently RSX is only one option)  
CELL_GPU_TYPE		=	RSX    
#CELL_PSGL_VERSION is debug, dpm or opt  
CELL_PSGL_VERSION	=	opt  

#Python binary - only useful for PSL1ght scripts
PYTHONBIN		= 	python2.7

CELL_MK_DIR		?=	$(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

# Geohot CFW defines
MKSELF_GEOHOT		=	make_self_npdrm
MKPKG_PSLIGHT		=	buildtools/PS3Py/pkg.py
PKG_FINALIZE		=	package_finalize

STRIP			=	$(CELL_HOST_PATH)/ppu/bin/ppu-lv2-strip

# important directories
UTILS_DIR		=	./utils
SRC_DIR			=	./src
CELL_FRAMEWORK_DIR	=	./src/cellframework
CELL_FRAMEWORK2_DIR	=	./src/cellframework2
FCEU_API_DIR		=	./src/fceumm

EMULATOR_VERSION	= 1.6


# all source directories
SOURCES			= $(SRC_DIR) \
                 $(SRC_DIR)/conf \
                 $(CELL_FRAMEWORK2_DIR)/input \
                 $(CELL_FRAMEWORK2_DIR)/audio \
                 $(CELL_FRAMEWORK_DIR)/fileio \
                 $(CELL_FRAMEWORK2_DIR)/utility \
                 $(FCEU_API_DIR)/mappers \
                 $(FCEU_API_DIR)/input \
                 $(FCEU_API_DIR)/boards

SOURCES_LAST   =  $(UTILS_DIR)/zlib

ifeq ($(CELL_DEBUG_CONSOLE),1)
PPU_CFLAGS     += -DCELL_DEBUG_CONSOLE
PPU_CXXFLAGS   += -DCELL_DEBUG_CONSOLE
L_CONTROL_CONSOLE_LDLIBS = -lcontrol_console_ppu
L_NET_CTL_LDLIBS = -lnetctl_stub
endif

ifeq ($(CELL_DEBUG_FPS),1)
PPU_CFLAGS     += -DCELL_DEBUG_FPS
PPU_CXXFLAGS   += -DCELL_DEBUG_FPS
endif

ifeq ($(CELL_DEBUG_PRINTF),1)
#Delay - change as necessary
DEFINES        = -DCELL_DEBUG_PRINTF -DCELL_DEBUG_PRINTF_DELAY
PPU_CFLAGS     += $(DEFINES)
PPU_CXXFLAGS   += $(DEFINES)
endif

ifeq ($(CELL_DEBUG_LOGGER),1)
DEFINES        = -DCELL_DEBUG_LOGGER
PPU_CFLAGS     += $(DEFINES)
PPU_CXXFLAGS   += $(DEFINES)
SOURCES        += $(CELL_FRAMEWORK2_DIR)/logger
L_NET_CTL_LDLIBS = -lnetctl_stub
endif

ifeq ($(MULTIMAN_SUPPORT),1)
PPU_CFLAGS     += -DMULTIMAN_SUPPORT=1
PPU_CXXFLAGS   += -DMULTIMAN_SUPPORT=1
   ifeq ($(shell uname), Linux)
      MKFSELF_WC		= $(HOME)/bin/make_self_wc
   else
      MKFSELF_WC		= $(CELL_SDK)/../bin/make_self_wc
   endif
endif

PPU_SRCS = $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp)) $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c)) \
$(FCEU_API_DIR)/cart.c \
$(FCEU_API_DIR)/cheat.c \
$(FCEU_API_DIR)/crc32.c \
$(FCEU_API_DIR)/fceu.c \
$(FCEU_API_DIR)/fds.c \
$(FCEU_API_DIR)/file.c \
$(FCEU_API_DIR)/general.c \
$(FCEU_API_DIR)/ines.c \
$(FCEU_API_DIR)/input.c \
$(FCEU_API_DIR)/md5.c \
$(FCEU_API_DIR)/memory.c \
$(FCEU_API_DIR)/myendian.c \
$(FCEU_API_DIR)/palette.c \
$(FCEU_API_DIR)/ppu.c \
$(FCEU_API_DIR)/sound.c \
$(FCEU_API_DIR)/state.c \
$(FCEU_API_DIR)/unif.c \
$(FCEU_API_DIR)/unzip.c \
$(FCEU_API_DIR)/vsuni.c \
$(FCEU_API_DIR)/x6502.c

PPU_SRCS += $(foreach dir,$(SOURCES_LAST),$(wildcard $(dir)/*.cpp)) $(foreach dir,$(SOURCES_LAST),$(wildcard $(dir)/*.c))

PPU_TARGET		=	fceu-ps3.ppu.elf


PPU_CXXFLAGS	+=	-I. -I$(FCEU_API_DIR) -I$(UTILS_DIR)/zlib -I$(UTILS_DIR)/sz -DPSS_STYLE=1 -DGEKKO -DPSGL -DPATH_MAX=1024 -DNDEBUG=1 -DSOUND_QUALITY=0
PPU_CFLAGS		+=	-I. -I$(FCEU_API_DIR) -I$(UTILS_DIR)/zlib -I$(UTILS_DIR)/sz -DPSS_STYLE=1 -DGEKKO -DPSGL -DPATH_MAX=1024 -DNDEBUG=1 -DSOUND_QUALITY=0

ifeq ($(CELL_BUILD_TOOLS),SNC)
PPU_CFLAGS		+= 	-Xbranchless=1 -Xfastmath=1 -Xassumecorrectsign=1 -Xassumecorrectalignment=1 \
				-Xunroll=1 -Xautovecreg=1 -DSNC_COMPILER 
PPU_CXXFLAGS		+=	-Xbranchless=1 -Xfastmath=1 -Xassumecorrectsign=1 -Xassumecorrectalignment=1 \
				-Xunroll=1 -Xautovecreg=1 -DSNC_COMPILER -Xc=cp+exceptions+rtti+wchar_t+bool+array_nd+tmplname
else
PPU_CFLAGS		+=	-funroll-loops -DGCC_COMPILER
PPU_CXXFLAGS		+=	-funroll-loops -DGCC_COMPILER
PPU_LDFLAGS		+=	-finline-limit=5000
PPU_LDFLAGS		+=	-Wl
endif

ifeq ($(SDK_340),1)
L_SYSUTIL_SCREENSHOT = -lsysutil_screenshot_stub
else
L_SYSUTIL_SCREENSHOT =
endif

PPU_LDLIBS		+= -L. -L$(CELL_SDK)/target/ppu/lib/PSGL/RSX/opt -ldbgfont -lPSGL -lPSGLcgc -lcgc \
			-lgcm_cmd -lgcm_sys_stub -lresc_stub -lm -lio_stub -lfs_stub -lsysutil_stub -lsysutil_game_stub $(L_SYSUTIL_SCREENSHOT) $(L_CONTROL_CONSOLE_LDLIBS) -lpngdec_stub -ljpgdec_stub \
			-lsysmodule_stub -laudio_stub -lpthread -lnet_stub $(L_NET_CTL_LDLIBS)

include $(CELL_MK_DIR)/sdk.target.mk

#standard pkg packaging for debug machines/normal jailbreak
.PHONY: pkg
pkg: $(PPU_TARGET)
ifeq ($(MULTIMAN_SUPPORT),1)
	$(MKFSELF_WC) $(PPU_TARGET) pkg/USRDIR/RELOAD.SELF
else
	$(MAKE_FSELF_NPDRM) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN
endif
	$(MAKE_PACKAGE_NPDRM) pkg/package.conf pkg

#massively reduced filesize using MKSELF_GEOHOT - use this for normal jailbreak builds
.PHONY: pkg-signed
pkg-signed: $(PPU_TARGET)
ifeq ($(MULTIMAN_SUPPORT),1)
	$(MKFSELF_WC) $(PPU_TARGET) pkg/USRDIR/RELOAD.SELF
else
	$(MKSELF_GEOHOT) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN FCEU90000
endif
	$(PYTHONBIN) $(MKPKG_PSLIGHT) --contentid IV0002-FCEU90000_00-SAMPLE0000000001 pkg/ fceunext-ps3-v$(EMULATOR_VERSION)-fw3.41.pkg

#use this to create a PKG for use with Geohot CFW 3.55
.PHONY: pkg-signed-cfw
pkg-signed-cfw:
ifeq ($(MULTIMAN_SUPPORT),1)
	$(MKFSELF_WC) $(PPU_TARGET) pkg/USRDIR/RELOAD.SELF
else
	$(MKSELF_GEOHOT) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN.SELF FCEU90000
endif
	$(PYTHONBIN) $(MKPKG_PSLIGHT) --contentid IV0002-FCEU90000_00-SAMPLE0000000001 pkg/ fceunext-ps3-v$(EMULATOR_VERSION)-cfw3.55.pkg
	$(PKG_FINALIZE) fceunext-ps3-v$(EMULATOR_VERSION)-cfw3.55.pkg
