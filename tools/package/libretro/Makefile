STATIC_LINKING := 0
AR             := ar

ifneq ($(V),1)
   Q := @
endif

ifneq ($(SANITIZER),)
   CFLAGS   := -fsanitize=$(SANITIZER) $(CFLAGS)
   CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
   LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -s),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -s)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -s)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -s)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),arm)
	arch = arm
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

CORE_DIR    += ../../../
VXT_DIR		:= $(CORE_DIR)/lib/vxt
VXTP_DIR	:= $(CORE_DIR)/lib/vxtp
TARGET_NAME := virtualxt
LIBM		    = -lm

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared
	LIBM :=
else ifneq (,$(findstring android,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_android.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif
else ifneq (,$(findstring ios,$(platform)))
	EXT ?= dylib
   TARGET := $(TARGET_NAME)_libretro_ios.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib
   DEFINES := -DIOS

   ifeq ($(IOSSDK),)
      IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
   endif
   ifeq ($(platform),ios-arm64)
      SHARED += -arch arm64 -isysroot $(IOSSDK)
      CFLAGS += -arch arm64 -isysroot $(IOSSDK)
   else
      SHARED += -arch armv7 -isysroot $(IOSSDK)
      CFLAGS += -arch armv7 -isysroot $(IOSSDK)
   endif

   ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
      SHARED += -miphoneos-version-min=8.0
      CFLAGS += -miphoneos-version-min=8.0
   else
      SHARED += -miphoneos-version-min=5.0
      CFLAGS += -miphoneos-version-min=5.0
   endif
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_vita.a
   CC = arm-vita-eabi-gcc
   AR = arm-vita-eabi-ar
   CXXFLAGS += -Wl,-q -Wall -O3
	STATIC_LINKING = 1
else
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--no-undefined
endif

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
   CXXFLAGS += -O0 -g
else
   CFLAGS += -O3 -DNDEBUG
   CXXFLAGS += -O3 -DNDEBUG
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)

CFLAGS   += -Wall -D__LIBRETRO__ $(fpic)
CXXFLAGS += -Wall -D__LIBRETRO__ $(fpic)

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	@$(if $(Q), $(shell echo echo LD $@),)
	$(Q)$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.c
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

%.o: %.cpp
	@$(if $(Q), $(shell echo echo CXX $<),)
	$(Q)$(CXX) $(CXXFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'
