ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
endif
endif

ifeq ($(platform), unix)
   TARGET := libsnes.so
   fpic := -fPIC
   SHARED := -shared
else ifeq ($(platform), osx)
   TARGET := libsnes.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else
   TARGET := snes.dll
   CC = gcc
   CXX = g++
   SHARED := -shared -static-libgcc -static-libstdc++ -s
endif

FCEU_DIR := ../fceu
UTILS_DIR := ../../utils

FCEU_SRC_DIRS := $(FCEU_DIR) $(FCEU_DIR)/boards $(FCEU_DIR)/input $(FCEU_DIR)/mappers $(FCEU_DIR)/utils $(UTILS_DIR)/zlib $(UTILS_DIR)/unzip

FCEU_CXXSRCS := $(foreach dir,$(FCEU_SRC_DIRS),$(wildcard $(dir)/*.cpp))
FCEU_CXXOBJ := $(FCEU_CXXSRCS:.cpp=.o)
FCEU_CSRCS := $(foreach dir,$(FCEU_SRC_DIRS),$(wildcard $(dir)/*.c))
FCEU_COBJ := $(FCEU_CSRCS:.c=.o)

OBJS := $(FCEU_COBJ) $(FCEU_CXXOBJ) libsnes.o

FCEU_DEFINES := -D__LIBSNES__ -DSOUND_QUALITY=0 -DPATH_MAX=1024 -DPSS_STYLE=1 -DLSB_FIRST=1 -DLOCAL_LE=1

CFLAGS += -O3 -g -std=gnu99 $(fpic) -Wno-write-strings $(FCEU_DEFINES)
CXXFLAGS += -O3 -g $(fpic) -Wno-write-strings $(FCEU_DEFINES)
LDFLAGS += -Wl,-no-undefined

INCDIRS := -I$(FCEU_DIR) -I$(UTILS_DIR)/zlib -I$(UTILS_DIR)/unzip
LIBS :=

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(SHARED) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCDIRS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

.PHONY: clean

